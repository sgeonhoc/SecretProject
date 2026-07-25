//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#include "Core/VRoidLogger.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "VRoidSdkCore"

class FVRoidSdkCoreModule final : public IModuleInterface
{
	void* ModuleHandle = nullptr;

	virtual void StartupModule() override
	{
		FString libraryPath;

#if PLATFORM_WINDOWS
		VROID_LOG(TEXT("Starts loading the VRoid SDK."));
		const FString PlatformPath = TEXT("Binaries/Win64/");
		const FString DllName = "VRoidSdk.dll";
		const FString DllPath = PlatformPath + DllName;
#if WITH_EDITOR
		const FString BaseDir = IPluginManager::Get().FindPlugin("VRoidSdk")->GetBaseDir();
		libraryPath = FPaths::Combine(*BaseDir, DllPath);
#if 0
		if (const FString LibraryDirectoryPath = FPaths::Combine(*BaseDir, TEXT("lib"));
			FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*LibraryDirectoryPath))
		{
			FPlatformProcess::AddDllDirectory(*LibraryDirectoryPath);
		}
		else
		{
#endif
			ForceCopyDll(DllName);
			ForceCopyDll("libcrypto-3-x64.dll");
			ForceCopyDll("libcurl.dll");
			ForceCopyDll("zlib1.dll");
		// }
#else
		libraryPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), DllPath));
#endif // WITH_EDITOR
#endif // PLATFORM_WINDOWS

		if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*libraryPath))
		{
			VROID_WARNING(TEXT("Failed to find module."));
			return;
		}

		ModuleHandle = (!libraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*libraryPath) : nullptr);
		if (ModuleHandle == nullptr)
		{
			VROID_FATAL(TEXT("Failed to load module."));
		}
	}

	virtual void ShutdownModule() override
	{
		if (ModuleHandle == nullptr)
		{
			return;
		}
		FPlatformProcess::FreeDllHandle(ModuleHandle);
		ModuleHandle = nullptr;
	}

#if PLATFORM_WINDOWS
	// Forces copy of DLLs for Blueprint-only projects.
	void ForceCopyDll(const FString& DllName) const
	{
		const FString PlatformPath = TEXT("Binaries/Win64/");
		const FString DllPath = PlatformPath + DllName;

		const FString BaseDir = IPluginManager::Get().FindPlugin("VRoidSdk")->GetBaseDir();
		const FString LibraryPath = FPaths::Combine(*BaseDir, DllPath);
			
		const FString BinaryPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), DllPath));
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		PlatformFile.CopyFile(*BinaryPath, *LibraryPath);
	}
#endif // PLATFORM_WINDOWS
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVRoidSdkCoreModule, VRoidSdkCore);
