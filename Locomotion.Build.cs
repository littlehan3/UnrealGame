using UnrealBuildTool;

public class Locomotion : ModuleRules
{
    public Locomotion(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "AnimGraphRuntime",
            "AIModule",
            "GameplayTasks",
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
