//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "VRoidDataTypes.generated.h"

UENUM(BlueprintType, Category="VRoid")
enum class EVRMType : uint8
{
	VRM,
	Humanoid,
	Mannequin,
};

UENUM(BlueprintType, Category="VRoid")
enum class ECharacterContainerType : uint8
{
	Account,
	StaffPicks,
	Hearts,
};

UENUM(BlueprintType, Category="VRoid")
enum class EVRoidDialogType : uint8
{
	ClientWait,
	AllClientSelected,
	// ErrorCode
	EmptyId = 0x10,
	DeleteAuth,
	InvalidCode,
	FailedRefresh,
	CouldNotFindSession,
};

USTRUCT(BlueprintType, Category="VRoid")
struct FDownloadLicense
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="VRoid")
	FString UniqueId = TEXT("");
	UPROPERTY(BlueprintReadWrite, Category="VRoid")
	FString DownloadLicenseId = TEXT("");

	FDownloadLicense()
	{
	}
	FDownloadLicense(const FString& Unique, const FString& LicenseId)
		: UniqueId(Unique), DownloadLicenseId(LicenseId)
	{
	}
};

UENUM(BlueprintType, Category="VRoid")
enum class EVRoidMeshAttachType : uint8
{
	RetargetMeshComponent,
	OwnerActor,
	None,
};

USTRUCT(BlueprintType, Category="VRoid")
struct FVrmSpawnParameter
{
	GENERATED_BODY()

	// If not through the authentication flow, the model selection window will open.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool IsInitialOpenFile = true;
	// Default VRM display flag. (recommended to hide).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool IsInitialVisibleVrm = false;
	// Enable async loading.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool UseAsyncLoad = true;
	// Enable VRoidConverter (Enable Texture generation and asynchronous processing in RHITexture).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool UseFastConverter = true;
	// Generate toon-enhancing actors (recommended to enable).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool UseAttachMToon = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool UseFootVirtualBone = false;
	// Synchronize animations even when mannequins are hidden (recommended to enable).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool EnableHiddenAnimation = true;
	// Setting whether to Attach to SkeletalMeshComponent or Parent Actor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	EVRoidMeshAttachType AttachType = EVRoidMeshAttachType::RetargetMeshComponent;
	// Rules for attaching components.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	EAttachmentRule AttachmentRule = EAttachmentRule::SnapToTarget;

	FVrmSpawnParameter()
	{
	}
};
