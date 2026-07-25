//
// Created by Mameo
// Copyright © 2022 pixiv Inc. All rights reserved.
//

using System.IO;
using UnrealBuildTool;

public class VRoidSdkCore : ModuleRules
{
	private string BinariesDir => Path.GetFullPath(Path.Combine(PluginDirectory, "Binaries/Win64/"));
	private string ProjectBinariesDir
	{
		get
		{
			if (Target.ProjectFile != null)
			{
				// Specify the path to Binaries/Win64 starting from the .uproject file.
				return Path.Combine(Target.ProjectFile.Directory.FullName, "Binaries", "Win64");
			}
			return Path.GetFullPath(Path.Combine(PluginDirectory, "../../Binaries/Win64"));
		}
	}

	public VRoidSdkCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.Add(ModuleDirectory);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"CoreOnline",
				"Engine",
				"Projects",
				"Json",
				"JsonUtilities",
				"HTTP",
				"ImageCore",
				"NetCore",
				"Serialization",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",

				"VRM4U",
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			LoadDll("VRoidSdk.dll");
			LoadDll("libcrypto-3-x64.dll");
			LoadDll("libcurl.dll");
			LoadDll("zlib1.dll");

			PublicAdditionalLibraries.Add(Path.Combine(PluginDirectory, "lib", "VRoidSdk.lib"));
		}

		if (bUseUnity)
		{
			PrivateDefinitions.Add("USE_UNITY_BUILD");
		}
	}

	private void CreateAndExistDirectory(string dirPath)
	{
		if (dirPath != null && Directory.Exists(dirPath) == false)
		{
			Directory.CreateDirectory(dirPath);
		}
	}

	private bool FileCopy(string src, string dst, string dllName)
	{
		try
		{
			File.Copy(src, dst, true);
			System.Console.WriteLine("copy file: {0}.", dst);
			return true;
		}
		catch (System.Exception ex)
		{
			System.Console.Error.WriteLine("failed to copy file: {0} ({1})", dllName, ex.Message);
		}

		return false;
	}

	// Binaries 以下にdll をCopy
	private void CopyDll(string dllName, string dllFullPath, string pluginPath, string projectPath)
	{
		if (File.Exists(dllFullPath) == false)
		{
			System.Console.WriteLine("file {0} does not exist.", dllName);
			return;
		}

		// Plugins/Binaries 以下にDllをCopy
		CreateAndExistDirectory(BinariesDir);
		FileCopy(dllFullPath, pluginPath, dllName);

		// Project/Binaries 以下にdllをCopy
		CreateAndExistDirectory(ProjectBinariesDir);
		FileCopy(dllFullPath, projectPath, dllName);
	}

	private void LoadDll(string dllName)
	{
		string dllPath = Path.Combine(PluginDirectory, "lib", dllName);
		PublicDelayLoadDLLs.Add(dllName);
		RuntimeDependencies.Add(dllPath);

		string binariesFullPath = Path.Combine(BinariesDir, dllName);
		PublicDelayLoadDLLs.Add(binariesFullPath);
		RuntimeDependencies.Add(binariesFullPath);

		CreateAndExistDirectory(ProjectBinariesDir);
		string projectBinariesPath = Path.Combine(ProjectBinariesDir, dllName);
		PublicDelayLoadDLLs.Add(projectBinariesPath);
		RuntimeDependencies.Add(projectBinariesPath);

		if (Target.Type == TargetRules.TargetType.Editor)
		{
			CopyDll(dllName, dllPath, binariesFullPath, projectBinariesPath);
		}
	}
}