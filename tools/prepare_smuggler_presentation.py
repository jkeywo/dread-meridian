import unreal, pathlib, json
root='/Game/DreadMeridian/Presentation/Animations'
dest=root+'/Smugglers'
target=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
report=[]
for pack,names in [('FightingAnimsetPro',['Animations/InPlace/KB_Gun','Animations/InPlace/KB_p_Hook_R']),('OpenWorldAnimset',['Animations/MachineGun/MG_gestures1'])]:
    source=unreal.load_asset('/Game/'+pack+'/UE4_Mannequin/Mesh/SK_Mannequin')
    rt=unreal.load_asset(root+'/RTG_DM_'+pack)
    inputs=unreal.IKRetargetBatchOperationInputs()
    for k,v in dict(assets_to_retarget=[unreal.EditorAssetLibrary.find_asset_data('/Game/'+pack+'/'+n) for n in names],source_mesh=source,target_mesh=target,ik_retarget_asset=rt,target_path=dest,prefix='A_DM_',include_referenced_assets=False,overwrite_existing_files=True,retain_additive_flags=False).items():inputs.set_editor_property(k,v)
    results=unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
    assert len(results)==len(names)
    for d in results:
        a=d.get_asset();a.set_editor_property('enable_root_motion',False);a.set_editor_property('force_root_lock',True)
        unreal.EditorAssetLibrary.save_loaded_asset(a)
        report.append({'clip':a.get_path_name(),'source_pack':pack,'length':a.get_play_length()})

comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(target)
bones=[]
for bone in unreal.DMArtRigLibrary.get_skeleton_bone_names(target.get_editor_property('skeleton')):
    parent=bone
    while str(parent)!='None':
        if str(parent) in ['clavicle_l','clavicle_r']:
            bones.append(bone);break
        parent=comp.get_parent_bone(parent)
poses={'Gunman':'GunmanRifle','Bruiser':'BruiserTruncheon','Lookout':'LookoutPistol','Bomber':'BomberGrenade','GangBoss':'GangBossSmg'}
motions={'Idle':'/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle','Jog':'/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd','Start':root+'/A_DM_Locomotion_M_Neutral_Run_Start_F_Rfoot','Stop':root+'/A_DM_Locomotion_M_Neutral_Run_Stop_F_Rfoot','TurnL':root+'/A_DM_Locomotion_M_Neutral_Stand_Turn_090_L','TurnR':root+'/A_DM_Locomotion_M_Neutral_Stand_Turn_090_R'}
for role,pose_name in poses.items():
    mesh=unreal.load_asset('/Game/DreadMeridian/Characters/Enemies/Smugglers/'+role+'/SKM_'+role)
    assert mesh and mesh.get_editor_property('skeleton')==target.get_editor_property('skeleton')
    pose_clip=unreal.load_asset('/Game/DreadMeridian/Characters/Enemies/Smugglers/Poses/A_'+pose_name+'Equipped')
    pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(pose_clip,0,unreal.AnimPoseEvaluationOptions())
    transforms=[unreal.AnimPoseExtensions.get_bone_pose(pose,b,unreal.AnimPoseSpaces.LOCAL) for b in bones]
    for motion,path in motions.items():
        original=unreal.load_asset(path);out=dest+'/A_DM_'+role+'_'+motion
        a=unreal.load_asset(out) if unreal.EditorAssetLibrary.does_asset_exist(out) else unreal.EditorAssetLibrary.duplicate_asset(path,out)
        assert unreal.DMArtRigLibrary.copy_animation_window(original,a,0,original.get_play_length()+.001)
        assert unreal.DMArtRigLibrary.bake_constant_bone_pose(a,bones,transforms)
        a.set_editor_property('enable_root_motion',False);a.set_editor_property('force_root_lock',True)
        unreal.EditorAssetLibrary.save_loaded_asset(a)
        report.append({'clip':out,'equipped_pose':pose_clip.get_path_name(),'length':a.get_play_length()})
pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/smuggler-animation-validation.json').write_text(json.dumps(report,indent=2))
unreal.log('SMUGGLER_ANIMATIONS_READY: 3 combat clips and 30 equipped locomotion clips')
