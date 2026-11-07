// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class KRASTOVExtraction : ModuleRules
{
	public KRASTOVExtraction(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"KRASTOVExtraction",
			"KRASTOVExtraction/Variant_Strategy",
			"KRASTOVExtraction/Variant_Strategy/UI",
			"KRASTOVExtraction/Variant_TwinStick",
			"KRASTOVExtraction/Variant_TwinStick/AI",
			"KRASTOVExtraction/Variant_TwinStick/Gameplay",
			"KRASTOVExtraction/Variant_TwinStick/UI"
		});



		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
