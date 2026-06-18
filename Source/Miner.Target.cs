// Copyright Schuyler Zheng. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MinerTarget : TargetRules
{
	public MinerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;

        ExtraModuleNames.AddRange(new string[] { "Miner", "ThirdParty" });

        //bUseIris = true;
	}
}
