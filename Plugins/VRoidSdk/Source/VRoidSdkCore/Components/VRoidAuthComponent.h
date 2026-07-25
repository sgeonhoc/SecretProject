//
// Created by Mameo
// Copyright © 2024 pixiv Inc. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Library/VRoidSdk.h"
#include "Core/VRoidDataTypes.h"
#include "VRoidAuthComponent.generated.h"

UCLASS()
class VROIDSDKCORE_API UVRoidAuthComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	TUniquePtr<vroid::VRoidSdk> sdk = nullptr;

public:
	UVRoidAuthComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Init(const FString& Id, const FString& Key);
	bool InitializeSdk(const std::string& AccessToken, FString& UserId, FString& UserName, FString& IconUrl) const;
	bool HasSdk(const bool EnableErrorLog = true) const;

	bool DownloadVRM(const FString& ModelId) const;
	void SaveVRM(const FString& Path) const;
	FString DownloadMultiplayLicenseId(const FString& ModelId) const;

	FString CharacterEndPoint(const ECharacterContainerType ContainerType) const;
	FString CharacterPropertyEndPoint(const FString& ModelId) const;
	FString AuthRefreshUrl(const std::string& RefreshToken) const;
	FString AuthCode(const bool IsMultiplay) const;
	FString AuthUrl(const FString& AuthCode) const;
};
