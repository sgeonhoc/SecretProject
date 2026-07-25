//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

#include "VRoidSdk.h"
#if WITH_EDITOR
#include "ISettingsModule.h"
#endif // WITH_EDITOR
#include "Editor/VRoidEditorSettings.h"

TAutoConsoleVariable<bool> CVarVRoidRuntimeMeshLoadVerboseLog(
	TEXT("vroid.MeshLoad.VerboseLog"),
	false,
	TEXT("Enable verbose logging for VRoid Runtime mesh loading, including detailed information on SkeletalMesh generation and material assignment.")
);

TAutoConsoleVariable<bool> CVarVRoidRuntimeLoadVerboseLog(
	TEXT("vroid.RuntimeLoad.VerboseLog"),
	false,
	TEXT("Enable verbose logging for VRoid Runtime loading, including detailed information on the overall runtime process.")
);

void FVRoidSdkModule::StartupModule()
{
#if WITH_EDITOR
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule == nullptr)
	{
		return;
	}
#define LOCTEXT_NAMESPACE "FVRoidSdkModule"
	SettingsModule->RegisterSettings(
		"Project", "Plugins", "VRoidSdk",
		LOCTEXT("RuntimeSettingsName", "VRoidSdk"),
		LOCTEXT("RuntimeSettingsDescription", "Configure the VRoidSdk plugin"),
		GetMutableDefault<UVRoidEditorSettings>()
	);
#undef LOCTEXT_NAMESPACE
#endif // WITH_EDITOR

	const TCHAR* Section = TEXT("/Script/VRoidSDK.VRoidRuntimeSettings");
	const TCHAR* MeshLoadVerboseLogKey = TEXT("vroid.MeshLoad.VerboseLog");
	const TCHAR* RuntimeLoadVerboseLogKey = TEXT("vroid.RuntimeLoad.VerboseLog");
#if WITH_EDITOR
	const FString DefaultEngineIniPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini"));
	FConfigCacheIni::NormalizeConfigIniPath(DefaultEngineIniPath);
#else
	const FString DefaultEngineIniPath = GEngineIni;
#endif // WITH_EDITOR
	// Retrieve the value from the ini file. If the value is not set, write a new one.
	const auto SetVerboseLogFromConfig = [Section, DefaultEngineIniPath](const TCHAR* Key, TAutoConsoleVariable<bool>& CVar)
	{
		if (bool bLogEnabled = false; GConfig->GetBool(Section, Key, bLogEnabled, DefaultEngineIniPath))
		{
			CVar->Set(bLogEnabled);
		}
		else
		{
			GConfig->SetBool(Section, Key, CVar.GetValueOnAnyThread(), DefaultEngineIniPath);
			GConfig->Flush(false, DefaultEngineIniPath);
		}
	};
	SetVerboseLogFromConfig(MeshLoadVerboseLogKey, CVarVRoidRuntimeMeshLoadVerboseLog);
	SetVerboseLogFromConfig(RuntimeLoadVerboseLogKey, CVarVRoidRuntimeLoadVerboseLog);
	// When the value of the CVar changes, write it to the ini file.
	const auto SaveVerboseLogToConfig = [Section, DefaultEngineIniPath](const TCHAR* Key, TAutoConsoleVariable<bool>& CVar)
	{
		CVar->SetOnChangedCallback(FConsoleVariableDelegate::CreateLambda(
			[Section, Key, DefaultEngineIniPath](const IConsoleVariable* ChangedCVar)
			{
				GConfig->SetBool(Section, Key, ChangedCVar->GetBool(), DefaultEngineIniPath);
				GConfig->Flush(false, DefaultEngineIniPath);
			})
		);
	};
	SaveVerboseLogToConfig(MeshLoadVerboseLogKey, CVarVRoidRuntimeMeshLoadVerboseLog);
	SaveVerboseLogToConfig(RuntimeLoadVerboseLogKey, CVarVRoidRuntimeLoadVerboseLog);
}

void FVRoidSdkModule::ShutdownModule()
{
#if WITH_EDITOR
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule == nullptr)
	{
		return;
	}
	SettingsModule->UnregisterSettings("Project", "Plugins", "VRoidSdk");
#endif // WITH_EDITOR
}

IMPLEMENT_MODULE(FVRoidSdkModule, VRoidSdk)
