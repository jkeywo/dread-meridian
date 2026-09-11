"""Use the measured FBX axis conversion to match SK_Mannequin's exact rest pose."""
import bpy,json
from pathlib import Path
from mathutils import Matrix,Quaternion,Vector
p=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(p/'manny-reference.blend'))
a=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
idle=a.animation_data.action;idle.name='Manny_Idle'
before=set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=str(p/'manny-walk.fbx'),automatic_bone_orientation=False)
other=next(o for o in set(bpy.data.objects)-before if o.type=='ARMATURE')
walk=other.animation_data.action;walk.name='Manny_Walk';walk.use_fake_user=True
for ob in set(bpy.data.objects)-before:bpy.data.objects.remove(ob,do_unlink=True)
cal=json.loads((p/'fbx-calibration-source.json').read_text())['photographer']
stock=json.loads((p/'stock-skeleton-transforms.json').read_text())
C=Matrix.Diagonal((1,-1,1,1))
def globals_(rows):
    out={}
    for n,parent in cal['bones'].items():
        d=rows[n];q=d['q'];m=Quaternion((q[3],*q[:3])).to_matrix().to_4x4();m.translation=Vector(d['t'])/100
        out[n]=out[parent]@m if parent in out else m
    return {n:C@m@C for n,m in out.items()}
oldUE=globals_(cal['reference_transforms']);newUE=globals_(stock)
target={n:newUE[n]@oldUE[n].inverted()@(a.matrix_world@b.matrix_local).normalized() for n,b in ((b.name,b) for b in a.data.bones)}
cache={}
for action in [idle,walk]:
    a.animation_data.action=action;a.animation_data.action_slot=action.slots[0]
    cache[action.name]=[]
    for f in range(int(action.frame_range[0]),int(action.frame_range[1])+1):
        bpy.context.scene.frame_set(f);bpy.context.view_layer.update()
        cache[action.name].append((f,a.matrix_basis.copy(),{b.name:b.matrix.copy() for b in a.pose.bones}))
a.animation_data_clear();a.location=(0,0,0);a.rotation_euler=(0,0,0);a.scale=(.01,.01,.01)
for b in a.pose.bones:b.matrix_basis=Matrix.Identity(4)
bpy.context.view_layer.update();bpy.context.view_layer.objects.active=a
bpy.ops.object.mode_set(mode='EDIT')
for b in a.data.edit_bones:
    m=(a.matrix_world.inverted()@target[b.name]).normalized();b.matrix=m
bpy.ops.object.mode_set(mode='OBJECT')
for name,frames in cache.items():
    old=bpy.data.actions.get(name);old.name=name+'_ExportSource'
    action=bpy.data.actions.new(name);action.use_fake_user=True
    a.animation_data_create();a.animation_data.action=action
    for f,obj,bones in frames:
        a.matrix_basis=obj
        for n,m in bones.items():
            b=a.pose.bones[n];b.matrix=m;bpy.context.view_layer.update()
            b.keyframe_insert(data_path='rotation_quaternion',frame=f);b.keyframe_insert(data_path='location',frame=f);b.keyframe_insert(data_path='scale',frame=f)
        for prop in ['location','rotation_euler','scale']:a.keyframe_insert(data_path=prop,frame=f)
    bpy.data.actions.remove(old)
a.animation_data.action=bpy.data.actions['Manny_Idle'];a.animation_data.action_slot=a.animation_data.action.slots[0]
bpy.context.scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(p/'manny-reference.blend'))
print('REFERENCE_CALIBRATED')
