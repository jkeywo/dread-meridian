import unreal
for name in ['Sapper','Photographer','Medium','Smuggler']:
 mesh=unreal.load_asset('/Game/DreadMeridian/Characters/Investigators/'+name+'/SKM_'+name)
 for slot in mesh.get_editor_property('materials'):
  mat=slot.material_interface
  unreal.MaterialEditingLibrary.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
  unreal.MaterialEditingLibrary.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('SKELETAL_MATERIAL_USAGE_SAVED')
