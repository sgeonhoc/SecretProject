// Copyright © 2023 pixiv Inc. All rights reserved.

#include "VRoidMultiplayDownloadComponent.h"
#include "Core/VRoidLogger.h"
#include "Core/VRoidSdkCoreFunctionLibrary.h"
#include "Subsystems/VRoidGameInstanceSubsystem.h"

UVRoidMultiplayDownloadComponent::UVRoidMultiplayDownloadComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UVRoidMultiplayDownloadComponent::BeginPlay()
{
	Super::BeginPlay();

	if (sdk == nullptr)
	{
		sdk = MakeUnique<vroid::VRoidSdk>();
	}
	if (InitialLoadAccount)
	{
		LoadAccount();
	}
}

void UVRoidMultiplayDownloadComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FString UVRoidMultiplayDownloadComponent::GetLastLoadVRMPath() const
{
	return LastLoadVrmPath;
}

void UVRoidMultiplayDownloadComponent::UpdateOrAddVrmPathMap(const FString& Id)
{
	if (LoadVrmPathMap.Contains(Id))
	{
		LoadVrmPathMap[Id] = LastLoadVrmPath;
	}
	else
	{
		LoadVrmPathMap.Add(Id, LastLoadVrmPath);
	}
}

void UVRoidMultiplayDownloadComponent::OnSuccessRefreshAccount()
{
	if (const auto VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this))
	{
		VRoidSubsystem->OnSuccessRefreshAccount.RemoveDynamic(this, &UVRoidMultiplayDownloadComponent::OnSuccessRefreshAccount);
	}
	if (sdk == nullptr)
	{
		VROID_WARNING(TEXT("VRoid SDK is uninitialized."));
		return;
	}
	if (const std::string AccessToken = UVRoidSdkCoreFunctionLibrary::LoadStoredAccessToken(this);
		AccessToken.empty() == false)
	{
		IsInitialized = sdk->InitializeApi(AccessToken);
	}
}

FString UVRoidMultiplayDownloadComponent::FindVrmPath(const FString& LicenseId) const
{
	if (const auto FindPath = LoadVrmPathMap.Find(LicenseId); FindPath != nullptr)
	{
		return *FindPath;
	}
	VROID_WARNING(TEXT("Could not find a Path matching the license Id (LicenseId:%s)."), *LicenseId);
	return TEXT("");
}

bool UVRoidMultiplayDownloadComponent::LoadAccount()
{
	if (sdk == nullptr)
	{
		sdk = MakeUnique<vroid::VRoidSdk>();
	}
	if (vroid::authorization::Oauth::Account Account;
		UVRoidSdkCoreFunctionLibrary::LoadAccountJson(this, Account) && sdk)
	{
		if (Account.access_token.empty() || Account.token_type.empty() ||
			Account.refresh_token.empty() || Account.scope.empty())
		{
			VROID_ERROR(TEXT("Failed to get account information."));
			return false;
		}
		if (UVRoidSdkCoreFunctionLibrary::IsAccessTokenExpired(Account))
		{
			const auto VRoidSubsystem = UVRoidSdkCoreFunctionLibrary::GetVRoidGameInstanceSubSystem(this);
			if (VRoidSubsystem == nullptr)
			{
				VROID_ERROR(TEXT("Failed to get VRoidGameInstanceSubSystem."));
				return false;
			}
			if (VRoidSubsystem->OnSuccessRefreshAccount.IsAlreadyBound(this, &UVRoidMultiplayDownloadComponent::OnSuccessRefreshAccount) == false)
			{
				VRoidSubsystem->OnSuccessRefreshAccount.AddDynamic(this, &UVRoidMultiplayDownloadComponent::OnSuccessRefreshAccount);
			}
			VRoidSubsystem->RequestAccountRefresh();
			VROID_WARNING(TEXT("Access token has expired. Account refresh has been requested and is pending."));
			return false;
		}
		if (sdk != nullptr)
		{
			IsInitialized = sdk->InitializeApi(Account.access_token);
		}
	}
	return IsInitialized;
}

bool UVRoidMultiplayDownloadComponent::DownloadAndSaveMultiplayVRM(const FString& download_license_id, const bool is_override)
{
	if (IsInitialized == false || sdk == nullptr)
	{
		VROID_WARNING(TEXT("VRoid SDK is uninitialized."));
		if (LoadAccount() == false)
		{
			return false;
		}
	}
	const FString VrmSavedRoot(FPaths::ProjectSavedDir() / "VRoid" / "vrm" / "Multiplay");
	if (FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*VrmSavedRoot) == false)
	{
		VROID_ERROR(TEXT("The specified directory does not exist. Also, a new one could not be created."));
		return false;
	}

	const FString FileName(download_license_id + TEXT(".enc.vrm"));
	const FString VrmPath(VrmSavedRoot / FileName);
	if (FPaths::FileExists(VrmPath))
	{
		// FIXME: MinimalVrmSize is currently hardcoded (1024). Should be moved to a config value or defined as a named constant for flexibility.
		if (constexpr int32 MinimalVrmSize = 1024;
			IFileManager::Get().FileSize(*VrmPath) > MinimalVrmSize)
		{
			VROID_LOG(TEXT("Downloaded VRM already exists (Id: %s)."), *download_license_id);
			LastLoadVrmPath = VrmPath;
			UpdateOrAddVrmPathMap(download_license_id);
			return true;
		}
	}

	if (sdk->DownloadVRM(FStringToString(download_license_id), true) == false)
	{
		VROID_ERROR(TEXT("Failed download vrm (Id: %s)."), *download_license_id);
		return false;
	}
	LastLoadVrmPath = VrmPath;
	UpdateOrAddVrmPathMap(download_license_id);
	if (is_override == false && FPaths::FileExists(LastLoadVrmPath))
	{
		return true;
	}
	sdk->SaveVRM(FStringToString(LastLoadVrmPath), true);
	return true;
}
