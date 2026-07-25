// Copyright © 2023 pixiv Inc. All rights reserved.

#include "VRoidGameInstanceSubsystem.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "GameFramework/PlayerState.h"

#include "VrmAssetListObject.h"
#include "Core/VRoidLogger.h"
#include "Core/VRoidSdkCoreFunctionLibrary.h"

UVRoidGameInstanceSubsystem::UVRoidGameInstanceSubsystem()
{
}

bool UVRoidGameInstanceSubsystem::GeneratePlayerUniqueId(const APlayerController* Controller, FString& OutUniqueId) const
{
	if (Controller == nullptr || Controller->GetWorld() == nullptr)
	{
		VROID_ERROR(TEXT("PlayerController has not valid."));
		return false;
	}
	const auto PS = Controller->PlayerState;
	if (PS == nullptr)
	{
		VROID_ERROR(TEXT("Failed to get PlayerState."));
		return false;
	}
	OutUniqueId = UserId;
	if (UserId.IsEmpty())
	{
		VROID_ERROR(TEXT("UserId is empty."));
		return false;
	}
	const UWorld* World = Controller->GetWorld();
	bool ShouldUseMultiplayerFlow = UVRoidSdkCoreFunctionLibrary::IsMultiplayMode(World);
	if (ShouldUseMultiplayerFlow == false)
	{
		if (const auto* VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(World))
		{
			ShouldUseMultiplayerFlow = VRoidSubsystem->GetUseMultiplayFlow();
		}
	}
	if (ShouldUseMultiplayerFlow)
	{
		// NOTE: Changed to avoid using GetUniqueId() since it can change before and after login when using systems like EOS.
		OutUniqueId.Appendf(TEXT("_%d"), PS->GetPlayerId());
	}
	else
	{
		// For single-player, the UniqueId is used as it has been traditionally.
		OutUniqueId.Appendf(TEXT("_%s"), *PS->GetUniqueId().ToString());
	}
#if WITH_EDITOR
	if (GEngine && GEngine->GetWorldContexts().Num() > 0)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() != World || Context.WorldType != EWorldType::PIE)
			{
				continue;
			}
			OutUniqueId.Appendf(TEXT("_%d"), Context.PIEInstance);
			return true;
		}
	}
#endif // WITH_EDITOR
	return true;
}

void UVRoidGameInstanceSubsystem::Init(const FString Id, const FString Key, const bool IsRefreshAccount)
{
	if (Id.IsEmpty() || Key.IsEmpty())
	{
		VROID_ERROR(TEXT("Failed to initialize VRoid GameInstanceSubsystem: Id or Key is empty."));
		return;
	}
	ApplicationId = Id;
	SecretKey = Key;
	if (SDK == nullptr)
	{
		SDK = MakeUnique<vroid::VRoidSdk>(FStringToString(ApplicationId), FStringToString(SecretKey));
	}
	if (IsRefreshAccount)
	{
		if (const FString AccountFileFullPath = UVRoidSdkCoreFunctionLibrary::EnsureAccessTokenPath(this);
			FPaths::ValidatePath(AccountFileFullPath) == false || FPaths::FileExists(AccountFileFullPath) == false)
		{
			VROID_WARNING(TEXT("Account refresh skipped: Account file does not exist. Account information may not be generated yet."));
			return;
		}
		RequestAccountRefresh();
		return;
	}
	if (vroid::authorization::Oauth::Account Account;
		UVRoidSdkCoreFunctionLibrary::LoadAccountJson(this, Account))
	{
		if (UVRoidSdkCoreFunctionLibrary::IsAccessTokenExpired(Account) == false)
		{
			(void)SDK->InitializeApi(Account.access_token);
		}
	}
}

bool UVRoidGameInstanceSubsystem::IsEmptyId() const
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

bool UVRoidGameInstanceSubsystem::RequestAccountRefresh()
{
	if (SDK == nullptr)
	{
		VROID_ERROR(TEXT("The sdk is uninitialized."));
		return false;
	}
	vroid::authorization::Oauth::Account Account;
	if (UVRoidSdkCoreFunctionLibrary::LoadAccountJson(this, Account) == false)
	{
		VROID_ERROR(TEXT("Failed load account."));
		return false;
	}
	if (UVRoidSdkCoreFunctionLibrary::IsAccessTokenExpired(Account) == false)
	{
		(void)SDK->InitializeApi(Account.access_token);
	}
	const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	// HttpRequest->SetHeader(vroid::api::AUTH_HEADER, FString(vroid::api::AUTH_SCHEME_PREFIX) + Account.access_token.c_str());
	HttpRequest->SetHeader(vroid::api::VERSION_HEADER, vroid::api::VERSION);
	HttpRequest->SetURL(SDK->CreateAuthRefreshUrl(Account.refresh_token).c_str());
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UVRoidGameInstanceSubsystem::OnAccountRefreshResponse);
	return HttpRequest->ProcessRequest();
}

