// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using Tools.DotNETCommon;

public class Undercover : ModuleRules
{
	public Undercover(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
        "Core",
        "CoreUObject",
        "Engine",
        "UMG",
		"Slate",
        "CoreUObject",
        "InputCore",
        "GameplayTags",
        "OnlineSubsystem",
        "OnlineSubsystemUtils",
		"GameLiftServerSDK",
		"AWSOSS",
		"OpenSSL"
      });

		string modulePath = ModuleDirectory;

		PublicIncludePaths.Add(modulePath);
		PrivateIncludePaths.Add(modulePath);

        OptimizeCode = CodeOptimization.InShippingBuildsOnly;

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
