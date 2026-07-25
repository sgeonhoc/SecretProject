// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Secret_ProjectEditorTarget : TargetRules
{
	public Secret_ProjectEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		// 2026-07-25 UE 5.8 전환 — V6(5.7 기본값)로 두면 ReturnType/Dangling/UnreachableCode
		// 경고 수준이 Off 로 남아, 설치형 UnrealEditor 와 빌드 환경이 달라져 빌드가 거부된다.
		// ("Secret_ProjectEditor modifies the values of properties ... This is not allowed")
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Secret_Project");
	}
}
