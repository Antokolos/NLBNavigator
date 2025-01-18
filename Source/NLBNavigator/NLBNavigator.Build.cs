using UnrealBuildTool;
using System.IO;

public class NLBNavigator : ModuleRules
{
    public NLBNavigator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG",
            "NLBCore", "PLDCore" 
        });

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory,
			Path.Combine(ModuleDirectory, "Adapters"),
			Path.Combine(ModuleDirectory, "NLB"),
			Path.Combine(ModuleDirectory, "PLD")
		});
    }
}
