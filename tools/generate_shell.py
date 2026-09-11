"""Run through Unreal's Python commandlet; creates the shell (front end) map once.

The shell level is deliberately empty. It hosts the main menu, lobby and case report, which
ADMShellHUD draws on the canvas, and streams L_CombatSandbox in when the lobby launches.
"""
import unreal

path = "/Game/DreadMeridian/Maps/L_Shell"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.log("Shell map already exists; preserving authored edits.")
else:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(path):
        raise RuntimeError("Could not create shell level")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    settings = world.get_world_settings()
    settings.set_editor_property(
        "default_game_mode",
        unreal.load_class(None, "/Script/DreadMeridian.DMShellGameMode"),
    )
    if not levels.save_current_level():
        raise RuntimeError("Could not save shell level")
    unreal.log("DREAD_SHELL_MAP_CREATED")
