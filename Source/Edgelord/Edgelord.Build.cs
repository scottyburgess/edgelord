// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Edgelord : ModuleRules
{
	public Edgelord(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"OnlineSubsystem",
			"OnlineSubsystemSteam",
			"OnlineSubsystemUtils",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"PhysicsCore"
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Edgelord",
			"Edgelord/Variant_Platforming",
			"Edgelord/Variant_Platforming/Animation",
			"Edgelord/Variant_Combat",
			"Edgelord/Variant_Combat/AI",
			"Edgelord/Variant_Combat/Animation",
			"Edgelord/Variant_Combat/Gameplay",
			"Edgelord/Variant_Combat/Interfaces",
			"Edgelord/Variant_Combat/UI",
			"Edgelord/Variant_SideScrolling",
			"Edgelord/Variant_SideScrolling/AI",
			"Edgelord/Variant_SideScrolling/Gameplay",
			"Edgelord/Variant_SideScrolling/Interfaces",
			"Edgelord/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
