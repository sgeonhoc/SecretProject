// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Secret_ProjectTarget : TargetRules
{
	public Secret_ProjectTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// 2026-07-25 UE 5.8 전환 — 에디터 타깃과 같은 이유로 V7. (V7 = 5.8 기본값)
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Secret_Project");
	}
}
