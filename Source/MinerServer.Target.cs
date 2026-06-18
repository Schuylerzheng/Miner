// Copyright Schuyler Zheng. All Rights Reserved.

using System;
using System.Collections.Generic;
using UnrealBuildTool;

[SupportedPlatforms(UnrealPlatformClass.Server)]
public class MinerServerTarget : TargetRules
{
    public MinerServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;

        ExtraModuleNames.AddRange(new string[] { "Miner", "ThirdParty" });

        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;

        bUseChecksInShipping = false;    // Should be false because if the server randomly crashes it will be bad, every player would disconnect and it is worse than just 1 player disconnecting. This might make people not find bugs, but it is worth it for stability.
        //bUseIris = true;
    }
}