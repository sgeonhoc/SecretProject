// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Secret_Project : ModuleRules
{
	public Secret_Project(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "Niagara" });

		// EngineCameras: UWaveOscillatorCameraShakePattern(전투 타격감 카메라 흔들림)용
		PrivateDependencyModuleNames.AddRange(new string[] { "EngineCameras" });

		// 에디터 전용: UIBuilderLibrary(위젯 BP 트리 빌더, build_all_ui.py용)가 UMGEditor/UnrealEd 사용
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UMGEditor", "UnrealEd" });
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
