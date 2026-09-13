"""Create the playable fixed fishing village map; preserve existing authored edits."""
import unreal

path = "/Game/DreadMeridian/Maps/L_FishingVillage"
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(path):
        raise RuntimeError("Could not create fishing village")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    village = actors.spawn_actor_from_class(unreal.load_class(None, "/Script/DreadMeridian.DMFishingVillage"), unreal.Vector())
    if not village:
        raise RuntimeError("Could not create village terrain")
    village.set_actor_label("Fishing Village - Fixed Scenario")
    palette = {"Ground": (.12,.19,.12), "Water": (.07,.17,.20), "Wood": (.28,.18,.09), "Stone": (.34,.35,.30), "Reeds": (.22,.29,.10), "Mud": (.20,.16,.12)}
    materials = {}
    for name, color in palette.items():
        folder = "/Game/DreadMeridian/Maps/VillageMaterials"
        material_path = folder + "/M_Village" + name
        mat = unreal.load_asset(material_path) if unreal.EditorAssetLibrary.does_asset_exist(material_path) else None
        if mat is None:
            mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_Village" + name, folder, unreal.Material, unreal.MaterialFactoryNew())
            expr = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
            expr.set_editor_property("constant", unreal.LinearColor(*color, 1))
            unreal.MaterialEditingLibrary.connect_material_property(expr, "", unreal.MaterialProperty.MP_BASE_COLOR)
            unreal.MaterialEditingLibrary.recompile_material(mat)
            unreal.EditorAssetLibrary.save_asset(material_path)
        materials[name] = mat
    for component in village.get_components_by_class(unreal.StaticMeshComponent):
        name = component.get_name()
        group = "Ground" if name == "MarshGround" else "Water" if name in ("FloodedBasin", "DeepMarsh") else "Wood" if any(key in name for key in ("Hut", "Store", "Boathouse", "Causeway", "Approach", "Jetty")) else "Stone"
        if name.startswith("Reeds"):
            group = "Reeds"
        elif name == "BasinBed":
            group = "Mud"
        component.set_material(0, materials[group])
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/DreadMeridian.DMFishingVillageGameMode"))
    if not levels.save_current_level():
        raise RuntimeError("Could not save fishing village")
unreal.log("DREAD_FISHING_VILLAGE_MAP_READY")
