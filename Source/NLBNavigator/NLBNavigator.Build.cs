using UnrealBuildTool;
using System.IO;

public class NLBNavigator : ModuleRules
{
    public NLBNavigator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG",
            "NLBPlayer", "PLDCore", "HeadMountedDisplay", "XRBase" 
        });

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory,
			Path.Combine(ModuleDirectory, "NLB"),
			Path.Combine(ModuleDirectory, "PLD"),
			Path.Combine(ModuleDirectory, "VR"),
			Path.Combine(ModuleDirectory, "UI")
		});
    }
}
