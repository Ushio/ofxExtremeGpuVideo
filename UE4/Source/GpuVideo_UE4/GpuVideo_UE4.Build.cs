// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;

public class GpuVideo_UE4 : ModuleRules
{
	public string ThirdPartyLibraryPath
    {
		get
        {
			return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/"));
        }
    }

	public GpuVideo_UE4(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RenderCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Core" });

		// Becase UE4.27 cannot find lz4.
		if(Target.bBuildEditor)
        {
			PublicDefinitions.Add("DEVELOPMENT_ONLY");
			PublicSystemIncludePaths.Add(ThirdPartyLibraryPath);
        }
	}
}
