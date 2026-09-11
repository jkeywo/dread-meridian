"""Independent commandlet readback after all imports have been saved."""
import unreal,json,pathlib
p=pathlib.Path(__file__).resolve().parent
script=p/'verify_unreal.py'
exec(compile(script.read_text(encoding='utf-8-sig'),str(script),'exec'),{'__file__':str(script),'__name__':'__main__'})
reports={}
for asset,spec in json.loads((p/'unreal-props-validation.json').read_text()).items():
    mesh=unreal.load_asset(spec['mesh']);assert isinstance(mesh,unreal.StaticMesh)
    comp=unreal.StaticMeshComponent();comp.set_static_mesh(mesh)
    for name,d in json.loads((p/'source'/(asset+'-locators.json')).read_text()).items():
        assert comp.does_socket_exist(name)
        socket=mesh.find_socket(name);m=d['matrix_local_blender']
        assert (socket.relative_location-unreal.Vector(m[0][3]*100,-m[1][3]*100,m[2][3]*100)).length()<.001
        q=socket.relative_rotation.quaternion();e=d['unreal_quaternion_xyzw']
        assert abs(q.x*e[0]+q.y*e[1]+q.z*e[2]+q.w*e[3])>.99999
    assert all(s.material_interface for s in mesh.get_editor_property('static_materials'))
    reports[asset]={'persisted_anchors':len(spec['anchors']),'passed':True}
clip=json.loads((p/'unreal-tpose-validation.json').read_text())
anim=unreal.load_asset(clip['animation']);assert isinstance(anim,unreal.AnimSequence)
pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,0,unreal.AnimPoseEvaluationOptions());assert unreal.AnimPoseExtensions.is_valid(pose)
script=p/'verify_players_refined.py'
exec(compile(script.read_text(encoding='utf-8-sig'),str(script),'exec'),{'__file__':str(script),'__name__':'__main__'})
(p/'unreal-equipment-readback.json').write_text(json.dumps({'props':reports,'tpose':clip['animation'],'passed':True},indent=2))
unreal.log('ALL_SAVED_ART_VALIDATION_PASSED')
