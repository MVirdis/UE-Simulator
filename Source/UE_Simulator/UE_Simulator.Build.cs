// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UE_Simulator : ModuleRules
{
	public UE_Simulator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput" });
	}
}
