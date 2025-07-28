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
            "NavigationSystem", // 네비게이션 시스템 모듈 추가
            "AnimGraphRuntime", // 애니메이션 그래프 런타임 모듈 추가
            "Niagara", // 나이아가라 모듈 추가
            "UMG",           // UMG 모듈 추가
            "Slate",         // Slate 모듈 추가  
            "SlateCore"      // SlateCore 모듈 추가
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
