import unreal,pathlib,json,hashlib
root='/Game/DreadMeridian/Presentation/Animations'
src=root+'/LocomotionSource'
folder=pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/locomotion')
tools=unreal.AssetToolsHelpers.get_asset_tools()
def imp(name,skeleton=None):
 t=unreal.AssetImportTask();t.filename=str(folder/(name+'.fbx'));t.destination_path=src;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
 o=unreal.FbxImportUI();o.automated_import_should_detect_type=False;o.import_materials=False;o.import_textures=False
 o.import_as_skeletal=True;o.import_mesh=skeleton is None;o.import_animations=skeleton is not None
 o.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION if skeleton else unreal.FBXImportType.FBXIT_SKELETAL_MESH
 if skeleton:o.skeleton=skeleton
 t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t])
 a=unreal.load_asset(src+'/'+name);assert a,name
 return a
source=imp('SKM_UEFN_Mannequin');sk=source.get_editor_property('skeleton')
assert sk
unreal.EditorAssetLibrary.save_loaded_asset(sk,False)
names=['M_Neutral_Run_Start_F_Rfoot','M_Neutral_Run_Stop_F_Rfoot','M_Neutral_Stand_Turn_090_L','M_Neutral_Stand_Turn_090_R']
anims=[imp(n,sk) for n in names]
for a in anims:
 assert a.get_editor_property('skeleton')==sk
 unreal.EditorAssetLibrary.save_loaded_asset(a,False)
target=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
def rig(name,mesh):
 a=unreal.load_asset(root+'/'+name) if unreal.EditorAssetLibrary.does_asset_exist(root+'/'+name) else tools.create_asset(name,root,unreal.IKRigDefinition,unreal.IKRigDefinitionFactory())
 c=unreal.IKRigController.get_controller(a);c.set_skeletal_mesh(mesh);assert c.apply_auto_generated_retarget_definition()
 unreal.EditorAssetLibrary.save_loaded_asset(a);return a
sr=rig('IK_DM_UEFN',source);tr=unreal.load_asset(root+'/IK_DM_Manny')
rtpath=root+'/RTG_DM_UEFN';existing=unreal.EditorAssetLibrary.does_asset_exist(rtpath)
rt=unreal.load_asset(rtpath) if existing else tools.create_asset('RTG_DM_UEFN',root,unreal.IKRetargeter,unreal.IKRetargetFactory())
c=unreal.IKRetargeterController.get_controller(rt)
c.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE,sr);c.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET,tr)
c.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE,source);c.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET,target)
if not existing:c.add_default_ops()
c.auto_map_chains(unreal.AutoMapChainType.FUZZY,True);c.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
unreal.EditorAssetLibrary.save_loaded_asset(rt)
i=unreal.IKRetargetBatchOperationInputs()
for k,v in dict(assets_to_retarget=[unreal.EditorAssetLibrary.find_asset_data(a.get_path_name()) for a in anims],source_mesh=source,target_mesh=target,ik_retarget_asset=rt,target_path=src+'/Retargeted',prefix='A_DM_Uncut_',include_referenced_assets=False,overwrite_existing_files=True,retain_additive_flags=False).items():i.set_editor_property(k,v)
expected=[src+'/Retargeted/A_DM_Uncut_'+n for n in names]
existing_outputs=all(unreal.EditorAssetLibrary.does_asset_exist(p) for p in expected)
r=[unreal.EditorAssetLibrary.find_asset_data(p) for p in expected] if existing_outputs else unreal.IKRetargetBatchOperation.run_batch_retarget(i)
assert len(r)==4
pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(unreal.load_asset(root+'/A_DM_Idle_Rifle_Hip'),0,unreal.AnimPoseEvaluationOptions())
comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(target)
bones=[]
for n in unreal.DMArtRigLibrary.get_skeleton_bone_names(target.get_editor_property('skeleton')):
 p=n
 while str(p)!='None':
  if str(p)=='spine_01':bones.append(n);break
  p=comp.get_parent_bone(p)
transforms=[unreal.AnimPoseExtensions.get_bone_pose(pose,b,unreal.AnimPoseSpaces.LOCAL) for b in bones]
report=[]
for d in r:
 full=d.get_asset()
 unreal.EditorAssetLibrary.save_loaded_asset(full,False)
 ap=root+'/'+full.get_name().replace('A_DM_Uncut_','A_DM_Locomotion_')
 a=unreal.load_asset(ap) if unreal.EditorAssetLibrary.does_asset_exist(ap) else unreal.EditorAssetLibrary.duplicate_asset(full.get_path_name(),ap)
 window=(.3,.85) if 'Start' in a.get_name() else (.8,1.95) if 'Stop' in a.get_name() else (.25,.95) if '_L' in a.get_name().split('Neutral')[-1] else (.25,1.25)
 assert unreal.DMArtRigLibrary.copy_animation_window(full,a,*window)
 a.set_editor_property('enable_root_motion',False);a.set_editor_property('force_root_lock',True);unreal.EditorAssetLibrary.save_loaded_asset(a)
 rp=root+'/'+a.get_name()+'_Rifle'
 rifle=unreal.load_asset(rp) if unreal.EditorAssetLibrary.does_asset_exist(rp) else unreal.EditorAssetLibrary.duplicate_asset(a.get_path_name(),rp)
 assert unreal.DMArtRigLibrary.copy_animation_window(a,rifle,0,a.get_play_length()+.001)
 assert unreal.DMArtRigLibrary.bake_constant_bone_pose(rifle,bones,transforms)
 rifle.set_editor_property('enable_root_motion',False);rifle.set_editor_property('force_root_lock',True);unreal.EditorAssetLibrary.save_loaded_asset(rifle)
 for clip in [a,rifle]:
  assert clip.get_editor_property('skeleton')==target.get_editor_property('skeleton')
  assert clip.get_play_length()>0
  report.append({'path':clip.get_path_name(),'length':clip.get_play_length(),'root_locked':True})
manifest={'source_project':'GameAnimationSample','method':'FBX animation export; reference skeleton recovered from FBX with a temporary triangle mesh; IK retarget to Manny, cropped transition windows, rifle upper-body pose baked from Idle_Rifle_Hip','files':[{'file':f.name,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in folder.glob('*.fbx')],'clips':report}
(folder/'manifest.json').write_text(json.dumps(manifest,indent=2))
unreal.log('LOCOMOTION_IMPORT_COMPLETE')

