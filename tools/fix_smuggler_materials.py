import unreal
for name in ['Gunman','Bruiser','Lookout','Bomber','GangBoss']:
 mesh=unreal.load_asset('/Game/DreadMeridian/Characters/Enemies/Smugglers/'+name+'/SKM_'+name)
 assert mesh,name
 for slot in mesh.get_editor_property('materials'):
  mat=slot.material_interface
  unreal.MaterialEditingLibrary.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
  unreal.MaterialEditingLibrary.recompile_material(mat)
  unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('SMUGGLER_MATERIAL_USAGE_SAVED')
