// Copyright © 2024 pixiv Inc. All rights reserved.

#include "VRoidMultiplayCharacter.h"
#include "Net/UnrealNetwork.h"

#include "VRoidPlayerStateBase.h"
#include "Core/VRoidLogger.h"
#include "Core/VRoidSdkCoreFunctionLibrary.h"
#include "Components/VRoidMultiplayDownloadComponent.h"
#include "Game/VRoidMannequinRetargetAttachActor.h"
#include "Subsystems/VRoidGameInstanceSubsystem.h"

AVRoidMultiplayCharacter::AVRoidMultiplayCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = true;

	VRoidMultiplayDownloadComponent = CreateDefaultSubobject<UVRoidMultiplayDownloadComponent>(TEXT("VRoidMultiplayDownloadComponent"));

	SpawnParameter.IsInitialOpenFile = false;
}

void AVRoidMultiplayCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (VRoidMultiplayDownloadComponent)
	{
		VRoidMultiplayDownloadComponent->LoadAccount();
	}
}

void AVRoidMultiplayCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVRoidMultiplayCharacter, DownloadLicenseId);
}

void AVRoidMultiplayCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	if (IsClientInitialized == false)
	{
		IsClientInitialized = UpdateClientVRM();
	}
	OnRepControllerBP();
}

void AVRoidMultiplayCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (IsClientInitialized == false)
	{
		IsClientInitialized = UpdateClientVRM();
	}
	OnRepPlayerStateBP();
}

void AVRoidMultiplayCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AVRoidMultiplayCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AVRoidMultiplayCharacter::Destroyed()
{
	Super::Destroyed();
	OnDestroyActorWithAttachActorBP();
}

void AVRoidMultiplayCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (CacheVRoidPS == nullptr && NewController != nullptr && NewController->PlayerState != nullptr)
	{
		CacheVRoidPS = Cast<AVRoidPlayerStateBase>(NewController->PlayerState);
		if (NewController->IsLocalController())
		{
			(void)TryUpdateMultiplayLicense();
		}
	}
}

void AVRoidMultiplayCharacter::UnPossessed()
{
	Super::UnPossessed();
}

bool AVRoidMultiplayCharacter::UpdateVRMWithLicenseId(const FString& LicenseId, const bool IsOverrideVrm)
{
	if (DownloadAndSaveMultiplayVRM(LicenseId, IsOverrideVrm))
	{
		if (DownloadLicenseId != LicenseId)
		{
			DownloadLicenseId = LicenseId;
		}
		if (const FString& VrmPath(VRoidMultiplayDownloadComponent->FindVrmPath(LicenseId));
			VrmPath.IsEmpty() == false)
		{
			AttachActor->UpdateVRMBP(VrmPath);
			return true;
		}
		AttachActor->UpdateVRMBP(VRoidMultiplayDownloadComponent->GetLastLoadVRMPath());
		return true;
	}
	return false;
}

APlayerController* AVRoidMultiplayCharacter::FindLocalPlayerController() const
{
	if (IsPawnControlled())
	{
		if (const auto PC = Cast<APlayerController>(Controller))
		{
			return PC;
		}
		VROID_ERROR(TEXT("Failed to cast PlayerController."));
	}
	if (HasAuthority())
	{
		VROID_ERROR(TEXT("Failed to get Controller."));
		return nullptr;
	}
	const auto World = GetWorld();
	if (World == nullptr)
	{
		VROID_ERROR(TEXT("Failed to get World."));
		return nullptr;
	}
	if (const auto PC = World->GetFirstPlayerController())
	{
		return PC;
	}
	VROID_ERROR(TEXT("Failed to get PlayerController."));
	return nullptr;
}

bool AVRoidMultiplayCharacter::FindLicenseId(FString& OutDownloadLicenseId)
{
	if (AttachActor == nullptr)
	{
		return false;
	}
	if (CacheVRoidPS)
	{
		OutDownloadLicenseId = CacheVRoidPS->GetDownloadLicenseId();
		return true;
	}
	const auto PC = FindLocalPlayerController();
	if (PC == nullptr)
	{
		return false;
	}
	const auto PS = PC->PlayerState;
	if (PS == nullptr)
	{
		if (HasAuthority())
		{
			VROID_ERROR(TEXT("Failed to get PlayerState."));
		}
		else
		{
			VROID_WARNING(TEXT("PlayerState has not yet been synchronized with the Client."));
		}
		return false;
	}
	CacheVRoidPS = Cast<AVRoidPlayerStateBase>(PS);
	if (CacheVRoidPS == nullptr)
	{
		VROID_ERROR(TEXT("Failed to cast VRoidPlayerStateBase."));
		return false;
	}
	if (IsLocallyControlled())
	{
		OutDownloadLicenseId = CacheVRoidPS->GetDownloadLicenseId();
		return true;
	}
	if (TryUpdateMultiplayLicense())
	{
		if (const auto* VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this))
		{
			OutDownloadLicenseId = VRoidSubsystem->GetDownloadMultiplayLicenseId();
			return true;
		}
	}
	return false;
}

bool AVRoidMultiplayCharacter::UpdateServerVRM(const FString& LicenseId, const bool IsOverrideVrm)
{
	if (LicenseId.IsEmpty() && HasAuthority())
	{
		return true;
	}
	if (UpdateVRMWithLicenseId(LicenseId, IsOverrideVrm))
	{
		return true;
	}
	return false;
}

