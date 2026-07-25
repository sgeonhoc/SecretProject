//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

using System.IO;
using UnrealBuildTool;

public class VRoidSdk : ModuleRules
{
	public VRoidSdk(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.Add(ModuleDirectory);
		PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../VRoidSdkCore/")));

		SetupIrisSupport(Target);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"CoreOnline",
				"Engine",
				"AnimGraphRuntime",
				"DeveloperSettings",
				"IKRig",
				"RHI",
				"RenderCore",
				"TimeManagement",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"UMG",
				"SlateCore",

				"VRoidSdkCore",
				"VRM4U",
				"VRM4ULoader",
			}
		);
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new [] { "UnrealEd", });
		}
	}
}