void UVRoidGameInstanceSubsystem::OnAccountRefreshResponse(const FHttpRequestPtr Request, const FHttpResponsePtr Response, const bool bSuccessful) const
{
	bool IsSuccessRefresh = false;
	ON_SCOPE_EXIT
	{
		if (IsSuccessRefresh == false && OnFailureRefreshAccount.IsBound())
		{
			OnFailureRefreshAccount.Broadcast();
		}
	};
	// Validate request/response and handle HTTP errors.
	if (Request.IsValid() == false)
	{
		VROID_ERROR(TEXT("Invalid smart pointer in Request (FunctionName:RequestAccountRefresh)."));
		return;
	}
	if (Response.IsValid() == false)
	{
		VROID_ERROR(TEXT("Invalid smart pointer in Response (FunctionName:RequestAccountRefresh)."));
		return;
	}
	if (const int32 ResponseCode = Response->GetResponseCode();
		bSuccessful == false || ResponseCode != EHttpResponseCodes::Ok)
	{
		VROID_ERROR(TEXT("Account refresh request failed (bSuccessful=%s, ResponseCode=%d)."), bSuccessful ? TEXT("true") : TEXT("false"), ResponseCode);
		return;
	}
	// Parse response and save access token data.
	const FString Content = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	if (const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
		FJsonSerializer::Deserialize(Reader, JsonObject) == false || JsonObject.IsValid() == false)
	{
		VROID_ERROR(TEXT("Failed to parse JSON response."));
		return;
	}
	FString OutputString;
	if (const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer) == false)
	{
		VROID_ERROR(TEXT("Failed to serialize JSON object."));
		return;
	}
	if (const FString AccessTokenPath = UVRoidSdkCoreFunctionLibrary::EnsureAccessTokenPath(this);
		FPaths::ValidatePath(AccessTokenPath) == false ||
		FPaths::FileExists(AccessTokenPath) == false ||
		FFileHelper::SaveStringToFile(OutputString, *AccessTokenPath) == false)
	{
		VROID_ERROR(TEXT("Failed to save retrieved account information."));
		return;
	}
	// Load account data and initialize SDK.
	const std::string AccessToken = UVRoidSdkCoreFunctionLibrary::LoadStoredAccessToken(this);
	if (AccessToken.empty())
	{
		VROID_ERROR(TEXT("Failed to load account information."));
		return;
	}
	IsSuccessRefresh = SDK->InitializeApi(AccessToken);
	if (IsSuccessRefresh && OnSuccessRefreshAccount.IsBound())
	{
		OnSuccessRefreshAccount.Broadcast();
	}
}

bool UVRoidGameInstanceSubsystem::GetIsLogin() const
{
	return IsLogin;
}

void UVRoidGameInstanceSubsystem::SetIsLogin(const bool InIsLogin, const FString& InUserId)
{
	IsLogin = InIsLogin;
	UserId = IsLogin ? InUserId : TEXT("");
}

UVrmAssetListObject* UVRoidGameInstanceSubsystem::FindVrmAssetList(const APlayerController* Controller) const
{
	if (VrmAssetListMap.Num() <= 0)
	{
		VROID_WARNING(TEXT("No assets have been registered in the VrmAssetListMap yet."));
		return nullptr;
	}
	if (FString UniqueId; GeneratePlayerUniqueId(Controller, UniqueId) && UniqueId.IsEmpty() == false)
	{
		if (const auto Asset = VrmAssetListMap.Find(UniqueId); Asset != nullptr)
		{
			return Asset->Get();
		}
	}
	VROID_ERROR(TEXT("Could not find the VrmAssetList."));
	return nullptr;
}

bool UVRoidGameInstanceSubsystem::UpdateVrmAssetList(const APlayerController* Controller, UVrmAssetListObject* const VrmAssetList)
{
	if (FString UniqueId; GeneratePlayerUniqueId(Controller, UniqueId) && UniqueId.IsEmpty() == false)
	{
		if (VrmAssetListMap.Contains(UniqueId))
		{
			VrmAssetListMap[UniqueId] = VrmAssetList;
		}
		VrmAssetListMap.Add(UniqueId, VrmAssetList);
		return true;
	}
	VROID_ERROR(TEXT("Failed to update the VrmAssetList."));
	return false;
}
