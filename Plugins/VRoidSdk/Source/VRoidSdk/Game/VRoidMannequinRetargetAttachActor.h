// Copyright © 2023 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VrmUtil.h"
#include "Core/VRoidDataTypes.h"
#include "VRoidMannequinRetargetAttachActor.generated.h"

UCLASS()
class VROIDSDK_API AVRoidMannequinRetargetAttachActor : public AActor
{
	GENERATED_BODY()

public:
	AVRoidMannequinRetargetAttachActor();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="VRoid", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> VRoidSkeletalMesh;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="VRoid", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UVrmDropFilesComponent> VrmDropFilesComponent;

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="VRoid", meta = (ExposeOnSpawn = true))
	TObjectPtr<USkeletalMeshComponent> RetargetMesh = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="VRoid", meta = (ExposeOnSpawn = true))
	FVrmSpawnParameter SpawnParameter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	FImportOptionData ImportOption;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="VRoid")
	TObjectPtr<UVrmAssetListObject> LoadVrmAssetList = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="VRoid")
	bool IsModelUpdateInProgress = false;

	UFUNCTION(BlueprintPure, Category="VRoid")
	bool IsValidLoadAsset() const;
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName="CreatePoseCopy", Category="VRoid")
	void CreatePoseCopyBP();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName="ReInitializePoseCopy", Category="VRoid")
	void ReInitializePoseCopyBP();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName="ReCreateMToonActor", Category="VRoid")
	void ReCreateMToonActorBP();

public:
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

	UFUNCTION()
	UVrmAssetListObject* GetLoadVrmAssetList() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName="GenerateRetargetPoseCopy", Category="VRoid")
	void GenerateRetargetPoseCopyBP(const UVrmAssetListObject* VrmAssetListObject);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, DisplayName="UpdateVRM", Category="VRoid")
	void UpdateVRMBP(const FString& FilePath);
};
