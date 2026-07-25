//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#pragma once

#include "GameFramework/Actor.h"
#include "VRoidCharacterModel.h"
#include "VRoidAuthActor.generated.h"

class IHttpRequest;
class IHttpResponse;

UENUM(BlueprintType)
enum class EVRoidNetMode : uint8
{
	Single,
	Multiplay
};

UCLASS()
class VROIDSDKCORE_API AVRoidAuthActor : public AActor
{
	GENERATED_BODY()

public:
	AVRoidAuthActor();

private:
	bool IsReAuthorization = false;
	bool InitializeSdk = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="VRoid", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UVRoidAuthComponent> VRoidAuthComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="VRoid", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UVRoidAccountComponent> VRoidAccountComponent;

	bool InitializeAuthorization(class UVRoidGameInstanceSubsystem* const VRoidSubsystem);
	void CreateSdkWithReAuthorization();
	static bool IsValidSharedHttp(const TSharedPtr<IHttpRequest>& Request, const TSharedPtr<IHttpResponse>& Response, const FString& FunctionName);
	static bool CheckResponseCode(const int32 ResponseCode, bool bSuccessful, const TFunction<void()>& FailedCallback = nullptr);
	bool OnReAuthAndRetryRequest(const TSharedPtr<IHttpRequest>& Request, const TSharedPtr<IHttpResponse>& Response,
								 void (AVRoidAuthActor::* RetryRequest)(const TSharedPtr<IHttpRequest>, const TSharedPtr<IHttpResponse>, bool));
	bool AuthRefreshRequest(const FString& Content);
	bool ProcessRequest(const FString& Verb, const FString& Url,
	                    void (AVRoidAuthActor::* Callback)(const TSharedPtr<IHttpRequest>, const TSharedPtr<IHttpResponse>, bool),
	                    const TMap<FString, FString>& Params = TMap<FString, FString>());
	void OnCharactersResponseReceived(const TSharedPtr<IHttpRequest> Request, const TSharedPtr<IHttpResponse> Response, const bool bSuccessful);
	void OnReceiveCharacterProperty(const TSharedPtr<IHttpRequest> Request, const TSharedPtr<IHttpResponse> Response, const bool bSuccessful);
	void OnAuthResponseReceived(const TSharedPtr<IHttpRequest> Request, const TSharedPtr<IHttpResponse> Response, const bool bSuccessful);
	void OnGetCharacters(const TArray<FVRMCharacterModelMinimal>& Characters ,const FString& Href, const FString& Url);
	void OnReceiveCharacterArray(const FString& Content, const FString& Url);
	void GetCharactersAsync(const ECharacterContainerType ContainerType, const int Count = 20, const FString& MaxId = "");
	void GetCharacterPropertyAsync(const FString& ModelId);

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	/**
	 * VRoid Hubで作成したアプリケーションのApplication Idを設定。
	 * Sets the Application Id for the application created with VRoid Hub.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString ApplicationId = TEXT("");
	/**
	 * VRoid Hubで作成したアプリケーションのSecret Keyを設定。
	 * Sets the Secret Key for the application created with VRoid Hub.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FString SecretKey = TEXT("");
	/**
	 * 認証レベル以外でHub連携を行うためのフラグ。
	 * デフォルトで認証用のUIが開かないようになります。
	 * 認証用のUIを開くにはVisibleAuthWidgetノードを使用してください。
	 * A flag to enable Hub integration in levels other than the authentication level.
	 * When enabled, the authentication UI will not open automatically by default.
	 * To display the authentication UI, use the VisibleAuthWidget node.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VRoid")
	bool IsRuntime = false;
	/**
	 * セッション接続前に指定するマルチプレイモード。
	 * セッション接続中はSingleを指定していてもPostInitializeComponentsでMultiplayに上書きされます。
	 * Specifies the multiplayer mode used before connecting to a session.
	 * If a session is active, it will be overridden to Multiplay in PostInitializeComponents, even when Single is specified.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VRoid")
	EVRoidNetMode VRoidNetMode = EVRoidNetMode::Single;
	/**
	 * モデル選択後に表示するPreview用のモデルの生成位置。
	 * Spawn location for the preview model displayed after model selection.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid", meta = (EditCondition = "!IsRuntime", EditConditionHides, ExposeOnSpawn = "true"))
	FVector PreviewActorSpawnLocation = FVector::ZeroVector;
	/**
	 * VRoidSDKから取得したBoundingBoxのデバッグ描画フラグ。
	 * Debug draw flag for the BoundingBox retrieved from VRoidSDK.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid|Debug")
	bool IsDrawDebugBoundingBox = false;

	UPROPERTY(BlueprintReadOnly, Category="VRoid")
	ECharacterContainerType DefaultContainerType;
	UPROPERTY(BlueprintReadWrite, Category="VRoid")
	TSoftObjectPtr<AActor> PreviewActor = nullptr;
	UPROPERTY(Transient, BlueprintReadOnly, Category="VRoid")
	TMap<ECharacterContainerType, FString> NextCharacterMap;
	UPROPERTY(BlueprintReadOnly, Category="VRoid")
	FVRMCharacterModelVersionBoundingBox ModelBoundingBox;

protected:
	UFUNCTION(BlueprintCallable, Category="VRoid")
	void LaunchAuthURL() const;
	UFUNCTION(BlueprintCallable, Category="VRoid")
	void AuthRequest(const FString& AuthCode);
	UFUNCTION(BlueprintCallable, Category="VRoid", meta = (AdvancedDisplay="count"))
	void GetStaffPicksCharactersAsync(const int Count = 20);
	UFUNCTION(BlueprintCallable, Category="VRoid", meta = (AdvancedDisplay="count"))
	void GetAllCharactersAsync(const int Count = 20);
	UFUNCTION(BlueprintCallable, Category="VRoid")
	void DownloadAndSaveVRM(const FString& ModelId, const FString& ModelName, const bool IsOverride);
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="VRoid")
	bool RegisterVrmAssetList(class UVrmAssetListObject* const VrmAssetListObject) const;
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="VRoid|Multiplay")
	FString DownloadMultiplayLicenseId(const FString& ModelId) const;

	UFUNCTION(BlueprintCallable, Category="VRoid|Multiplay", meta = (AdvancedDisplay="Count"))
	void OnLoadNextDataChunk(const ECharacterContainerType ContainerType, const int Count = 20);

	UFUNCTION(BlueprintPure, Category="VRoid")
	static bool IsVRoidStudioModel(const FVRMCharacterModelMinimal& Model);
	UFUNCTION(BlueprintPure, Category="VRoid")
	bool IsEmptyId() const;
	UFUNCTION(BlueprintPure, Category="VRoid|Multiplay")
	bool ShouldUseMultiplayFlow() const;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, DisplayName="OnFailedDownloadVrm", Category="VRoid")
	void OnFailedDownloadVrmBP(const FString& VrmPath);

	UFUNCTION(BlueprintImplementableEvent, DisplayName="MultiplayBeginPlay", Category="VRoid|Multiplay")
	void MultiplayBeginPlayBP();
	UFUNCTION(BlueprintImplementableEvent, DisplayName="OnGetCharacters", Category="VRoid")
	void OnGetCharactersBP(const ECharacterContainerType CharacterContainerType, const TArray<FVRMCharacterModelMinimal>& DataArray);
	UFUNCTION(BlueprintImplementableEvent, DisplayName="OnDownloadVrm", Category="VRoid")
	void OnDownloadVrmBP(const FString& VrmPath);
	UFUNCTION(BlueprintImplementableEvent, DisplayName="OnSucceedAuthorization", Category="VRoid")
	void OnSucceedAuthorizationBP(const	FString& UserName, const FString& UserIconUrl);
	UFUNCTION(BlueprintImplementableEvent, DisplayName="OnFailedAuthorization", Category="VRoid")
	void OnFailedAuthorizationBP(const bool IsRefresh);
};
