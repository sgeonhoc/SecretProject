//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#include "VRoidAuthActor.h"
#include "http.h"
#include "Tasks/Task.h"
#include "Kismet/KismetSystemLibrary.h"

#include "VRoidLogger.h"
#include "VRoidCharacterModel.h"
#include "VRoidSdkCoreFunctionLibrary.h"
#include "Components/VRoidAuthComponent.h"
#include "Components/VRoidAccountComponent.h"
#include "Subsystems/VRoidGameInstanceSubsystem.h"

AVRoidAuthActor::AVRoidAuthActor()
	: DefaultContainerType(ECharacterContainerType::Account)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	VRoidAuthComponent = CreateDefaultSubobject<UVRoidAuthComponent>(TEXT("VRoidAuthComponent"));
	VRoidAccountComponent = CreateDefaultSubobject<UVRoidAccountComponent>(TEXT("VRoidAccountComponent"));
}

void AVRoidAuthActor::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	if (IsDrawDebugBoundingBox)
	{
		SetActorTickEnabled(true);
	}
#endif // !UE_BUILD_SHIPPING

	CreateSdkWithReAuthorization();
	if (const auto World = GetWorld();
		World != nullptr && bReplicates && ShouldUseMultiplayFlow())
	{
		MultiplayBeginPlayBP();
		// Allow SeamlessTravel.
		if (HasAuthority())
		{
			GEngine->Exec(World, TEXT("net.AllowPIESeamlessTravel true"));
		}
	}
	// If pre-authentication, get a list of StaffPicks characters.
	if (VRoidAuthComponent && VRoidAuthComponent->HasSdk())
	{
		if (VRoidAccountComponent->ValidateAccountFile() == false)
		{
			GetStaffPicksCharactersAsync();
		}
	}
}

void AVRoidAuthActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Enable the Subsystem's MultiplayFlow if NetMode or VRoidNetMode is Multiplay.
	if (UVRoidSdkCoreFunctionLibrary::IsMultiplayMode(this))
	{
		VRoidNetMode = EVRoidNetMode::Multiplay;
	}
	const auto VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this);
	if (VRoidSubsystem == nullptr)
	{
		return;
	}
	if (ShouldUseMultiplayFlow())
	{
		VRoidSubsystem->EnableUseMultiplayFlow();
	}
	// If id is empty, copy it from the subsystem.
	if (IsEmptyId() && VRoidSubsystem->IsEmptyId() == false)
	{
		ApplicationId = VRoidSubsystem->GetApplicationId();
		SecretKey = VRoidSubsystem->GetSecretKey();
	}
}

void AVRoidAuthActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if !UE_BUILD_SHIPPING
	// Draw BoundingBox for Debug when IsDrawDebugBoundingBox is enabled in non-shipping builds.
	if (PreviewActor == nullptr && IsDrawDebugBoundingBox == false)
	{
		return;
	}
	// This code is provided as a sample to demonstrate that the BoundingBox can be obtained.
	// The current implementation is not versatile because the position depends on the Level.
	constexpr float OffsetRoot = 74.0f;
	FVector ActorRootLocation(PreviewActorSpawnLocation);
	ActorRootLocation.Z -= OffsetRoot;
	const FVector Center(ModelBoundingBox.Center + ActorRootLocation);
	UKismetSystemLibrary::DrawDebugBox(this, Center, ModelBoundingBox.Size, FLinearColor::Red);

	constexpr float DebugPointSize = 15.0f;
	const FVector MinPoint(ModelBoundingBox.Min + ActorRootLocation);
	UKismetSystemLibrary::DrawDebugPoint(this, MinPoint, DebugPointSize, FLinearColor::Yellow);
	const FVector MaxPoint(ModelBoundingBox.Max + ActorRootLocation);
	UKismetSystemLibrary::DrawDebugPoint(this, MaxPoint, DebugPointSize, FLinearColor::Yellow);
#endif
}

