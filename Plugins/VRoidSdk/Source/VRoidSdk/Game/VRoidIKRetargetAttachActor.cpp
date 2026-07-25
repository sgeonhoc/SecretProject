//
// Created by Mameo
// Copyright © 2024 pixiv Inc. All rights reserved.
//

#include "VRoidIKRetargetAttachActor.h"

#include "VrmDropFiles.h"
#include "VrmMetaObject.h"
#include "VrmAssetListObject.h"

AVRoidIKRetargetAttachActor::AVRoidIKRetargetAttachActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RetargetVRoidMesh = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshRetargetSrc");
	VrmDropFilesComponent = CreateDefaultSubobject<UVrmDropFilesComponent>("VrmDropFilesComponent");

	if (RetargetVRoidMesh)
	{
		const static ConstructorHelpers::FObjectFinder<USkeletalMesh> VRoidBaseMesh(TEXT("/VRoidSdk/Characters/VRoidRetargetBaseCharacter/SK_VRoidRetargetBaseCharacter_ue4mannequin"));
		if (VRoidBaseMesh.Object)
		{
			RetargetVRoidMesh->SetSkeletalMesh(VRoidBaseMesh.Object);
		}
	}

	ImportOption.MaterialType = EVRMImportMaterialType::VRMIMT_MToon;
	ImportOption.bGenerateHumanoidRenamedMesh = true;
	ImportOption.bSkipMorphTarget = false;
	ImportOption.bForceOriginalMorphTargetName = true;
	ImportOption.bGenerateOutlineMaterial = false;
	ImportOption.bMergeMaterial = false;
#if 0
	ImportOption.bAPoseRetarget = true;
	ImportOption.bGenerateIKBone = true;
	ImportOption.bGenerateRigIK = true;
#endif
}

void AVRoidIKRetargetAttachActor::BeginPlay()
{
	Super::BeginPlay();

	switch (SpawnParameter.AttachType)
	{
	case EVRoidMeshAttachType::RetargetMeshComponent:
		if (ParentRetargetSrcMesh)
		{
			AttachToComponent(ParentRetargetSrcMesh, FAttachmentTransformRules(SpawnParameter.AttachmentRule, false));
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

	if (const auto Parent = GetAttachParentActor(); Parent)
	{
		Parent->OnDestroyed.AddDynamic(this, &AVRoidIKRetargetAttachActor::OnParentDestroy);
	}
}

void AVRoidIKRetargetAttachActor::OnParentDestroy(AActor* Parent)
{
	(void)Parent;
	Destroy();
}

bool AVRoidIKRetargetAttachActor::IsValidMannequinMesh() const
{
	if (const auto VrmAssetList = LoadVrmAssetList)
	{
		if (const auto MannequinMeta = VrmAssetList->VrmMannequinMetaObject)
		{
			if (MannequinMeta->SkeletalMesh)
			{
				return true;
			}
		}
	}
	return false;
}

void AVRoidIKRetargetAttachActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AVRoidIKRetargetAttachActor::Destroyed()
{
	Super::Destroyed();
}
