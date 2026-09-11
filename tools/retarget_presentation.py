import unreal,json,pathlib
root='/Game/DreadMeridian/Presentation/Animations'
tools=unreal.AssetToolsHelpers.get_asset_tools()
manifest=json.loads(pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/migration.json').read_text())
target=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
assert target
def rig(name,mesh):
 p=root+'/'+name
 a=unreal.load_asset(p) if unreal.EditorAssetLibrary.does_asset_exist(p) else tools.create_asset(name,root,unreal.IKRigDefinition,unreal.IKRigDefinitionFactory())
 c=unreal.IKRigController.get_controller(a);c.set_skeletal_mesh(mesh)
 assert c.apply_auto_generated_retarget_definition()
 unreal.EditorAssetLibrary.save_loaded_asset(a);return a
targetrig=rig('IK_DM_Manny',target)
reports=[]
for pack in ['AnimStarterPack','FightingAnimsetPro','OpenWorldAnimset']:
 source=unreal.load_asset('/Game/'+pack+'/UE4_Mannequin/Mesh/SK_Mannequin');assert source,pack
 sr=rig('IK_DM_'+pack,source)
 name='RTG_DM_'+pack;p=root+'/'+name
 rt=unreal.load_asset(p) if unreal.EditorAssetLibrary.does_asset_exist(p) else tools.create_asset(name,root,unreal.IKRetargeter,unreal.IKRetargetFactory())
 c=unreal.IKRetargeterController.get_controller(rt)
 c.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE,sr);c.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET,targetrig)
 c.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE,source);c.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET,target)
 c.add_default_ops();c.auto_map_chains(unreal.AutoMapChainType.FUZZY,True)
 c.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
 unreal.EditorAssetLibrary.save_loaded_asset(rt)
 paths=[a['path'] for a in manifest['selected'] if a['class']=='AnimSequence' and a['path'].startswith('/Game/'+pack+'/')]
 inputs=unreal.IKRetargetBatchOperationInputs()
 for k,v in dict(assets_to_retarget=[unreal.EditorAssetLibrary.find_asset_data(p) for p in paths],source_mesh=source,target_mesh=target,ik_retarget_asset=rt,target_path=root,prefix='A_DM_',include_referenced_assets=False,overwrite_existing_files=True,retain_additive_flags=False).items():inputs.set_editor_property(k,v)
 results=unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
 assert len(results)==len(paths),(pack,len(results),len(paths))
 for d in results:
  a=d.get_asset();a.set_editor_property('enable_root_motion',False);a.set_editor_property('force_root_lock',True)
  unreal.EditorAssetLibrary.save_loaded_asset(a)
  reports.append({'path':a.get_path_name(),'skeleton':a.get_editor_property('skeleton').get_path_name(),'length':a.get_play_length()})
pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/retarget-validation.json').write_text(json.dumps(reports,indent=2))
unreal.log('PRESENTATION_RETARGET_COMPLETE')
