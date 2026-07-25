// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

using UnrealBuildTool;
using System.IO;
using System.Reflection;


public class VRM4ULoader : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }

	public VRM4ULoader(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// ★2026-07-25 (UE 5.8 전환) — 이 모듈은 유니티 빌드로 합치면 깨진다.
		//   VrmConvertModel_Description.cpp:1579 의 `auto *p` + UTF8_TO_TCHAR 가 다른 .cpp 와
		//   한 덩어리로 묶이면서 C3536('p' cannot be used before it is initialized) /
		//   C2512(TStringConversion 기본 생성자 없음) / C2264(CreateSwingHead) 가 난다.
		//   처음엔 통과했는데, 그건 그 파일이 아직 깃에 없어서 UBT 의 적응형 비-유니티 작업 집합
		//   ("Using 'git status' to determine working set")에 들어가 홀로 컴파일됐기 때문이다.
		//   커밋해서 깃이 깨끗해지자 유니티로 묶여 깨졌다. → 이 모듈만 비-유니티로 고정한다.
		//   ※VRM4U 를 새 판으로 갈면 이 줄이 날아간다. 그때 빌드가 깨지면 여기를 다시 볼 것.
		bUseUnity = false;

		BuildVersion Version;
		BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version);

		//PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		//PCHUsage = PCHUsageMode.NoSharedPCHs;

		{
			var unityBuildProperty = GetType().GetProperty("bUseUnityBuild", BindingFlags.Public | BindingFlags.Instance);
			if (unityBuildProperty != null)
			{
				// UE5.8+
			//	unityBuildProperty.SetValue(this, false);
			}
		}
		{
			// UE5.7 and earlier
			var unityProperty = GetType().GetProperty("bUseUnity", BindingFlags.Public | BindingFlags.Instance);
			if (unityProperty != null)
			{
			//	unityProperty.SetValue(this, false);
			}
		}

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ThirdPartyPath, "assimp/include"),
				Path.Combine(ThirdPartyPath, "rapidjson/include")
                // ... add public include paths required here ...
            }
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Slate",
				"SlateCore",
				"Core",
				"CoreUObject",
				"Engine",
				"RHI",
				"RenderCore",
				"CinematicCamera",
				"AnimGraphRuntime",
				"Projects",
				"VRM4U",
			});
		PrivateDependencyModuleNames.Add("TimeManagement");


		if (Version.MajorVersion == 5 && Version.MinorVersion >= 8)
		{
			PrivateDependencyModuleNames.Add("MeshDescription");
			PrivateDependencyModuleNames.Add("StaticMeshDescription");
			PrivateDependencyModuleNames.Add("SkeletalMeshDescription");
		}

		if (Target.bBuildEditor) {
			PrivateDependencyModuleNames.Add("Persona");
		}

		{
			//if (Version.MajorVersion == X && Version.MinorVersion == Y)
			if (Version.MajorVersion == 5)
			{
				PrivateDependencyModuleNames.Add("IKRig");
				if (Target.bBuildEditor)
				{
					PrivateDependencyModuleNames.Add("IKRigEditor");
				}
			}
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);

		RuntimeDependencies.Add(Path.Combine(ThirdPartyPath, "vrm_specification", "vrm0", "schema", "*"));
		RuntimeDependencies.Add(Path.Combine(ThirdPartyPath, "vrm_specification", "vrm1", "*", "schema", "*"));

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
			//PrivateDependencyModuleNames.Add("VRM4UImporter");
			//PrivateIncludePaths.Add("VRM4UImporter/Private");

			//CircularlyReferencedDependentModules.Add("VRM4UImporter");
		}

		if ((Target.Platform == UnrealTargetPlatform.Win64))
		{
			string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";

			bool bDebug = false;

			if (bDebug){
				PublicDefinitions.Add("WITH_VRM4U_ASSIMP_DEBUG=1");

				string BuildString = (Target.Configuration != UnrealTargetConfiguration.Debug) ? "Debug" : "Debug";
				PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, BuildString, "assimp-vc141-mtd.lib"));

				PublicDelayLoadDLLs.Add("assimp-vc141-mtd.dll");
				RuntimeDependencies.Add(Path.Combine(ThirdPartyPath, "assimp", "bin", PlatformString, "assimp-vc141-mtd.dll"));
			}
			else
			{
				PublicDefinitions.Add("WITH_VRM4U_ASSIMP_DEBUG=0");

				string BuildString = (Target.Configuration != UnrealTargetConfiguration.Debug) ? "Release" : "Release";
				PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, BuildString, "assimp-vc141-mt.lib"));

				PublicDelayLoadDLLs.Add("assimp-vc141-mt.dll");
				RuntimeDependencies.Add(Path.Combine(ThirdPartyPath, "assimp", "bin", PlatformString, "assimp-vc141-mt.dll"));

			}
		}
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// static link
			{
				string PlatformString = "armeabi-v7a";
				PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, "libassimp.a"));
			}

			/*
			// dynamic link
			{
				string PlatformString = "armeabi-v7a";
				PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp/lib", PlatformString, "libassimp.so"));

				string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
				AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "VRM4ULoader_APL.xml"));
			}
			*/
		}
		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			string PlatformString = "IOS";
			PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, "libassimp.a"));
		}
		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			// static lib
			string PlatformString = "Mac";
			PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "assimp", "lib", PlatformString, "libassimp.a"));
		}

	}
}
