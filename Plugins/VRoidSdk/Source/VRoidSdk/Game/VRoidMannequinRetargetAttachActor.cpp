// Copyright © 2023 pixiv Inc. All rights reserved.

#include "VRoidMannequinRetargetAttachActor.h"

#include "VrmDropFiles.h"
#include "VrmMetaObject.h"
#include "VrmAssetListObject.h"

AVRoidMannequinRetargetAttachActor::AVRoidMannequinRetargetAttachActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	VRoidSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshRetargetSrc");
	VrmDropFilesComponent = CreateDefaultSubobject<UVrmDropFilesComponent>("VrmDropFilesComponent");

	if (VRoidSkeletalMesh)
	{
		const static ConstructorHelpers::FObjectFinder<USkeletalMesh> VRoidMesh(TEXT("/VRM4U/Util/BaseCharacter/Mesh/SK_VRoidSimple"));
		if (VRoidMesh.Object)
		{
			VRoidSkeletalMesh->SetSkeletalMesh(VRoidMesh.Object);
		}
	}

	ImportOption.bGenerateIKBone = true;
	ImportOption.bGenerateRigIK = true;
	ImportOption.bAPoseRetarget = true;
}

void AVRoidMannequinRetargetAttachActor::BeginPlay()
{
	Super::BeginPlay();

	VRoidSkeletalMesh->SetVisibility(SpawnParameter.IsInitialVisibleVrm);

	switch (SpawnParameter.AttachType)
	{
	case EVRoidMeshAttachType::RetargetMeshComponent:
		if (RetargetMesh)
		{
			AttachToComponent(RetargetMesh, FAttachmentTransformRules(SpawnParameter.AttachmentRule, false));
		}
		break;
	case EVRoidMeshAttachType::OwnerActor:
		if (Owner)
		{
			AttachToActor(Owner, FAttachmentTransformRules(SpawnParameter.AttachmentRule, false));
		}
		break;
	default:
		break;
	}
}

void AVRoidMannequinRetargetAttachActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AVRoidMannequinRetargetAttachActor::Destroyed()
{
	Super::Destroyed();
}

bool AVRoidMannequinRetargetAttachActor::IsValidLoadAsset() const
{
	if (LoadVrmAssetList == nullptr)
	{
		return false;
	}
	if (LoadVrmAssetList->VrmMetaObject == nullptr)
	{
		return false;
	}
	return LoadVrmAssetList->VrmMetaObject->SkeletalMesh != nullptr;
}

UVrmAssetListObject* AVRoidMannequinRetargetAttachActor::GetLoadVrmAssetList() const
{
	return LoadVrmAssetList;
}
