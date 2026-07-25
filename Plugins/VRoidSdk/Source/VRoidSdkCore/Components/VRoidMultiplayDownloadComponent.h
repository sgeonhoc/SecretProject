// Copyright © 2023 pixiv Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Library/VRoidSdk.h"
#include "VRoidMultiplayDownloadComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VROIDSDKCORE_API UVRoidMultiplayDownloadComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVRoidMultiplayDownloadComponent();

private:
	TUniquePtr<vroid::VRoidSdk> sdk = nullptr;

	void UpdateOrAddVrmPathMap(const FString& Id);
	UFUNCTION()
	void OnSuccessRefreshAccount();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VRoid")
	bool InitialLoadAccount = false;
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="VRoid")
	bool IsInitialized = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="VRoid")
	FString LastLoadVrmPath = TEXT("");
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="VRoid")
	TMap<FString, FString> LoadVrmPathMap; //!< @brief LicenseId and Path of the download model.

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	FString GetLastLoadVRMPath() const;
	UFUNCTION()
	FString FindVrmPath(const FString& LicenseId) const;
	UFUNCTION(BlueprintCallable, Category="VRoid|Multiplay")
	bool LoadAccount();
	UFUNCTION(BlueprintCallable, Category="VRoid|Multiplay")
	bool DownloadAndSaveMultiplayVRM(UPARAM(ref) const FString& download_license_id, const bool is_override);
};
