// Copyright © 2022 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/VRoidDataTypes.h"
#include "Core/VRoidCharacterModel.h"
#include "ThumbnailItem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FClickedThumbnailDelegate,
                                             const FVRMCharacterModelMinimal&, ModelMinimal,
                                             const ECharacterContainerType, ContainerType);

UCLASS(Blueprintable)
class VROIDSDK_API UThumbnailItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category="VRoid", meta = (ExposeOnSpawn = true))
	FVRMCharacterModelMinimal ModelData;
	UPROPERTY(Transient, BlueprintReadWrite, Category="VRoid", meta = (ExposeOnSpawn = true))
	bool IsVRoidStudioModel = false;
	UPROPERTY(Transient, BlueprintReadWrite, Category="VRoid", meta = (ExposeOnSpawn = true))
	ECharacterContainerType CharacterContainer = ECharacterContainerType::Account;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category="VRoid")
	FClickedThumbnailDelegate OnClickedThumbnail;
};
