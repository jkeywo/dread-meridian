import unreal,json
from pathlib import Path
P=Path(__file__).resolve().parent;reports={}
for asset,spec in json.loads((P/'manifest.json').read_text()).items():
    mesh=unreal.load_asset(spec['native_path']);assert mesh
    assert mesh.skeleton.get_path_name()=='/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin'
    comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(mesh)
    sockets=json.loads((P/f'{asset}-sockets.json').read_text());assert all(comp.does_socket_exist(s) for s in sockets)
    mats=mesh.get_editor_property('materials');assert all(s.material_interface and 'M_Supplied_' in s.material_interface.get_name() for s in mats)
    assert all(s.material_interface.get_editor_property('used_with_skeletal_mesh') for s in mats),asset+' lacks skeletal material usage'
    for slot in mats:
        node=unreal.MaterialEditingLibrary.get_material_property_input_node(slot.material_interface,unreal.MaterialProperty.MP_BASE_COLOR)
        assert isinstance(node,unreal.MaterialExpressionTextureSample),asset+' lacks basecolor connection'
        assert node.texture and 'basecolor' in node.texture.get_name(),asset+' lacks basecolor texture'
    options=unreal.AnimPoseEvaluationOptions();clips=[]
    for path in ['/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle','/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd','/Game/DreadMeridian/Presentation/Animations/A_DM_Idle_Rifle_Hip']:
        anim=unreal.load_asset(path);assert anim and anim.get_editor_property('skeleton')==mesh.skeleton
        pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,anim.get_play_length()*.4,options);assert unreal.AnimPoseExtensions.is_valid(pose);clips.append(path)
    reports[asset]={'mesh':mesh.get_path_name(),'bones':comp.get_num_bones(),'sockets':len(sockets),'materials':[s.material_interface.get_path_name() for s in mats],'evaluated_clips':clips,'passed':True}
(P/'unreal-readback-validation.json').write_text(json.dumps(reports,indent=2))
unreal.log('SUPPLIED_MODELS_READBACK_PASSED')
