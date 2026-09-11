"""Exercise every delivered Blender skin and socket throughout both stock clips."""
import bpy,json,math
from pathlib import Path
from mathutils import Matrix
p=Path(__file__).resolve().parent
reports={}
for asset in ['gunman','bruiser','lookout','bomber','gang-boss']:
    bpy.ops.wm.open_mainfile(filepath=str(p/(asset+'-rigged.blend')))
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    sockets=[o for o in bpy.context.scene.objects if o.name.startswith('SOCKET_')]
    assert len(sockets)==(9 if asset=='bomber' else 8)
    assert all(o.parent==rig and o.parent_bone in rig.data.bones for o in sockets)
    assert all(len(v.groups)>0 and abs(sum(g.weight for g in v.groups)-1)<1e-5 for v in mesh.data.vertices)
    baselines={};posed=[];max_radius=0
    for name in ['Manny_Idle','Manny_Walk','T_Pose']:
        action=bpy.data.actions[name];rig.animation_data_create();rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0]
        frames=[int(action.frame_range[0]+(action.frame_range[1]-action.frame_range[0])*i/8) for i in range(9)]
        for frame in sorted(set(frames)):
            bpy.context.scene.frame_set(frame);bpy.context.view_layer.update()
            evaluated=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
            coords=[evaluated.matrix_world@v.co for v in evaluated.data.vertices]
            assert all(math.isfinite(x) for v in coords for x in v)
            origin=rig.matrix_world@rig.pose.bones['pelvis'].head
            radius=max((v-origin).length for v in coords);max_radius=max(max_radius,radius)
            assert radius<4,(asset,name,frame,radius,rig.scale[:],str(coords[max(range(len(coords)),key=lambda i:coords[i].length)]))
            for ob in sockets:
                local=(rig.matrix_world@rig.pose.bones[ob.parent_bone].matrix).inverted()@ob.matrix_world
                if ob.name not in baselines:baselines[ob.name]=local.copy()
                error=max(abs(local[i][j]-baselines[ob.name][i][j]) for i in range(4) for j in range(4))
                assert error<1e-4,(asset,ob.name,'socket drift',error)
            posed.append([name,frame])
    reports[asset]={'weighted_vertices':len(mesh.data.vertices),'bone_parented_sockets':len(sockets),'sampled_poses':posed,'maximum_vertex_radius_m':max_radius,'passed':True}
(p/'deformation-validation.json').write_text(json.dumps(reports,indent=2))
print('DEFORMATION_VALIDATION_PASSED')



