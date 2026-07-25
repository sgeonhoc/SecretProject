// Copyright © 2023 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Library/VRoidSdk.h"
#include "VRoidGameInstanceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSuccessRefreshAccount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFailureRefreshAccount);

UCLASS()
class VROIDSDKCORE_API UVRoidGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UVRoidGameInstanceSubsystem();

protected:
	bool IsLogin = false;
	bool UseMultiplayFlow = false;
	FString UserId = TEXT("");
	TUniquePtr<vroid::VRoidSdk> SDK = nullptr;
	FString ApplicationId = TEXT("");
	FString SecretKey = TEXT("");
	FString DownloadMultiplayLicenseId = TEXT("");

	UPROPERTY()
	TMap<FString, TObjectPtr<class UVrmAssetListObject>> VrmAssetListMap;

private:
	/** 
	 * PlayerController を基にプレイヤーの一意な識別子を生成します
	 * シングルプレイ時にはPixivUserIdとPlayerStateのUniqueIdを組み合わせて一意性を確保します。
	 * マルチプレイ時にはPixivUserIdとPlayerStateのPlayerIdを組み合わせて一意性を確保します。
	 * エディタ実行時には PIEInstanceId を組み合わせて構築されます。
	 * Generates a unique identifier for the player based on the PlayerController.
	 * In single-player mode, combines the PixivUserId and the PlayerState's UniqueId to ensure uniqueness.
	 * In multiplayer mode, combines the PixivUserId and the PlayerState's PlayerId to ensure uniqueness.
	 * During editor execution, the PIEInstanceId is also combined to construct the final identifier.
	 */
	bool GeneratePlayerUniqueId(const APlayerController* Controller, FString& OutUniqueId) const;

public:
	UPROPERTY(BlueprintAssignable, Category="VRoid")
	FSuccessRefreshAccount OnSuccessRefreshAccount;
	UPROPERTY(BlueprintAssignable, Category="VRoid")
	FFailureRefreshAccount OnFailureRefreshAccount;

	bool IsEmptyVrmAssetList() const { return VrmAssetListMap.IsEmpty(); }
	FString GetDownloadMultiplayLicenseId() const { return DownloadMultiplayLicenseId; }
	void SetDownloadMultiplayLicenseId(const FString& Id) { DownloadMultiplayLicenseId = Id; }
	bool GetUseMultiplayFlow() const { return UseMultiplayFlow; }
	void EnableUseMultiplayFlow() { UseMultiplayFlow = true; }
	/**
	 * VRoid Hubで作成したアプリケーションのApplicationIdとSecretKeyを設定し、SDKを初期化します。
	 * ゲーム起動時に一度だけ呼び出してください。
	 * Initializes the SDK using the ApplicationId and SecretKey of the application created on VRoid Hub.
	 * This should be called once during application startup.
	 */
	UFUNCTION(BlueprintCallable, Category="VRoid", meta = (AdvancedDisplay="IsRefreshAccount"))
	void Init(UPARAM(DisplayName = "ApplicationId")const FString Id, UPARAM(DisplayName = "SecretKey")const FString Key, const bool IsRefreshAccount = true);

	UFUNCTION(BlueprintCallable, Category="VRoid")
	FString GetApplicationId() const { return ApplicationId; };
	UFUNCTION(BlueprintCallable, Category="VRoid")
	FString GetSecretKey() const { return SecretKey; };
	UFUNCTION(BlueprintPure, Category="VRoid")
	bool IsEmptyId() const;
	UFUNCTION(BlueprintCallable, Category="VRoid")
	bool RequestAccountRefresh();
	void OnAccountRefreshResponse(const TSharedPtr<class IHttpRequest> Request, const TSharedPtr<class IHttpResponse> Response, const bool bSuccessful) const;

	UFUNCTION(BlueprintPure, Category="VRoid")
	bool GetIsLogin() const;
	UFUNCTION()
	void SetIsLogin(const bool InIsLogin, const FString& InUserId = TEXT(""));

	UFUNCTION(BlueprintPure, Category="VRoid")
	UVrmAssetListObject* FindVrmAssetList(const APlayerController* Controller) const;
	UFUNCTION(BlueprintCallable, Category="VRoid")
	bool UpdateVrmAssetList(const APlayerController* Controller, UVrmAssetListObject* const VrmAssetList);
};
