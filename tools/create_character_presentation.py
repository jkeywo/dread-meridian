"""Create the tiny shared emissive particle material. Never replace existing art."""
import unreal
path = '/Game/DreadMeridian/Presentation/M_AttackGlow'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_AttackGlow', '/Game/DreadMeridian/Presentation', unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVectorParameter)
    color.set_editor_property('parameter_name', 'Tint')
    color.set_editor_property('default_value', unreal.LinearColor(4, 2, 1, 1))
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
mat = unreal.load_asset(path)
unreal.MaterialEditingLibrary.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
unreal.MaterialEditingLibrary.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(path)
for asset in ['/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple', '/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle', '/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd']:
    assert unreal.load_asset(asset), asset
print('DREAD_CHARACTER_ASSETS_PASSED')
