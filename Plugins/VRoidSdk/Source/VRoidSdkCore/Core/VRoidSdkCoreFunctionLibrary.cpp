// Copyright © 2023 pixiv Inc. All rights reserved.

#include "VRoidSdkCoreFunctionLibrary.h"

#include "VRoidLogger.h"
#include "Subsystems/VRoidGameInstanceSubsystem.h"

UVRoidGameInstanceSubsystem* UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(const UObject* const WorldContextObject)
{
	if (const auto World = WorldContextObject->GetWorld())
	{
		if (const auto GameInstance = World->GetGameInstance())
		{
			if (const auto Subsystem = GameInstance->GetSubsystem<UVRoidGameInstanceSubsystem>())
			{
				return Subsystem;
			}
		}
	}
	return nullptr;
}

bool UVRoidSdkCoreFunctionLibrary::IsMultiplayMode(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		VROID_ERROR(TEXT("WorldContextObject is NULL."));
		return false;
	}
	const auto World = WorldContextObject->GetWorld();
	if (World == nullptr)
	{
		VROID_ERROR(TEXT("Failed to get World"));
		return false;
	}
	const ENetMode NetMode(World->GetNetMode());
	if (NetMode == NM_MAX)
	{
		VROID_ERROR(TEXT("Unexpected error: GetNetMode returned NM_MAX."));
		return false;
	}
	// If NetMode is NM_Standalone, it is considered as single-player.
	return (NetMode != NM_Standalone);
}

FString UVRoidSdkCoreFunctionLibrary::EnsureAccessTokenPath(const UObject* WorldContextObject)
{
	// Use a fixed file for single-player mode.
	bool ShouldUseMultiplayerFlow = IsMultiplayMode(WorldContextObject);
	if (ShouldUseMultiplayerFlow == false)
	{
		if (const auto* VRoidSubsystem = GetVRoidGameInstanceSubSystem(WorldContextObject))
		{
			ShouldUseMultiplayerFlow = VRoidSubsystem->GetUseMultiplayFlow();
		}
	}
	const FString FileName(TEXT("access_token.json"));
	const FString SavePath(FPaths::ProjectSavedDir() / "VRoid");
	if (ShouldUseMultiplayerFlow == false)
	{
		return SavePath / FileName;
	}
	// Use command line argument (-AccessTokenSlot=SlotNum).
	int32 Slot = INDEX_NONE;
	if (FString SlotStr;
		FParse::Value(FCommandLine::Get(), TEXT("-AccessTokenSlot="), SlotStr))
	{
		Slot = FCString::Atoi(*SlotStr);
	}
	// Use the PIE instance id (Editor PIE execution).
#if WITH_EDITOR
	if (const UWorld* World = WorldContextObject->GetWorld();
		Slot == INDEX_NONE && World && GEngine && GEngine->GetWorldContexts().Num() > 0)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() != World || Context.WorldType != EWorldType::PIE)
			{
				continue;
			}
			Slot = Context.PIEInstance;
			break;
		}
	}
#endif // WITH_EDITOR
	// (fallback) Use the OS process id.
	if (Slot == INDEX_NONE)
	{
		Slot = FPlatformProcess::GetCurrentProcessId();
	}
	const FString BaseName = FPaths::GetBaseFilename(FileName);
	const FString Extension = FPaths::GetExtension(FileName, true);
	const FString NewFileName = FString::Printf(TEXT("%s_%d%s"), *BaseName, Slot, *Extension);

	return SavePath / NewFileName;
}

bool UVRoidSdkCoreFunctionLibrary::IsAccessTokenExpired(const vroid::authorization::Oauth::Account& Account)
{
	const int64 UnixTime = FDateTime::UtcNow().ToUnixTimestamp();
	return(Account.created_at + Account.expires_in) < UnixTime;
}

bool UVRoidSdkCoreFunctionLibrary::LoadAccountJson(const UObject* WorldContextObject, vroid::authorization::Oauth::Account& Account)
{
	const FString AccountFileFullPath = EnsureAccessTokenPath(WorldContextObject);
	if (FPaths::ValidatePath(AccountFileFullPath) == false || FPaths::FileExists(AccountFileFullPath) == false)
	{
		VROID_WARNING(TEXT("An access token has not yet been created."));
		return false;
	}
	FString RawData;
	if (FFileHelper::LoadFileToString(RawData, *AccountFileFullPath) == false)
	{
		return false;
	}
	TSharedPtr<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
	if (const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(RawData);
		FJsonSerializer::Deserialize(JsonReader, JsonRootObject) == false)
	{
		return false;
	}
	Account.access_token = FStringToString(JsonRootObject->GetStringField(TEXT("access_token")));
	Account.token_type = FStringToString(JsonRootObject->GetStringField(TEXT("token_type")));
	Account.expires_in = JsonRootObject->GetNumberField(TEXT("expires_in"));
	Account.refresh_token = FStringToString(JsonRootObject->GetStringField(TEXT("refresh_token")));
	Account.scope = FStringToString(JsonRootObject->GetStringField(TEXT("scope")));
	Account.created_at = JsonRootObject->GetNumberField(TEXT("created_at"));
	return true;
}

std::string UVRoidSdkCoreFunctionLibrary::LoadStoredAccessToken(const UObject* WorldContextObject)
{
	const FString AccountFileFullPath = EnsureAccessTokenPath(WorldContextObject);
	if (FPaths::ValidatePath(AccountFileFullPath) == false || FPaths::FileExists(AccountFileFullPath) == false)
	{
		VROID_WARNING(TEXT("Stored access token does not exist yet."));
		return "";
	}
	FString RawData;
	if (FFileHelper::LoadFileToString(RawData, *AccountFileFullPath) == false)
	{
		VROID_ERROR(TEXT("Failed to load access token: %s"), *AccountFileFullPath);
		return "";
	}
	TSharedPtr<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
	if (const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(RawData);
		FJsonSerializer::Deserialize(JsonReader, JsonRootObject) == false)
	{
		VROID_ERROR(TEXT("Failed to parse access token: %s."), *AccountFileFullPath);
		return "";
	}
	const FString AccessToken = JsonRootObject->GetStringField(TEXT("access_token"));
	if (AccessToken.IsEmpty())
	{
		VROID_ERROR(TEXT("Stored access token is empty: %s"), *AccountFileFullPath);
		return "";
	}
	return FStringToString(AccessToken);
}

std::string UVRoidSdkCoreFunctionLibrary::FStringToStdString(const FString& Str)
{
#if 1
	return StringCast<ANSICHAR>(StringCast<UTF8CHAR>(*Str).Get()).Get();
#else
	return reinterpret_cast<const ANSICHAR*>(StringCast<UTF8CHAR>(*Str).Get());
#endif
}