bool AVRoidMultiplayCharacter::UpdateClientVRM() const
{
	if (HasAuthority() || IsClientInitialized)
	{
		return false;
	}
	if (IsPlayerControlled() == false || IsLocallyControlled() == false)
	{
		return false;
	}
	const APlayerController* LocalPC = Cast<APlayerController>(Controller);
	if (LocalPC == nullptr || LocalPC->PlayerState == nullptr)
	{
		return false;
	}
	if (AttachActor == nullptr)
	{
		VROID_ERROR(TEXT("AttachActor is NULL."));
		return false;
	}
	if (const auto VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this))
	{
		if (VRoidSubsystem->IsEmptyVrmAssetList())
		{
			VROID_LOG(TEXT("No assets have been registered in the VrmAssetListMap."));
			return true;
		}
		if (const auto FindVrmAssetList = VRoidSubsystem->FindVrmAssetList(LocalPC);
			FindVrmAssetList)
		{
			AttachActor->GenerateRetargetPoseCopyBP(FindVrmAssetList);
			return true;
		}
	}
	return false;
}

bool AVRoidMultiplayCharacter::DownloadAndSaveMultiplayVRM(const FString& LicenseId, const bool IsOverrideVrm) const
{
	if (AttachActor == nullptr)
	{
		VROID_ERROR(TEXT("AttachActor is NULL."));
		return false;
	}
	if (VRoidMultiplayDownloadComponent == nullptr)
	{
		VROID_ERROR(TEXT("VRoidMultiplayDownloadComponent is NULL."));
		return false;
	}
	return VRoidMultiplayDownloadComponent->DownloadAndSaveMultiplayVRM(LicenseId, IsOverrideVrm);
}

void AVRoidMultiplayCharacter::StartWaitLicenseForServer(const float Interval)
{
	WaitLicenseAndCall(ServerLicenseTimer, Interval,
		[this](const FString& LicenseId)
		{
			UpdateServerVRM(LicenseId, false);
		}
	);
}

void AVRoidMultiplayCharacter::StartWaitLicenseForOtherClients(const float Interval)
{
	WaitLicenseAndCall(OtherClientLicenseTimer, Interval,
		[this, Interval](const FString& LicenseId)
		{
			const UWorld* World = GetWorld();
			if (World == nullptr)
			{
				return;
			}
			World->GetTimerManager().ClearTimer(UpdateOtherClientLicenseTimer);
			if (UpdateOtherClientsVRM(LicenseId, false) == false)
			{
				World->GetTimerManager().SetTimer(UpdateOtherClientLicenseTimer,
					FTimerDelegate::CreateWeakLambda(this, [this, Interval]
						{
							StartWaitLicenseForOtherClients(Interval);
						}
					), Interval, true
				);
			}
		}
	);
}

bool AVRoidMultiplayCharacter::UpdateOtherClientsVRM(const FString& LicenseId, const bool IsOverrideVrm)
{
	// Synchronize models from different Clients.
	if (HasAuthority() || IsPawnControlled() || DownloadLicenseId == LicenseId)
	{
		return false;
	}
	return UpdateVRMWithLicenseId(DownloadLicenseId, IsOverrideVrm);
}

void AVRoidMultiplayCharacter::WaitLicenseAndCall(FTimerHandle& TimerHandle, const float Interval, TFunction<void(const FString&)> OnFoundLicense)
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	World->GetTimerManager().ClearTimer(TimerHandle);
	TWeakObjectPtr<AVRoidMultiplayCharacter> WeakThis(this);

	const FTimerDelegate TimerDelegate = FTimerDelegate::CreateWeakLambda(this,
		[WeakThis, &TimerHandle, OnFoundLicense]() mutable
		{
			if (WeakThis.IsValid() == false)
			{
				return;
			}
			if (FString LicenseId;
				WeakThis->FindLicenseId(LicenseId))
			{
				OnFoundLicense(LicenseId);
				if (const UWorld* InnerWeakWorld = WeakThis->GetWorld())
				{
					InnerWeakWorld->GetTimerManager().ClearTimer(TimerHandle);
				}
			}
		}
	);
	World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, Interval, true);
}

bool AVRoidMultiplayCharacter::TryUpdateMultiplayLicense() const
{
	if (CacheVRoidPS == nullptr)
	{
		return false;
	}
	if (CacheVRoidPS->GetIsModelSelected())
	{
		return false;
	}
	if (CacheVRoidPS->GetDownloadLicenseId().IsEmpty() == false)
	{
		return false;
	}

	const auto* VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this);
	if (VRoidSubsystem == nullptr)
	{
		return false;
	}
	CacheVRoidPS->ServerUpdateUniqueLicense(VRoidSubsystem->GetDownloadMultiplayLicenseId());
	return true;
}

void AVRoidMultiplayCharacter::SyncUpdateVrm(const FString& LicenseId)
{
	if (HasAuthority())
	{
		MulticastSyncUpdateVrm(LicenseId);
	}
	else
	{
		ServerSyncUpdateVrm(LicenseId);
	}
}

void AVRoidMultiplayCharacter::ServerSyncUpdateVrm_Implementation(const FString& LicenseId)
{
	if (UpdateVRMWithLicenseId(LicenseId, false))
	{
		MulticastSyncUpdateVrm(LicenseId);
	}
}

void AVRoidMultiplayCharacter::MulticastSyncUpdateVrm_Implementation(const FString& LicenseId)
{
	const bool IsUpdate = UpdateVRMWithLicenseId(LicenseId, true);
	(void)IsUpdate;
}
