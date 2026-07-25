//
// Created by Mameo
// Copyright © 2024 pixiv Inc. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VrmUtil.h"
#include "Core/VRoidDataTypes.h"
#include "VRoidIKRetargetAttachActor.generated.h"

UCLASS()
class VROIDSDK_API AVRoidIKRetargetAttachActor : public AActor
{
	GENERATED_BODY()

public:
	AVRoidIKRetargetAttachActor();

private:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="VRoid", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> RetargetVRoidMesh;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="VRoid", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UVrmDropFilesComponent> VrmDropFilesComponent;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="VRoid", meta = (ExposeOnSpawn = true))
	TObjectPtr<USkeletalMeshComponent> ParentRetargetSrcMesh = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid", meta = (ExposeOnSpawn = true))
	FVrmSpawnParameter SpawnParameter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FImportOptionData ImportOption;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="VRoid")
	TObjectPtr<UVrmAssetListObject> LoadVrmAssetList = nullptr;

	UFUNCTION()
	void OnParentDestroy(AActor* Parent);
	UFUNCTION(BlueprintPure, Category="VRoid")
	bool IsValidMannequinMesh() const;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName="LoadVRM", Category="VRoid")
	void LoadVRMBP(const FString& FilePath);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName="UpdateVRMWithMannequinMesh", Category="VRoid")
	void UpdateVRMWithMannequinMeshBP(const UVrmAssetListObject* VrmAssetListObject);
};
