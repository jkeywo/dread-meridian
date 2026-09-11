"""Read authoritative reference transforms and sample stock animation poses without FBX export."""
import unreal, pathlib, json
out=pathlib.Path(__file__).resolve().parent
mesh=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
comp=unreal.SkeletalMeshComponent()
comp.set_skeletal_mesh_asset(mesh)
names=[comp.get_bone_name(i) for i in range(comp.get_num_bones())]
def serial(t):
    p=t.translation; q=t.rotation; s=t.scale3d
    return {'t':[p.x,p.y,p.z],'q':[q.x,q.y,q.z,q.w],'s':[s.x,s.y,s.z]}
data={'engine':unreal.SystemLibrary.get_engine_version(),'mesh':mesh.get_path_name(),'skeleton':mesh.skeleton.get_path_name(),'bones':{str(n):{'parent':str(comp.get_parent_bone(n)),**serial(comp.get_ref_pose_transform(i))} for i,n in enumerate(names)},'animations':{}}
for name,path in [('idle','/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle'),('walk','/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd')]:
    anim=unreal.load_asset(path)
    duration=anim.get_play_length()
    frames=[]
    for i in range(round(duration*30)+1):
        pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,min(i/30,duration),unreal.AnimPoseEvaluationOptions())
        frames.append({str(n):serial(unreal.AnimPoseExtensions.get_bone_pose(pose,n,unreal.AnimPoseSpaces.LOCAL)) for n in names})
    data['animations'][name]={'path':path,'duration':duration,'fps':30,'frames':frames}
(out/'manny-data.json').write_text(json.dumps(data,indent=2))
unreal.log('MANNY_DATA_COMPLETE '+str(len(names)))

