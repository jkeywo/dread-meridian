"""Editable equipped pose scenes and baked Manny pose clips.

Arm target solver uses joint heads, not Blender's imported display-bone tails.
Run Blender --background --python build_equipped.py -- <role> <item>.
"""
import bpy,sys,json,math
from pathlib import Path
from mathutils import Matrix,Vector,Quaternion
p=Path(__file__).resolve().parent;out=p/'equipped';out.mkdir(exist_ok=True)
role,item=sys.argv[sys.argv.index('--')+1:sys.argv.index('--')+3]
bpy.ops.wm.open_mainfile(filepath=str(p/(role+'-rigged.blend')))
bpy.context.preferences.filepaths.save_version=0
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
rig.animation_data.action=bpy.data.actions['Manny_Idle'];rig.animation_data.action_slot=bpy.data.actions['Manny_Idle'].slots[0]
bpy.context.scene.frame_set(1);bpy.context.view_layer.update()
pose={b.name:b.matrix.copy() for b in rig.pose.bones};rig.animation_data.action=None
for n,m in pose.items():rig.pose.bones[n].matrix=m
bpy.context.view_layer.update()
ref={b.name:(rig.matrix_world@b.matrix_local).normalized() for b in rig.data.bones}
sockets={o.get('attachment_id'):o for o in bpy.context.scene.objects if o.type=='EMPTY'}
hand_local={side:(rig.matrix_world@rig.pose.bones['hand_'+side].matrix).inverted()@sockets['SOCKET_hand_'+side].matrix_world for side in ['r','l']}
manifest=json.loads((p/'attachments.json').read_text());bindings=manifest['characters'][role]['states'][item+'_held']
loaded={}
for b in bindings:
    before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=str(p/(b['item']+'.glb')))
    obs=list(set(bpy.data.objects)-before);root=next(o for o in obs if o.parent is None)
    anchors={o.get('attachment_id'):o for o in obs if o.get('attachment_id')}
    loaded[b['item']]=(root,anchors)
    if b['item']!=item:
        local=root.matrix_world.inverted()@anchors[b['anchor']].matrix_world
        root.parent=sockets[b['socket']];root.matrix_parent_inverse=Matrix.Identity(4);root.matrix_basis=local.inverted()
root,anchors=loaded[item]
local_anchors={n:root.matrix_world.inverted()@a.matrix_world for n,a in anchors.items()}

def frame(forward,up=(0,0,1)):
    x=Vector(forward).normalized();y=Vector(up).cross(x).normalized();z=x.cross(y).normalized()
    return Matrix((x,y,z)).transposed().to_4x4()

# Deliberate staging: muzzle forward, equipment clear of chest, elbows down.
placements={
 'gunman-rifle':((-.10,-.28,1.23),(0,-1,.12)),
 'gang-boss-smg':((-.11,-.27,1.24),(0,-1,.10)),
 'lookout-binoculars':((0,-.20,1.66),(0,-1,0)),
 'lookout-pistol':((-.18,-.45,1.36),(0,-1,.04)),
 'bruiser-truncheon':((-.38,-.19,.96),(.25,-.35,-1)),
 'bomber-grenade':((-.35,-.27,1.13),(1,0,0)),
 'bomber-satchel':((-.38,-.16,.96),(1,0,0)),
}
pos,forward=placements[item];desired=frame(forward);desired.translation=Vector(pos)
root.matrix_world=desired;bpy.context.view_layer.update()
controls=bpy.data.collections.new('Equipment pose controls');bpy.context.scene.collection.children.link(controls)
targets={};errors={}

def solve_arm(side,target,pole):
    """Analytic two-link solution, followed by exact wrist orientation."""
    s=-1 if side=='r' else 1
    a=rig.matrix_world@rig.pose.bones['upperarm_'+side].head
    b0=ref['lowerarm_'+side].translation;a0=ref['upperarm_'+side].translation;c0=ref['hand_'+side].translation
    l1=(b0-a0).length;l2=(c0-b0).length;c=target.translation;delta=c-a;dist=delta.length
    assert abs(l1-l2)+.001<dist<l1+l2-.001,(role,item,side,'unreachable',dist,l1+l2)
    axis=delta.normalized();pol=Vector(pole)-a;perp=(pol-axis*pol.dot(axis)).normalized()
    along=(l1*l1-l2*l2+dist*dist)/(2*dist);height=math.sqrt(max(0,l1*l1-along*along));b=a+axis*along+perp*height
    for name,start,end,original in [('upperarm',a,b,b0-a0),('lowerarm',b,c,c0-b0)]:
        n=name+'_'+side;m=(original.rotation_difference(end-start).to_matrix()@ref[n].to_3x3()).to_4x4()
        m=Matrix.LocRotScale(start,m.to_quaternion(),rig.matrix_world.to_scale())
        rig.pose.bones[n].matrix=rig.matrix_world.inverted()@m;bpy.context.view_layer.update()
    rig.pose.bones['hand_'+side].matrix=rig.matrix_world.inverted()@target;bpy.context.view_layer.update()
    # Copy the driving limb transform to its twist auxiliaries in rest-relative space.
    for limb in ['upperarm','lowerarm']:
        parent=limb+'_'+side
        for i in [1,2]:
            n=f'{limb}_twist_{i:02d}_{side}'
            rig.pose.bones[n].matrix=rig.pose.bones[parent].matrix@rig.data.bones[parent].matrix_local.inverted()@rig.data.bones[n].matrix_local
    ob=bpy.data.objects.new('CTRL_wrist_'+side,None);controls.objects.link(ob);ob.empty_display_type='ARROWS';ob.empty_display_size=.075;ob.matrix_world=target
    ob['usage']='Wrist target for analytic two-link arm solve; rerun solver after moving.'
    targets[side]=ob