bool AVRoidAuthActor::InitializeAuthorization(UVRoidGameInstanceSubsystem* const VRoidSubsystem)
{
	const std::string AccessToken = VRoidAccountComponent->GetAccount().access_token;
	FString PixivUserId(TEXT(""));
	FString PixivUserName(TEXT(""));
	FString PixivUserIconUrl(TEXT(""));
	InitializeSdk = VRoidAuthComponent->InitializeSdk(AccessToken, PixivUserId, PixivUserName, PixivUserIconUrl);

	if (InitializeSdk)
	{
		DefaultContainerType = ECharacterContainerType::Account;
		OnSucceedAuthorizationBP(PixivUserName, PixivUserIconUrl);
		if (VRoidSubsystem == nullptr)
		{
			VROID_ERROR(TEXT("Failed to get VRoidGameInstanceSubSystem. Login state has not been updated successfully."));
			return false;
		}
		VRoidSubsystem->SetIsLogin(true, PixivUserId);
	}
	return InitializeSdk;
}

void AVRoidAuthActor::CreateSdkWithReAuthorization()
{
	const auto VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this);
	if (VRoidSubsystem == nullptr)
	{
		VROID_ERROR(TEXT("Failed to get VRoidGameInstanceSubSystem."));
		return;
	}
	if (IsEmptyId())
	{
		VROID_ERROR(TEXT("Failed to authenticate because the ApplicationId or SecretKey is empty."));
		return;
	}
	if (VRoidSubsystem->IsEmptyId())
	{
		VRoidSubsystem->Init(ApplicationId, SecretKey, false);
	}
	if (VRoidAuthComponent && VRoidAuthComponent->HasSdk(false) == false)
	{
		VRoidAuthComponent->Init(ApplicationId, SecretKey);
	}
	if (VRoidAuthComponent->HasSdk() == false)
	{
		return;
	}
	if (VRoidAccountComponent->ValidateAccountFile() == false || VRoidAccountComponent->LoadAccountJson() == false)
	{
		return;
	}
	const auto Account = VRoidAccountComponent->GetAccount();
	if (ShouldUseMultiplayFlow())
	{
		if (const FString Scope(Account.scope.c_str()); Scope.Contains("multiplay") == false)
		{
			IsReAuthorization = true;
			return;
		}
		if (IsRuntime)
		{
			if (UVRoidSdkCoreFunctionLibrary::IsAccessTokenExpired(Account))
			{
				if (const auto Url = VRoidAuthComponent->AuthRefreshUrl(Account.refresh_token); Url.IsEmpty() == false)
				{
					ProcessRequest(TEXT("POST"), Url, &AVRoidAuthActor::OnAuthResponseReceived);
				}
				return;
			}
			InitializeAuthorization(VRoidSubsystem);
			return;
		}
	}
	if (const auto Url = VRoidAuthComponent->AuthRefreshUrl(Account.refresh_token); Url.IsEmpty() == false)
	{
		ProcessRequest(TEXT("POST"), Url, &AVRoidAuthActor::OnAuthResponseReceived);
		return;
	}
}

bool AVRoidAuthActor::IsValidSharedHttp(const FHttpRequestPtr& Request, const FHttpResponsePtr& Response, const FString& FunctionName)
{
	if (Request.IsValid() == false)
	{
		VROID_ERROR(TEXT("Invalid smart pointer in Request (FunctionName:%s)."), *FunctionName);
		return false;
	}
	if (Response.IsValid() == false)
	{
		VROID_ERROR(TEXT("Invalid smart pointer in Response (FunctionName:%s)."), *FunctionName);
		return false;
	}
	return true;
}

bool AVRoidAuthActor::CheckResponseCode(const int32 ResponseCode, bool bSuccessful, const TFunction<void()>& FailedCallback)
{
	if (bSuccessful && ResponseCode == EHttpResponseCodes::Ok)
	{
		return true;
	}
	if (ResponseCode >= EHttpResponseCodes::ServerError)
	{
		VROID_ERROR(TEXT("Server Error:(%d)"), ResponseCode);
	}
	else if (ResponseCode >= EHttpResponseCodes::BadRequest)
	{
		VROID_ERROR(TEXT("Client Error:(%d)"), ResponseCode);
	}
	if (FailedCallback)
	{
		FailedCallback();
	}
	return false;
}

