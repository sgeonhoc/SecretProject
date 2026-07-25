// Copyright © 2023 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Library/vroidsdk/authorization/oauth.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VRoidSdkCoreFunctionLibrary.generated.h"

UCLASS()
class VROIDSDKCORE_API UVRoidSdkCoreFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="VRoid", meta = (WorldContext = "WorldContextObject"))
	static class UVRoidGameInstanceSubsystem* GetVRoidGameInstanceSubSystem(const UObject* const WorldContextObject);
	UFUNCTION(BlueprintPure, Category="VRoid|Multiplay", meta=(WorldContext = "WorldContextObject"))
	static bool IsMultiplayMode(const UObject* WorldContextObject);

	static FString EnsureAccessTokenPath(const UObject* WorldContextObject);
	static bool IsAccessTokenExpired(const vroid::authorization::Oauth::Account& Account);
	static bool LoadAccountJson(const UObject* WorldContextObject, vroid::authorization::Oauth::Account& Account);
	static std::string LoadStoredAccessToken(const UObject* WorldContextObject);
	static std::string FStringToStdString(const FString& Str);
};

inline std::string (*FStringToString)(const FString&) = UVRoidSdkCoreFunctionLibrary::FStringToStdString;
