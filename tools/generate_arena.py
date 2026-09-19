"""Generate the local developer arena; preserve authored map edits on subsequent runs."""
import unreal

path = "/Game/DreadMeridian/Maps/L_TestArena"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.log("Test arena already exists; preserving authored edits.")
else:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(path):
        raise RuntimeError("Could not create test arena")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    arena = actors.spawn_actor_from_class(
        unreal.load_class(None, "/Script/DreadMeridian.DMTestArenaGeometry"), unreal.Vector())
    if not arena:
        raise RuntimeError("Could not spawn arena geometry")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property(
        "default_game_mode", unreal.load_class(None, "/Script/DreadMeridian.DMTestArenaGameMode"))
    if not levels.save_current_level():
        raise RuntimeError("Could not save test arena")
    unreal.log("DREAD_TEST_ARENA_CREATED")