/*
 * If the access token has expired with a Client error, re-authenticate with refresh_token and Retry the Request.
 */
bool AVRoidAuthActor::OnReAuthAndRetryRequest(const FHttpRequestPtr& Request, const FHttpResponsePtr& Response,
                                              void (AVRoidAuthActor::* RetryRequest)(const FHttpRequestPtr, const FHttpResponsePtr, bool))
{
	if (IsValidSharedHttp(Request, Response, TEXT("OnReAuthAndRetryRequest")) == false)
	{
		return false;
	}
	const int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode >= EHttpResponseCodes::ServerError)
	{
		return false;
	}
	if (ResponseCode >= EHttpResponseCodes::BadRequest)
	{
		if (IsReAuthorization == false)
		{
			if (AuthRefreshRequest(Response->GetContentAsString()))
			{
				ProcessRequest(Request->GetVerb(), Request->GetURL(), RetryRequest);
				return true;
			}
		}
	}
	return false;
}

bool AVRoidAuthActor::AuthRefreshRequest(const FString& Content)
{
	if (VRoidAuthComponent->HasSdk() == false ||  VRoidAccountComponent->HasAccount() == false)
	{
		IsReAuthorization = true;
		return false;
	}

	if (TSharedPtr<FJsonObject> OutObject; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Content), OutObject))
	{
		if (const TSharedPtr<FJsonObject>* Error; OutObject->TryGetObjectField(TEXT("error"), Error))
		{
			if (FString Message; Error->Get()->TryGetStringField(TEXT("message"), Message))
			{
				if (Message.Contains("access token expired"))
				{
					const auto& RefreshToken(VRoidAccountComponent->GetAccount().refresh_token);
					if (const auto Url = VRoidAuthComponent->AuthRefreshUrl(RefreshToken); Url.IsEmpty() == false)
					{
						ProcessRequest(TEXT("POST"), Url, &AVRoidAuthActor::OnAuthResponseReceived);
					}
					IsReAuthorization = false;
					return true;
				}
			}
		}
	}
	IsReAuthorization = true;
	return false;
}

/*
 * @param verb	The HTTP verb for the request. GET, POST.
 */
bool AVRoidAuthActor::ProcessRequest(const FString& Verb, const FString& Url,
                                     void (AVRoidAuthActor::* Callback)(const FHttpRequestPtr, const FHttpResponsePtr, bool),
                                     const TMap<FString, FString>& Params)
{
	if ((Verb.Equals(TEXT("GET")) || Verb.Equals(TEXT("POST"))) == false)
	{
		VROID_ERROR(TEXT("Use GET or POST as request parameters."));
		return false;
	}
	const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(Verb);
	HttpRequest->SetHeader(vroid::api::VERSION_HEADER, vroid::api::VERSION);
	if (Url.Contains(TEXT("refresh_token=")) == false)
	{
		const FString AccessToken(VRoidAccountComponent->GetAccount().access_token.c_str());
		HttpRequest->SetHeader(vroid::api::AUTH_HEADER, vroid::api::AUTH_SCHEME_PREFIX + AccessToken);
	}
	if (Params.IsEmpty())
	{
		HttpRequest->SetURL(Url);
	}
	else
	{
		FString Param;
		bool IsFirstParam = true;
		for (const auto& p : Params)
		{
			if (p.Key.IsEmpty() || p.Value.IsEmpty())
			{
				continue;
			}
			Param += (IsFirstParam ? TEXT("?") : TEXT("&")) + p.Key + TEXT("=") + p.Value;
			IsFirstParam = false;
		}
		HttpRequest->SetURL(Url + Param);
	}
	HttpRequest->OnProcessRequestComplete().BindUObject(this, Callback);

	return HttpRequest->ProcessRequest();
}

void AVRoidAuthActor::OnCharactersResponseReceived(const FHttpRequestPtr Request, const FHttpResponsePtr Response, const bool bSuccessful)
{
	if (IsValidSharedHttp(Request, Response, TEXT("OnCharactersResponseReceived")) == false)
	{
		return;
	}
	if (CheckResponseCode(Response->GetResponseCode(), bSuccessful))
	{
		OnReceiveCharacterArray(Response->GetContentAsString(), Request->GetURL());
		return;
	}
	if (OnReAuthAndRetryRequest(Request, Response, &AVRoidAuthActor::OnCharactersResponseReceived))
	{
		return;
	}
	VROID_ERROR(TEXT("Failed get character array."));
}

