using UnrealBuildTool;

public class DreadMeridianEditorTarget : TargetRules
{
    public DreadMeridianEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new[] { "DreadMeridian", "DreadMeridianEditor" });
    }
}
