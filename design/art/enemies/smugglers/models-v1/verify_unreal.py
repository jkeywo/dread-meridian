"""Read back saved native meshes; validate sockets, materials and stock-clip evaluation."""
import unreal,json,pathlib,math
p=pathlib.Path(__file__).resolve().parent
reports={}
for asset in ['gunman','bruiser','lookout','bomber','gang-boss']:
    path='/Game/DreadMeridian/Characters/Enemies/Smugglers/'+asset.title().replace('-','')+'/SKM_'+asset.title().replace('-','')
    mesh=unreal.load_asset(path)
    assert mesh.skeleton.get_path_name()=='/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin'
    comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(mesh)
    spec=json.loads((p/(asset+'-sockets.json')).read_text())
    for name in spec:assert comp.does_socket_exist(name),name
    mats=mesh.get_editor_property('materials');assert all(s.material_interface for s in mats)
    options=unreal.AnimPoseEvaluationOptions()
    # Evaluate the supplied stock clips through Unreal's animation pose API.
    clips={}
    for name,animpath in [('idle','/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle'),('walk','/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd')]:
        anim=unreal.load_asset(animpath);assert anim.get_editor_property('skeleton')==mesh.skeleton
        pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,anim.get_play_length()*.4,options)
        assert unreal.AnimPoseExtensions.is_valid(pose)
        clips[name]={'asset':animpath,'evaluated':True}
    reports[asset]={'saved_mesh':path,'bone_count':comp.get_num_bones(),'persisted_sockets':len(spec),'persisted_materials':len(mats),'stock_clip_evaluation':clips,'passed':True}
(p/'unreal-readback-validation.json').write_text(json.dumps(reports,indent=2))
unreal.log('RIG_READBACK_VALIDATION_PASSED')


