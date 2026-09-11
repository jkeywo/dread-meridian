using UnrealBuildTool;

public class PlaytraceCapture : ModuleRules
{
    public PlaytraceCapture(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Json" });
    }
}
