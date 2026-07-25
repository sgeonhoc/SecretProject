//
// Created by Mameo
// Copyright © 2023 pixiv Inc. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "VRoidEditorSettings.generated.h"

UCLASS(Config=VRoidSDK, DefaultConfig)
class VROIDSDK_API UVRoidEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UVRoidEditorSettings(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(config, EditAnywhere, Category="VRoid|Settings")
	bool EnableInitializeMannequinMesh = true;
	UPROPERTY(config, EditAnywhere, Category="VRoid|Settings")
	bool EnableInitializeMannequinAnim = true;
	UPROPERTY(config, EditAnywhere, Category="VRoid|Settings")
	bool EnableInitializeGenderAnim = true;

	UPROPERTY(config, EditAnywhere, Category="VRoid|Settings")
	FString MannequinMeshPath;
	UPROPERTY(config, EditAnywhere, Category="VRoid|Settings")
	FString MannequinAnimPath;
	UPROPERTY(config, EditAnywhere, Category="VRoid|Settings")
	FString MannyAnimPath;
	UPROPERTY(config, EditAnywhere, Category="VRoid|Settings")
	FString QuinnAnimPath;
};
