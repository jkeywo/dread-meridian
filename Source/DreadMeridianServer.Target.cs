using UnrealBuildTool;

// Dedicated server packaging requires an Unreal source build.
public class DreadMeridianServerTarget : TargetRules
{
    public DreadMeridianServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("DreadMeridian");
    }
}
