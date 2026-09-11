using UnrealBuildTool;

public class DreadMeridianEditor : ModuleRules
{
    public DreadMeridianEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "DreadMeridian", "UnrealEd",
            "ToolMenus", "Slate", "SlateCore", "LevelEditor", "InputCore"
        });
    }
}