/*
 * The properties of the character are retrieved, but currently only the BoundingBox is saved.
 */
void AVRoidAuthActor::OnReceiveCharacterProperty(const FHttpRequestPtr Request, const FHttpResponsePtr Response, const bool bSuccessful)
{
	if (IsValidSharedHttp(Request, Response, TEXT("OnReceiveCharacterProperty")) == false)
	{
		return;
	}
	if (CheckResponseCode(Response->GetResponseCode(), bSuccessful))
	{
		TSharedPtr<FJsonObject> rawCharacterData;
		if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()), rawCharacterData) == false)
		{
			VROID_ERROR(TEXT("Can not Deserialize VRM Character model property data Json Object."));
			return;
		}

		if (const TSharedPtr<FJsonObject>* CharacterObject; rawCharacterData->TryGetObjectField(TEXT("data"), CharacterObject))
		{
			const FVRMCharacterModelProperty& Deserialized = FVRMCharacterModelProperty::Deserialize(CharacterObject->ToSharedRef());
			ModelBoundingBox = Deserialized.BoundingBox;
		}
		return;
	}
	if (OnReAuthAndRetryRequest(Request, Response, &AVRoidAuthActor::OnReceiveCharacterProperty))
	{
		return;
	}
	VROID_ERROR(TEXT("Failed get character property."));
}

void AVRoidAuthActor::OnAuthResponseReceived(const FHttpRequestPtr Request, const FHttpResponsePtr Response, const bool bSuccessful)
{
	if (IsValidSharedHttp(Request, Response, TEXT("OnAuthResponseReceived")) == false)
	{
		return;
	}

	const bool IsRefresh(Request->GetURL().Contains(TEXT("refresh_token=")));
	if (Response == nullptr)
	{
		VROID_ERROR(TEXT("Authentication has been performed, but Response is NULL."));
		OnFailedAuthorizationBP(IsRefresh);
		return;
	}
	if (const TFunction<void()> FailedCallback = [&]()
	{
		OnFailedAuthorizationBP(IsRefresh);
	}; CheckResponseCode(Response->GetResponseCode(), bSuccessful, FailedCallback))
	{
		if (VRoidAccountComponent->WriteAccountJson(Response->GetContentAsString()) == false)
		{
			VROID_ERROR(TEXT("Failed to save retrieved account information."));
			return;
		}
		if (VRoidAccountComponent->LoadAccountJson() == false)
		{
			VROID_ERROR(TEXT("Failed to load account information."));
			return;
		}
		InitializeAuthorization(UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this));
		return;
	}

	if (IsRefresh)
	{
		VROID_ERROR(TEXT("Failed Client authentication with refresh token."));
	}
	else
	{
		VROID_ERROR(TEXT("Failed client auth."));
	}
}

void ReplaceNextCharacterMap(TMap<ECharacterContainerType, FString>& Map, const ECharacterContainerType Type, const FString& Next)
{
	if (Next.IsEmpty())
	{
		return;
	}
	FString left, next_id;
	Next.Split("max_id=", &left, &next_id);
	if (Map.Contains(Type))
	{
		Map.Remove(Type);
	}
	Map.Emplace(Type, next_id);
}

