// Copyright © 2024 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/VRoidDataTypes.h"
#include "VRoidMultiplayCharacter.generated.h"

UCLASS()
class VROIDSDK_API AVRoidMultiplayCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AVRoidMultiplayCharacter();

private:
	FTimerHandle ServerLicenseTimer;
	FTimerHandle OtherClientLicenseTimer;
	FTimerHandle UpdateOtherClientLicenseTimer;
	TSoftObjectPtr<class AVRoidPlayerStateBase> CacheVRoidPS = nullptr;

	UPROPERTY(Transient)
	bool IsClientInitialized = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid|Multiplay", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UVRoidMultiplayDownloadComponent> VRoidMultiplayDownloadComponent;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category="VRoid|Multiplay")
	FString DownloadLicenseId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FVrmSpawnParameter SpawnParameter;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="VRoid")
	TObjectPtr<class AVRoidMannequinRetargetAttachActor> AttachActor;

protected:
	bool UpdateVRMWithLicenseId(const FString& LicenseId, const bool IsOverrideVrm = true);
	APlayerController* FindLocalPlayerController() const;
	bool FindLicenseId(FString& OutDownloadLicenseId);
	bool UpdateServerVRM(const FString& LicenseId, const bool IsOverrideVrm = false);
	bool UpdateOtherClientsVRM(const FString& LicenseId, const bool IsOverrideVrm = false);
	void WaitLicenseAndCall(FTimerHandle& TimerHandle, const float Interval, TFunction<void(const FString&)> OnFoundLicense);
	bool TryUpdateMultiplayLicense() const;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Destroyed() override;

	//~ Begin APawn Interface.
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	//~ End APawn Interface

	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="VRoid|Multiplay")
	bool DownloadAndSaveMultiplayVRM(const FString& LicenseId, const bool IsOverrideVrm = true) const;
	UFUNCTION(BlueprintCallable, Category="VRoid|Multiplay")
	void StartWaitLicenseForServer(const float Interval = 0.2f);
	UFUNCTION(BlueprintCallable, Category="VRoid|Multiplay")
	void StartWaitLicenseForOtherClients(const float Interval = 0.2f);
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="VRoid|Multiplay")
	bool UpdateClientVRM() const;
	UFUNCTION(BlueprintCallable, Category="VRoid|Multiplay")
	void SyncUpdateVrm(const FString& LicenseId);

	UFUNCTION(Server, Reliable)
	void ServerSyncUpdateVrm(const FString& LicenseId);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSyncUpdateVrm(const FString& LicenseId);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName="OnDestroyActorWithAttachActor", Category="VRoid")
	void OnDestroyActorWithAttachActorBP();
	UFUNCTION(BlueprintImplementableEvent, DisplayName="OnRepController", Category="VRoid|Multiplay")
	void OnRepControllerBP();
	UFUNCTION(BlueprintImplementableEvent, DisplayName="OnRepPlayerState", Category="VRoid|Multiplay")
	void OnRepPlayerStateBP();
};
