// Copyright Schuyler Zheng. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Miner : ModuleRules
{
	public Miner(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"ThirdParty"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"OnlineSubsystem", // For online features
			"GeometryScriptingCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			/* The next 4 lines are for the dynamic terrain */
			"GeometryFramework",
			"GeometryCore",
			"DynamicMesh",
			"ModelingComponents"
		});

		PublicIncludePaths.AddRange(new string[] {
			"Miner",
			"Miner/Variant_Horror",
			"Miner/Variant_Shooter",
			"Miner/Variant_Shooter/AI",
			"ThirdParty",
		});

		SetupIrisSupport(Target);
	}
}
