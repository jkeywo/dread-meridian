"""Run through Unreal's Python commandlet; creates the native map once."""
import unreal

path = "/Game/DreadMeridian/Maps/L_CombatSandbox"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.log("Sandbox map already exists; preserving authored edits.")
else:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(path):
        raise RuntimeError("Could not create sandbox level")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    arena_class = unreal.load_class(None, "/Script/DreadMeridian.DMSandboxArena")
    arena = actors.spawn_actor_from_class(arena_class, unreal.Vector(0, 0, 0))
    if not arena:
        raise RuntimeError("Could not spawn sandbox arena")
    arena.set_actor_label("Combat Sandbox Arena")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/DreadMeridian.DMCombatGameMode"))
    if not levels.save_current_level():
        raise RuntimeError("Could not save sandbox level")
    unreal.log("DREAD_SANDBOX_MAP_CREATED")
