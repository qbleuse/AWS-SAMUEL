// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using Tools.DotNETCommon;
using System.IO;

public class AWSOSS : ModuleRules
{
	public AWSOSS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
        "Core",
        "CoreUObject",
        "Engine",
        "CoreUObject",
        "OnlineSubsystem",
        "OnlineSubsystemUtils",
		"OnlineSubsystemNull",
		"GameLiftServerSDK",
		"HTTP",
		"Json",
		"Sockets"
      });

		PrivateDependencyModuleNames.AddRange(new string[] {});

		string modulePath = ModuleDirectory;

		Log.TraceInformation(modulePath);

		PublicIncludePaths.Add(Path.Combine(modulePath, "AWSOSS/Public"));
		PrivateIncludePaths.Add(modulePath);

        OptimizeCode = CodeOptimization.InShippingBuildsOnly;

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