void AVRoidAuthActor::OnGetCharacters(const TArray<FVRMCharacterModelMinimal>& Characters, const FString& Href, const FString& Url)
{
	ECharacterContainerType containerType;
	if (Url.Contains(TEXT("account")))
	{
		containerType = ECharacterContainerType::Account;
	}
	else if (Url.Contains(TEXT("staff_picks")))
	{
		containerType = ECharacterContainerType::StaffPicks;
	}
	else if (Url.Contains(TEXT("hearts")))
	{
		containerType = ECharacterContainerType::Hearts;
	}
	else
	{
		return;
	}

	if (InitializeSdk)
	{
		if (Characters.Num() <= 0)
		{
			switch (containerType)
			{
			case ECharacterContainerType::Account:
				DefaultContainerType = ECharacterContainerType::Hearts;
				return;
			case ECharacterContainerType::Hearts:
				DefaultContainerType = ECharacterContainerType::StaffPicks;
				return;
			case ECharacterContainerType::StaffPicks:
			default:
				return;
			}
		}
	}
	else
	{
		DefaultContainerType = ECharacterContainerType::StaffPicks;
	}

	ReplaceNextCharacterMap(NextCharacterMap, containerType, Href);
	// Only the list of characters in StaffPicks when not logged in is automatically retrieved in its entirety.
	if (InitializeSdk == false && DefaultContainerType == ECharacterContainerType::StaffPicks)
	{
		if (const auto MaxId = NextCharacterMap.Find(containerType))
		{
			FString left, countStr;
			Url.Split(TEXT("count="), &left, &countStr);
			if (const int Count = FCString::Atoi(*countStr); 0 < Count)
			{
				GetCharactersAsync(containerType, Count, *MaxId);
			}
		}
	}
	OnGetCharactersBP(containerType, Characters);
}

void AVRoidAuthActor::OnReceiveCharacterArray(const FString& Content, const FString& Url)
{
	TSharedPtr<FJsonObject> rawCharacterData;
	if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Content), rawCharacterData) == false)
	{
		VROID_ERROR(TEXT("Can not Deserialize VRM Character data Json Object."));
		return;
	}

	TArray<FVRMCharacterModelMinimal> characters;
	const auto EmplaceCharacter = [&](const FVRMCharacterModelMinimal& Model, const bool IsAuthor)
	{
		if (Model.Name.IsEmpty() == false && Model.ModelId.IsEmpty() == false)
		{
			if (IsAuthor)
			{
				characters.Emplace(Model);
				return;
			}
			if (Model.IsDownloadable)
			{
				if (Model.License.CharacterizationAllowedUser.Contains("author") == false)
				{
					characters.Emplace(Model);
				}
			}
		}
	};

	if (const TArray<TSharedPtr<FJsonValue>>* Data; rawCharacterData->TryGetArrayField(TEXT("data"), Data))
	{
		const bool IsAuthor(Url.Contains(TEXT("account")));
		const bool HasStaffPicks(Url.Contains(TEXT("staff_picks")));
		for (const auto& CharacterObject : *Data)
		{
			if (const TSharedPtr<FJsonObject>* Object; CharacterObject->TryGetObject(Object))
			{
				const auto& Model(HasStaffPicks
									  ? FStaffPicksCharacterModelMinimal::Deserialize(*Object).CharacterModel
									  : FVRMCharacterModelMinimal::Deserialize(*Object));
				EmplaceCharacter(Model, IsAuthor);
			}
		}
	}
	else
	{
		VROID_ERROR(TEXT("Can not get field \"data\" from VRM Character data Json Object."));
		return;
	}

	FString href;
	if (const TSharedPtr<FJsonObject>* Links; rawCharacterData->TryGetObjectField(TEXT("_links"), Links))
	{
		if (const TSharedPtr<FJsonObject>* Next; Links->Get()->TryGetObjectField(TEXT("next"), Next))
		{
			Next->Get()->TryGetStringField(TEXT("href"), href);
		}
		OnGetCharacters(characters, href, Url);
		return;
	}
	VROID_WARNING(TEXT("Can not get field \"next.href\" from VRM Character data Json Object."));
	OnGetCharacters(characters, href, Url);
}

void AVRoidAuthActor::GetCharactersAsync(const ECharacterContainerType ContainerType, const int Count, const FString& MaxId)
{
	if (VRoidAuthComponent->HasSdk() == false)
	{
		VROID_FATAL(TEXT("VRoid sdk is not initialized."));
		return;
	}
	TMap<FString, FString> params;
	if (MaxId.IsEmpty() == false)
	{
		params.Emplace("max_id", MaxId);
	}
	params.Emplace("count", FString::FromInt(Count));

	if (const auto Endpoint = VRoidAuthComponent->CharacterEndPoint(ContainerType); Endpoint.IsEmpty() == false)
	{
		ProcessRequest(TEXT("GET"), Endpoint, &AVRoidAuthActor::OnCharactersResponseReceived, params);
	}
	if (NextCharacterMap.Contains(ContainerType))
	{
		NextCharacterMap.Remove(ContainerType);
	}
}

