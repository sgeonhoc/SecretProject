//
// Created by Mameo
// Copyright © 2023 pixiv Inc. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "VRoidAnimInstanceBase.generated.h"

UCLASS()
class VROIDSDK_API UVRoidAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid|Skeleton")
	TObjectPtr<class UVrmMetaObject> VrmMetaObject;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
 	TObjectPtr<class UIKRetargeter> IKRetargeterAsset;

 	// If Neck is present on the IK Rig that is the Source of Retarget, Neck is applied to the IK Rig that is the Target, and if Neck is not present, Head is applied.
	UFUNCTION(BlueprintCallable, Category="VRoid|IKRetargeter")
	void UpdateNeckChain();
	UFUNCTION(BlueprintCallable, Category="VRoid|IKRetargeter", meta = (AdvancedDisplay="OverrideStartBoneName"))
	void UpdateChain(const FName ChainName, const FName EndBoneName, const FName IKGoal = NAME_None, const FName OverrideStartBoneName = NAME_None);
};
