using UnrealBuildTool;

public class DreadMeridian : ModuleRules
{
    public DreadMeridian(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "GameplayTags", "GameplayAbilities", "GameplayTasks", "AIModule"
        });
        PrivateDependencyModuleNames.AddRange(new[] {
            "Json", "PlaytraceCapture", "EnhancedInput", "InputCore", "Niagara", "Slate", "SlateCore"
        });
    }
}
