import unreal,json,pathlib
p=pathlib.Path(__file__).resolve().parent
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX False')
skeleton=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
task=unreal.AssetImportTask();task.filename=str(p/'smugglers-tpose.fbx');task.destination_path='/Game/DreadMeridian/Characters/Enemies/Smugglers';task.destination_name='A_Smugglers_TPose';task.automated=True;task.replace_existing=True;task.save=True
opt=unreal.FbxImportUI();opt.import_mesh=False;opt.import_as_skeletal=True;opt.import_animations=True;opt.skeleton=skeleton;opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
task.options=opt;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
anims=[unreal.load_asset(n) for n in task.imported_object_paths if isinstance(unreal.load_asset(n),unreal.AnimSequence)]
assert len(anims)==1,task.imported_object_paths
anim=anims[0];assert anim.get_editor_property('skeleton')==skeleton
pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,0,unreal.AnimPoseEvaluationOptions());assert unreal.AnimPoseExtensions.is_valid(pose)
# Both arms must be horizontal in the saved native clip.
checks={}
for side in ['l','r']:
    arm=unreal.AnimPoseExtensions.get_bone_pose(pose,'upperarm_'+side,unreal.AnimPoseSpaces.WORLD)
    hand=unreal.AnimPoseExtensions.get_bone_pose(pose,'hand_'+side,unreal.AnimPoseSpaces.WORLD)
    delta=hand.translation-arm.translation;assert abs(delta.z)<.02,(side,str(delta))
    checks[side]={'height_delta_cm':delta.z,'span_cm':delta.length()}
(p/'unreal-tpose-validation.json').write_text(json.dumps({'animation':anim.get_path_name(),'arms':checks,'passed':True},indent=2))
unreal.log('TPOSE_IMPORT_COMPLETE')
