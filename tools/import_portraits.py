# Import original portrait layers and build the enemy hue-shift material.
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
DEST = '/Game/DreadMeridian/UI/Portraits'
players = ['Sapper', 'Photographer', 'Medium', 'Smuggler']
enemies = ['Gunman', 'Bruiser', 'Lookout', 'Bomber', 'GangBoss']
sources = {n: ROOT / 'design/art/investigators/2026-09-10-v1' / (n.lower() + '-head.png') for n in players}
sources.update({n: ROOT / 'design/art/enemies/smugglers/2026-09-10-v1' / (('gang-boss' if n == 'GangBoss' else n.lower()) + '-head.png') for n in enemies})
sources['Background'] = ROOT / 'design/art/investigators/2026-09-10-v1/background-head.png'
for name, source in sources.items():
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DEST
    task.destination_name = 'T_Portrait_' + name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(DEST + '/T_Portrait_' + name)
    assert isinstance(texture, unreal.Texture2D), name
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('never_stream', True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)

path = DEST + '/M_Portrait_EnemyBackground'
material = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_Portrait_EnemyBackground', DEST, unreal.Material, unreal.MaterialFactoryNew())
lib = unreal.MaterialEditingLibrary
lib.delete_all_material_expressions(material)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
sample = lib.create_material_expression(material, unreal.MaterialExpressionTextureSample)
sample.set_editor_property('texture', unreal.load_asset(DEST + '/T_Portrait_Background'))
shift = lib.create_material_expression(material, unreal.MaterialExpressionCustom)
shift.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inp = unreal.CustomInput()
inp.set_editor_property('input_name', 'Color')
shift.set_editor_property('inputs', [inp])
# Shift the background's green/teal hue to red, preserving saturation and value.
# Keep this in a material so the original generated artwork stays intact.
shift.set_editor_property('code', '''float hi = max(Color.r, max(Color.g, Color.b));
float lo = min(Color.r, min(Color.g, Color.b));
float d = hi - lo;
float h = 0;
if (d > 0.00001) {
    if (hi == Color.r) h = (Color.g - Color.b) / d;
    else if (hi == Color.g) h = 2 + (Color.b - Color.r) / d;
    else h = 4 + (Color.r - Color.g) / d;
    h = frac(h / 6 + 1);
}
h = frac(h + 0.6);
float s = hi > 0.00001 ? d / hi : 0;
float3 rgb = saturate(abs(frac(h + float3(0, 2.0/3.0, 1.0/3.0)) * 6 - 3) - 1);
return hi * lerp(float3(1,1,1), rgb, s);''')
lib.connect_material_expressions(sample, 'RGB', shift, 'Color')
lib.connect_material_property(shift, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('PORTRAITS: imported all nine characters, green background and red hue-shift material')
