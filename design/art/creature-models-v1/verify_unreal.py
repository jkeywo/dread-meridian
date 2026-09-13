import unreal,json,math
from pathlib import Path
P=Path(__file__).resolve().parent;report={}
skel=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
stock=dict(zip(map(str,unreal.DMArtRigLibrary.get_skeleton_bone_names(skel)),unreal.DMArtRigLibrary.get_skeleton_reference_transforms(skel)))
for asset,spec in json.loads((P/'manifest.json').read_text()).items():
    mesh=unreal.load_asset(spec['native_path']);assert mesh,asset
    if spec['humanoid']:
        assert mesh.skeleton==skel
        mats=[s.material_interface for s in mesh.get_editor_property('materials')]
        comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(mesh)
        for i in range(comp.get_num_bones()):
            name=str(comp.get_bone_name(i));actual=comp.get_ref_pose_transform(i);expected=stock[name]
            assert (actual.translation-expected.translation).length()<.001,(asset,name)
            a=actual.rotation;b=expected.rotation;dot=abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)
            assert math.degrees(2*math.acos(min(1,dot)))<.03,(asset,name)
        for path in ['/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle','/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd','/Game/DreadMeridian/Presentation/Animations/A_DM_Death_1']:
            clip=unreal.load_asset(path);pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(clip,clip.get_play_length()*.4,unreal.AnimPoseEvaluationOptions());assert unreal.AnimPoseExtensions.is_valid(pose)
    else:mats=[mesh.get_material(0)]
    for mat in mats:
        assert mat,asset+' missing saved material'
        if spec['humanoid']:assert mat.get_editor_property('used_with_skeletal_mesh')
        node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
        assert isinstance(node,unreal.MaterialExpressionTextureSample) and node.texture,asset
    report[asset]={'passed':True,'mesh':mesh.get_path_name(),'materials':[m.get_path_name() for m in mats],'humanoid':spec['humanoid']}
(P/'unreal-readback-validation.json').write_text(json.dumps(report,indent=2))