void AVRoidAuthActor::GetCharacterPropertyAsync(const FString& ModelId)
{
	if (const auto Url = VRoidAuthComponent->CharacterPropertyEndPoint(ModelId); Url.IsEmpty() == false)
	{
		ProcessRequest(TEXT("GET"), Url, &AVRoidAuthActor::OnReceiveCharacterProperty);
	}
}

void AVRoidAuthActor::LaunchAuthURL() const
{
	if (const FString Code(VRoidAuthComponent->AuthCode(ShouldUseMultiplayFlow()));
		FPlatformProcess::CanLaunchURL(*Code))
	{
		UKismetSystemLibrary::LaunchURL(Code);
		return;
	}
	VROID_ERROR(TEXT("The generated authentication code (URL) is incorrect."));
}

void AVRoidAuthActor::AuthRequest(const FString& AuthCode)
{
	if (const auto Url = VRoidAuthComponent->AuthUrl(AuthCode); Url.IsEmpty() == false)
	{
		ProcessRequest(TEXT("POST"), Url, &AVRoidAuthActor::OnAuthResponseReceived);
	}
}

void AVRoidAuthActor::GetStaffPicksCharactersAsync(const int Count)
{
	GetCharactersAsync(ECharacterContainerType::StaffPicks, Count);
}

void AVRoidAuthActor::GetAllCharactersAsync(const int Count)
{
	GetCharactersAsync(ECharacterContainerType::Account, Count);
	GetCharactersAsync(ECharacterContainerType::Hearts, Count);
	GetCharactersAsync(ECharacterContainerType::StaffPicks, Count);
}

void AVRoidAuthActor::DownloadAndSaveVRM(const FString& ModelId, const FString& ModelName, const bool IsOverride)
{
	VROID_DISPLAY(TEXT("Attempt download and save VRM file"));

	if (VRoidAuthComponent->HasSdk() == false || InitializeSdk == false)
	{
		VROID_ERROR(TEXT("SDK initialization is not complete."));
		return;
	}

	const FString VrmSavedRoot(FPaths::ProjectSavedDir() / "VRoid" / "vrm");
	if (auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile(); PlatformFile.DirectoryExists(*VrmSavedRoot) == false)
	{
		if (PlatformFile.CreateDirectory(*VrmSavedRoot) == false)
		{
			VROID_WARNING(TEXT("The specified directory does not exist. Also, a new one could not be created."));
			return;
		}
	}

	// If the override flag is set and there is already a DLed model, only Callback is called and the process is terminated.
	const FString SaveFileName(ModelId + ".enc.vrm");
	const FString VrmPath(VrmSavedRoot / SaveFileName);
	if (IsOverride == false && FPaths::FileExists(VrmPath))
	{
		VROID_LOG(TEXT("%s file has exists (Id:%s)."), *ModelName, *ModelId);
		GetCharacterPropertyAsync(ModelId);
		OnDownloadVrmBP(VrmPath);
		return;
	}

	// Execute DL processing and polling in a worker thread.
	const auto World(GetWorld());
	if (World == nullptr)
	{
		VROID_ERROR(TEXT("Failed get World."));
		return;
	}
	const auto DownloadAndSaveTask = MakeShared<UE::Tasks::TTask<bool>, ESPMode::ThreadSafe>
	(
		UE::Tasks::Launch(TEXT("VRoidDownloadAndSaveTasks"), [this, ModelId, VrmPath]
		{
			const auto TryDownload = [&](const bool IsRetry = false)
			{
				if (IsRetry)
				{
					VROID_LOG(TEXT("Retry download vrm (Id:%s)."), *ModelId);
				}
				if (VRoidAuthComponent->DownloadVRM(ModelId))
				{
					VRoidAuthComponent->SaveVRM(VrmPath);
					GetCharacterPropertyAsync(ModelId);
					return true;
				}
				return false;
			};
			if (TryDownload())
			{
				return true;
			}
			// TODO: Include some kind of waiting process, such as retrying at the next frame.
			return TryDownload(true);
		})
	);
	const double StartTime = World->GetRealTimeSeconds();
	const auto PollingDownload = MakeShared<TFunction<void()>, ESPMode::ThreadSafe>();
	*PollingDownload = [this, World, PollingDownload, DownloadAndSaveTask, VrmPath, StartTime]
	{
		// The default setting is 15 seconds to determine ModelDL failure.
		if (constexpr float TimeoutSeconds = 15.0f; TimeoutSeconds < (World->GetRealTimeSeconds() - StartTime))
		{
			VROID_ERROR(TEXT("Timed out vrm download and save task."));
			OnFailedDownloadVrmBP(VrmPath);
			return;
		}
		if (DownloadAndSaveTask->IsValid() && DownloadAndSaveTask.Get().IsCompleted())
		{
			DownloadAndSaveTask->GetResult() ? OnDownloadVrmBP(VrmPath) : OnFailedDownloadVrmBP(VrmPath);
			return;
		}
		World->GetTimerManager().SetTimerForNextTick([PollingDownload] { (*PollingDownload)(); });
	};
	(*PollingDownload)();
}

