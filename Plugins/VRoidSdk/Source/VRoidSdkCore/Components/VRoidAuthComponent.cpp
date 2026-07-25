//
// Created by Mameo
// Copyright © 2024 pixiv Inc. All rights reserved.
//

#include "VRoidAuthComponent.h"
#include "Core/VRoidLogger.h"
#include "Core/VRoidSdkCoreFunctionLibrary.h"

UVRoidAuthComponent::UVRoidAuthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UVRoidAuthComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UVRoidAuthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVRoidAuthComponent::Init(const FString& Id, const FString& Key)
{
	if (sdk == nullptr)
	{
		sdk = MakeUnique<vroid::VRoidSdk>(FStringToString(Id), FStringToString(Key));
	}
}

bool UVRoidAuthComponent::InitializeSdk(const std::string& AccessToken, FString& UserId, FString& UserName, FString& IconUrl) const
{
	if (HasSdk() == false)
	{
		return false;
	}
	if (sdk->InitializeApi(AccessToken) == false)
	{
		VROID_ERROR(TEXT("sdk initialization failed, Please review your account information."));
		return false;
	}
	UserId = sdk->GetUserId().c_str();
	UserName = sdk->GetUserName().c_str();
	IconUrl = sdk->CreateUserIconEndpoint().c_str();
	return true;
}

bool UVRoidAuthComponent::HasSdk(const bool EnableErrorLog) const
{
	if (sdk == nullptr)
	{
		if (EnableErrorLog)
		{
			VROID_ERROR(TEXT("The sdk is uninitialized."));
		}
		return false;
	}
	return true;
}

bool UVRoidAuthComponent::DownloadVRM(const FString& ModelId) const
{
	if (HasSdk() == false)
	{
		return false;
	}
	if (sdk->DownloadVRM(FStringToString(ModelId), false) == false)
	{
		VROID_ERROR(TEXT("Failed download vrm (Id: %s)."), *ModelId);
		return false;
	}
	return true;
}

void UVRoidAuthComponent::SaveVRM(const FString& Path) const
{
	if (HasSdk() == false)
	{
		return;
	}
	sdk->SaveVRM(FStringToString(Path), false);
}

FString UVRoidAuthComponent::DownloadMultiplayLicenseId(const FString& ModelId) const
{
	if (HasSdk() == false)
	{
		return TEXT("");
	}
	return sdk->DownloadMultiplayLicenseId(FStringToString(ModelId)).c_str();
}

FString UVRoidAuthComponent::CharacterEndPoint(const ECharacterContainerType ContainerType) const
{
	if (HasSdk() == false)
	{
		return TEXT("");
	}
	switch (ContainerType)
	{
	case ECharacterContainerType::Account:
		return sdk->CreateCharacterEndpoint().c_str();
	case ECharacterContainerType::StaffPicks:
		return sdk->CreateStaffpicksEndpoint().c_str();
	case ECharacterContainerType::Hearts:
		return sdk->CreateHeartsEndpoint().c_str();
	}
	VROID_ERROR(TEXT("Invalid type is set in ContainerType."));
	return TEXT("");
}

FString UVRoidAuthComponent::CharacterPropertyEndPoint(const FString& ModelId) const
{
	if (HasSdk() == false)
	{
		return TEXT("");
	}
	return sdk->CreateCharacterPropertyEndpoint(FStringToString(ModelId)).c_str();
}

FString UVRoidAuthComponent::AuthRefreshUrl(const std::string& RefreshToken) const
{
	if (HasSdk() == false)
	{
		return TEXT("");
	}
	return sdk->CreateAuthRefreshUrl(RefreshToken).c_str();
}

FString UVRoidAuthComponent::AuthCode(const bool IsMultiplay) const
{
	if (HasSdk() == false)
	{
		return TEXT("");
	}
	return sdk->CreateAuthCode(IsMultiplay).c_str();
}

FString UVRoidAuthComponent::AuthUrl(const FString& AuthCode) const
{
	if (HasSdk() == false)
	{
		return TEXT("");
	}
	return sdk->CreateAuthUrl(FStringToString(AuthCode)).c_str();
}
