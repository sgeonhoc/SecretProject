//
// Created by Mameo
// Copyright © 2023 pixiv Inc. All rights reserved.
//

#include "VRoidAnimInstanceBase.h"

#include "Retargeter/IKRetargeter.h"
#include "Misc/EngineVersionComparison.h"

#include "Core/VRoidDefinitions.h"
#include "Core/VRoidLogger.h"

void UVRoidAnimInstanceBase::UpdateNeckChain()
{
	if (IKRetargeterAsset == nullptr)
	{
		VROID_ERROR(TEXT("IK Retargeter asset is NULL."));
		return;
	}
	const FName ChainNameNeck("Neck");
	const auto RetargetChainSettings = IKRetargeterAsset->GetChainMapByName(ChainNameNeck);
	if (RetargetChainSettings == nullptr)
	{
		return;
	}
#if UE_OLDER_5_4
	const UIKRigDefinition* SourceIKRig = IKRetargeterAsset->GetSourceIKRig();
#else
	const UIKRigDefinition* SourceIKRig = IKRetargeterAsset->GetIKRig(ERetargetSourceOrTarget::Source);
#endif // UE_OLDER_5_4
	if (SourceIKRig == nullptr)
	{
		VROID_ERROR(TEXT("Source IK Rig not found in IK Retargeter."));
		return;
	}
	const TArray<FBoneChain>& RetargetChains = SourceIKRig->GetRetargetChains();
	const bool HasNeckChain = RetargetChains.ContainsByPredicate([&](const FBoneChain& Chain)
	{
		return (Chain.ChainName == ChainNameNeck);
	});
	const FName ChainNameHead("Head");
	const FName OverrideStartBoneName = HasNeckChain ? TEXT("head") : TEXT("neck_01");
	RetargetChainSettings->SourceChain = HasNeckChain ? ChainNameNeck : ChainNameHead;
	UpdateChain(ChainNameHead, TEXT("head"), NAME_None, OverrideStartBoneName);
}

void UVRoidAnimInstanceBase::UpdateChain(const FName ChainName, const FName EndBoneName, const FName IKGoal, const FName OverrideStartBoneName)
{
	if (IKRetargeterAsset == nullptr)
	{
		VROID_ERROR(TEXT("IK Retargeter asset is NULL."));
		return;
	}
#if UE_OLDER_5_4
	const auto TargetIKRig = IKRetargeterAsset->GetTargetIKRig();
#else
	const auto TargetIKRig = IKRetargeterAsset->GetIKRig(ERetargetSourceOrTarget::Target);
#endif // UE_OLDER_5_4
	if (TargetIKRig == nullptr)
	{
		VROID_ERROR(TEXT("Target IK Rig not found in IK Retargeter."));
		return;
	}
	FBoneChain* Chain = const_cast<FBoneChain*>(TargetIKRig->GetRetargetChainByName(ChainName));
	// const FIKRigSkeleton& IKRigSkeleton = TargetIKRig->GetSkeleton();
	// const int32 EndBoneIndex = IKRigSkeleton.GetBoneIndexFromName(EndBoneName);

	Chain->ChainName = ChainName;
	// Chain->StartBone = IKRigSkeleton.GetBoneNameFromIndex(Chain->StartBone.BoneIndex);
	if (OverrideStartBoneName != NAME_None)
	{
		Chain->StartBone = OverrideStartBoneName;
	}
	// Chain->EndBone = IKRigSkeleton.GetBoneNameFromIndex(EndBoneIndex);
	Chain->EndBone = EndBoneName;
	Chain->IKGoalName = IKGoal;
}
