// Copyright Schuyler Zheng. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MinerEditorTarget : TargetRules
{
	public MinerEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;

        ExtraModuleNames.AddRange(new string[] { "Miner", "ThirdParty" });
		
		//bUseIris = true;
	}
}