bool AVRoidAuthActor::RegisterVrmAssetList(UVrmAssetListObject* const VrmAssetListObject) const
{
	if (VrmAssetListObject == nullptr)
	{
		VROID_ERROR(TEXT("VrmAssetListObject is empty."));
		return false;
	}
	const auto World = GetWorld();
	if (World == nullptr)
	{
		VROID_ERROR(TEXT("Failed get World."));
		return false;
	}
	const auto PC = World->GetFirstPlayerController();
	if (PC == nullptr)
	{
		VROID_ERROR(TEXT("Failed get first PlayerController."));
		return false;
	}
	const auto VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this);
	if (VRoidSubsystem == nullptr)
	{
		VROID_ERROR(TEXT("Failed get VRoidGameInstanceSubSystem."));
		return false;
	}	
	return VRoidSubsystem->UpdateVrmAssetList(PC, VrmAssetListObject);
}

FString AVRoidAuthActor::DownloadMultiplayLicenseId(const FString& ModelId) const
{
	if (ShouldUseMultiplayFlow())
	{
		if (const FString Id(VRoidAuthComponent->DownloadMultiplayLicenseId(ModelId));
			Id.IsEmpty() == false)
		{
			if (const auto VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this);
				VRoidSubsystem != nullptr)
			{
				VRoidSubsystem->SetDownloadMultiplayLicenseId(Id);
			}
			return Id;
		}
		VROID_ERROR(TEXT("Failed to obtain a download license for a model in multiplayer (Id: %s)."), *ModelId);
		return TEXT("");
	}
	VROID_LOG(TEXT("The processing of model license ID for multiplayer is skipped because the multiplay mode flag is not valid."));
	return TEXT("");
}

void AVRoidAuthActor::OnLoadNextDataChunk(const ECharacterContainerType ContainerType, const int Count)
{
	if (const auto MaxId = NextCharacterMap.Find(ContainerType))
	{
		GetCharactersAsync(ContainerType, Count, *MaxId);
	}
}

bool AVRoidAuthActor::IsVRoidStudioModel(const FVRMCharacterModelMinimal& Model)
{
	if (Model.ExporterVersion.Contains(TEXT("VRoidStudio-")))
	{
		return true;
	}
	return Model.ExporterVersion.Contains(TEXT("VRoid Studio-"));
}

bool AVRoidAuthActor::IsEmptyId() const
{
	if (ApplicationId.IsEmpty())
	{
		return true;
	}
	if (SecretKey.IsEmpty())
	{
		return true;
	}
	return false;
}

bool AVRoidAuthActor::ShouldUseMultiplayFlow() const
{
	if (VRoidNetMode == EVRoidNetMode::Multiplay)
	{
		return true;
	}
	return UVRoidSdkCoreFunctionLibrary::IsMultiplayMode(this);
}
