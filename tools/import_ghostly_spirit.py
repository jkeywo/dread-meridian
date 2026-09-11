# Import the ghostly-figure static mesh and its base color texture for the Medium's spirits,
# and build a translucent unlit material so DMAbilityMarker can render bound/wandering spirits.
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'design/art/presentation/spirits'
DEST = '/Game/DreadMeridian/Presentation/Props'
tools = unreal.AssetToolsHelpers.get_asset_tools()

mesh_task = unreal.AssetImportTask()
mesh_task.filename = str(SRC / 'SM_GhostlyFigure.fbx')
mesh_task.destination_path = DEST
mesh_task.destination_name = 'SM_GhostlyFigure'
mesh_task.automated = True
mesh_task.replace_existing = True
mesh_task.save = True
mesh_options = unreal.FbxImportUI()
mesh_options.import_as_skeletal = False
mesh_options.import_mesh = True
mesh_options.import_animations = False
mesh_options.import_materials = False
mesh_options.import_textures = False
mesh_options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
mesh_options.automated_import_should_detect_type = False
mesh_options.static_mesh_import_data.combine_meshes = True
mesh_options.static_mesh_import_data.convert_scene = True
mesh_options.static_mesh_import_data.convert_scene_unit = True
mesh_task.options = mesh_options
mesh_task.factory = unreal.FbxFactory()
tools.import_asset_tasks([mesh_task])
mesh = unreal.load_asset(DEST + '/SM_GhostlyFigure')
assert isinstance(mesh, unreal.StaticMesh), 'SM_GhostlyFigure import failed'
unreal.EditorAssetLibrary.save_loaded_asset(mesh)

tex_task = unreal.AssetImportTask()
tex_task.filename = str(SRC / 'T_GhostlyFigure_BaseColor.jpg')
tex_task.destination_path = DEST
tex_task.destination_name = 'T_GhostlyFigure_BaseColor'
tex_task.automated = True
tex_task.replace_existing = True
tex_task.save = True
tools.import_asset_tasks([tex_task])
texture = unreal.load_asset(DEST + '/T_GhostlyFigure_BaseColor')
assert isinstance(texture, unreal.Texture2D), 'T_GhostlyFigure_BaseColor import failed'
unreal.EditorAssetLibrary.save_loaded_asset(texture)

mat_path = DEST + '/M_GhostlyFigure'
material = unreal.load_asset(mat_path) if unreal.EditorAssetLibrary.does_asset_exist(mat_path) \
    else tools.create_asset('M_GhostlyFigure', DEST, unreal.Material, unreal.MaterialFactoryNew())
lib = unreal.MaterialEditingLibrary
lib.delete_all_material_expressions(material)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property('two_sided', True)

sample = lib.create_material_expression(material, unreal.MaterialExpressionTextureSample)
sample.set_editor_property('texture', texture)
lib.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# Fixed 50% opacity; ExposeMaterialParameter would let the marker tint it, but spirits don't need per-instance color.
opacity = lib.create_material_expression(material, unreal.MaterialExpressionConstant)
opacity.set_editor_property('r', 0.5)
lib.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)

lib.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('GHOSTLY_SPIRIT_IMPORT_COMPLETE')