for side,name in [('r','ANCHOR_grip_r'),('l','ANCHOR_support_l')]:
    if name not in anchors:continue
    grip=desired@local_anchors[name]
    # The left support palm approaches the opposite side of the equipment.
    if side=='l':
        flip=Matrix.Rotation(math.pi,4,'X');grip=grip@flip
    target=grip@hand_local[side].inverted()
    solve_arm(side,target,(-.65 if side=='r' else .65,-.12,1.00 if item!='lookout-binoculars' else 1.30))

# Curl the weighted finger chains around their local flexion axes.
for side in targets:
    longitudinal=(ref['middle_01_'+side].translation-ref['hand_'+side].translation).normalized()
    across=(ref['index_01_'+side].translation-ref['pinky_01_'+side].translation).normalized()
    palm_normal=longitudinal.cross(across).normalized()*(-1 if side=='r' else 1)
    for finger in ['index','middle','ring','pinky']:
        for j,angle in [(1,36),(2,52),(3,30)]:
            n=f'{finger}_{j:02d}_{side}';pb=rig.pose.bones[n]
            nxt=f'{finger}_{min(j+1,3):02d}_{side}'
            direction=(ref[nxt].translation-ref[n].translation) if j<3 else (ref[n].translation-ref[f'{finger}_02_{side}'].translation)
            axis=direction.normalized().cross(palm_normal).normalized()
            local_axis=ref[n].to_3x3().inverted()@axis
            pb.rotation_mode='QUATERNION';pb.rotation_quaternion=Quaternion(local_axis,math.radians(angle))
bpy.context.view_layer.update()
action=bpy.data.actions.new('Equip_'+item.replace('-','_'));action.use_fake_user=True;rig.animation_data.action=action
for pb in rig.pose.bones:
    pb.keyframe_insert(data_path='location',frame=1);pb.keyframe_insert(data_path='rotation_quaternion',frame=1);pb.keyframe_insert(data_path='scale',frame=1)
# Attach primary equipment to the solved hand, preserving the intended grip.
root.parent=sockets['SOCKET_hand_r'];root.matrix_parent_inverse=Matrix.Identity(4);root.matrix_basis=local_anchors['ANCHOR_grip_r'].inverted()
bpy.context.scene.frame_start=1;bpy.context.scene.frame_end=2;bpy.context.scene.frame_set(1);bpy.context.view_layer.update()
for side in targets:
    actual=rig.matrix_world@rig.pose.bones['hand_'+side].matrix
    error=(actual.translation-targets[side].matrix_world.translation).length;assert error<.0001,(item,side,error)
    errors[side]=error
name=role+'-'+item.removeprefix(role+'-')+'-equipped'
text=bpy.data.texts.new('Equipment Controls.py');text.write((p/'equipment_controls.py').read_text())
mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH' and any(m.type=='ARMATURE' for m in o.modifiers))
evaluated=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
origin=rig.matrix_world@rig.pose.bones['pelvis'].head
radius=max((evaluated.matrix_world@v.co-origin).length for v in evaluated.data.vertices)
assert math.isfinite(radius) and radius<2,'Invalid equipped skin bounds'
hand_world={}
for side in targets:
    m=rig.matrix_world@rig.pose.bones['hand_'+side].matrix;flip=Matrix.Diagonal((1,-1,1,1));ue=flip@m@flip;q=ue.to_quaternion()
    hand_world[side]={'t':[v*100 for v in ue.translation],'q':[q.x,q.y,q.z,q.w]}
# Live target controls are documented authoring inputs; the delivery action is baked.
scene=bpy.context.scene
with bpy.data.libraries.load(str(p/'source'/(role+'.blend')),link=False) as (src,dst):dst.collections=[n for n in src.collections if 'Presentation' in n]
for col in dst.collections:scene.collection.children.link(col)
camera=next(o for o in scene.objects if o.type=='CAMERA');scene.camera=camera;camera.location=(2.7,-6,2.9);camera.rotation_euler=(Vector((0,-.15,1))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=2.45
scene.world=bpy.data.worlds.new('Equipped studio');scene.world.color=(.12,.12,.12)
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True;scene.render.resolution_x=1000;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(out/(name+'.blend')))
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True)
scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
(out/(name+'-validation.json')).write_text(json.dumps({'role':role,'item':item,'action':action.name,'wrist_error_m':errors,'unreal_hand_world':hand_world,'maximum_skin_radius_m':radius,'pose_clip_only':True,'finger_pose':'provisional flexion; visual contact review required','passed':True},indent=2))
print('EQUIPPED_COMPLETE',name)

