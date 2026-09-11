"""Skin the original roster to the stock Manny FBX skeleton. Blender 5.0.

No automatic bone orientation: preserve Unreal's exported axes and hierarchy.
Source T-pose landmarks provide a reversible bind-space fit; skin is baked to
the stock reference pose. The original static source files remain unchanged.
"""
import bpy,sys,json,math
import numpy as np
from pathlib import Path
from mathutils import Vector,Matrix

OUT=Path(__file__).resolve().parent
SRC=OUT/'source'
asset=sys.argv[sys.argv.index('--')+1]
bpy.ops.wm.open_mainfile(filepath=str(OUT/'manny-reference.blend'))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
rig.name='root'
idle=bpy.data.actions['Manny_Idle'];idle.use_fake_user=True
walk=bpy.data.actions['Manny_Walk'];walk.use_fake_user=True
# The reference preview scene contains an object-placement track at Y=-4.5m.
# Keep the stock bone motion, strip presentation transforms from portable clips.
for action in [idle,walk]:
    for layer in action.layers:
        for strip in layer.strips:
            for bag in strip.channelbags:
                for curve in list(bag.fcurves):
                    if curve.data_path in ['location','rotation_euler','rotation_quaternion','scale']:
                        bag.fcurves.remove(curve)
rig.animation_data_clear()
for pb in rig.pose.bones:pb.matrix_basis=Matrix.Identity(4)
rig.location=(0,0,0);rig.rotation_euler=(0,0,0)
bpy.context.view_layer.update()
ref={b.name:(rig.matrix_world@b.matrix_local).normalized() for b in rig.data.bones}

# Source T-pose body landmarks, in metres, matching the authored geometry.
cfg=json.loads((OUT/'landmarks.json').read_text())[asset]
c=cfg;z=c['shoulder'];points={};ends={}
points['pelvis']=Vector((0,.025,c['hip']))
spines=['spine_%02d'%i for i in range(1,6)]
for i,n in enumerate(spines):points[n]=Vector((0,.025,c['hip']+(z-c['hip'])*(.18+.18*i)))
points['neck_01']=Vector((0,.012,c['neck']-.035))
points['neck_02']=Vector((0,.008,c['neck']+.025))
points['head']=Vector((0,0,c['head']))
ends['head']=points['head']+Vector((0,0,.16))
for side,s in [('l',1),('r',-1)]:
    points['clavicle_'+side]=Vector((s*.07,.025,z))
    for n,x in [('upperarm',c['sx']),('lowerarm',c['elbow']),('hand',c['wrist'])]:points[n+'_'+side]=Vector((s*x,.025,z))
    ends['hand_'+side]=Vector((s*(c['palm']+.03*c['size']),.025,z))
    for n,pos in [('thigh',(s*c['leg']*.84,.025,c['hip'])),('calf',(s*c['leg'],.025,c['knee'])),('foot',(s*c['leg'],.015,.115)),('ball',(s*c['leg'],-.105,.065))]:points[n+'_'+side]=Vector(pos)
    ends['ball_'+side]=points['ball_'+side]+Vector((0,-.075,0))
    for i,(finger,dy,length) in enumerate([('index',-.027,.074),('middle',-.008,.088),('ring',.012,.081),('pinky',.030,.065)]):
        y=.025+dy*c['size'];start=c['palm']+.036*c['size'];length*=c['size']
        if asset=='sapper':y=[-.004,.016,.037,.057][i];start=.88;length=[.084,.095,.089,.071][i]
        points[finger+'_metacarpal_'+side]=Vector((s*(c['wrist']+.028*c['size']),y,z))
        for j,t in enumerate([0,.53,.79],1):points[f'{finger}_{j:02d}_{side}']=Vector((s*(start+length*t),y,z-.004*t))
        ends[finger+'_03_'+side]=Vector((s*(start+length),y,z-.006))
    thumb=[(c['palm']-.01,-.005,z-.004),(c['palm']+.008*c['size'],-.037*c['size'],z-.012),(c['palm']+.026*c['size'],-.042*c['size'],z-.014)]
    if asset=='sapper':thumb=[(.833,-.001,z-.003),(.86,-.041,z-.011),(.878,-.05,z-.014)]
    for j,pos in enumerate(thumb,1):points[f'thumb_{j:02d}_{side}']=Vector((s*pos[0],pos[1],pos[2]))
    ends['thumb_03_'+side]=points['thumb_03_'+side]+Vector((s*.016,-.005,-.002))

nextbone={'pelvis':'spine_01',**{spines[i]:spines[i+1] for i in range(4)},'spine_05':'neck_01','neck_01':'neck_02','neck_02':'head'}
for side in ['l','r']:
    for a,b in [('clavicle','upperarm'),('upperarm','lowerarm'),('lowerarm','hand'),('thigh','calf'),('calf','foot'),('foot','ball')]:nextbone[a+'_'+side]=b+'_'+side
    for f in ['index','middle','ring','pinky','thumb']:
        if f!='thumb':nextbone[f+'_metacarpal_'+side]=f+'_01_'+side
        nextbone[f+'_01_'+side]=f+'_02_'+side;nextbone[f+'_02_'+side]=f+'_03_'+side

source={};segments={}
for n,p in points.items():
    end=points[nextbone[n]] if n in nextbone else ends[n]
    rp=ref[n].translation
    if n in nextbone:re=ref[nextbone[n]].translation
    elif n=='head':re=rp+Vector((0,0,.16))
    elif n.startswith('hand_'):re=ref['middle_01_'+n[-1]].translation
    elif n.startswith('ball_'):re=rp+Vector((0,-.075,0))
    else:re=rp+(rp-ref[rig.data.bones[n].parent.name].translation).normalized()*.025
    dv=end-p;rv=re-rp
    rot=rv.rotation_difference(dv).to_matrix()
    basis=rot@ref[n].to_3x3()
    # Match length along the limb while retaining each character's girth.
    axis=dv.normalized();ratio=dv.length/max(rv.length,.001)
    stretch=Matrix.Identity(3)
    for i in range(3):
        for j in range(3):stretch[i][j]+=(ratio-1)*axis[i]*axis[j]
    m=(stretch@basis).to_4x4();m.translation=p;source[n]=m;segments[n]=(p,end)

# Stock twist joints use the same source-space fit as their driving limb.
for n,b in ((b.name,b) for b in rig.data.bones if '_twist_' in b.name):
    parent=b.parent.name
    source[n]=source[parent]@ref[parent].inverted()@ref[n]
    segments[n]=(source[n].translation,source[n].translation+Vector((0,0,.02)))

before=set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=str(SRC/(asset+'.glb')))
imported=list(set(bpy.data.objects)-before)
meshes=[o for o in imported if o.type=='MESH']
sockets=[o for o in imported if o.name.startswith('SOCKET_')]
bpy.ops.object.select_all(action='DESELECT')
for ob in meshes:ob.select_set(True)
bpy.context.view_layer.objects.active=meshes[0];bpy.ops.object.join();mesh=bpy.context.object
mesh.name='SKM_'+asset.title().replace('-','')
mw=mesh.matrix_world.copy();mesh.parent=None;mesh.matrix_world=mw
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
verts=np.empty((len(mesh.data.vertices),3),dtype=np.float64);mesh.data.vertices.foreach_get('co',verts.ravel())
names=list(source);idx={n:i for i,n in enumerate(names)}
weights=np.zeros((len(verts),len(names)),dtype=np.float32)

def add(mask,n,w):weights[mask,idx[n]]+=w
def linear(mask,ns,coords,values):
    values=np.asarray(values);j=np.clip(np.searchsorted(coords,values)-1,0,len(coords)-2)
    t=np.clip((values-np.array(coords)[j])/(np.array(coords)[j+1]-np.array(coords)[j]),0,1)
    rows=np.flatnonzero(mask)
    for k in range(len(ns)-1):
        sel=j==k;weights[rows[sel],idx[ns[k]]]+=1-t[sel];weights[rows[sel],idx[ns[k+1]]]+=t[sel]

x=np.abs(verts[:,0]);height=verts[:,2]
arms=(x>c['sx']*.72)&(height>z-.15)&(height<z+.145)
heads=(height>c['neck']-.025)&~arms
legs=(height<c['hip']-.065)&~arms
body=~(arms|heads|legs)
linear(heads,['neck_01','neck_02','head'],[c['neck']-.04,c['neck']-.005,c['neck']+.025] if asset in ['lookout','bomber'] else [c['neck']-.04,c['neck']+.005,c['head']-.07],height[heads])
linear(body,['pelvis']+spines+['neck_01'],[c['hip']]+[points[n].z for n in spines]+[c['neck']-.035],height[body])
for side,s in [('l',1),('r',-1)]:
    sideMask=verts[:,0]*s>=0
    mask=legs&sideMask
    linear(mask,['foot_'+side,'calf_'+side,'calf_'+side,'thigh_'+side,'thigh_'+side,'pelvis'],[.15,.23,c['knee']-.06,c['knee']+.06,c['hip']-.045,c['hip']+.04],height[mask])
    # Preserve rigid boots, with a short ankle blend instead of shin stretching.
    low=mask&(height<.17);weights[low]=0;add(low,'foot_'+side,1)
    hand=arms&sideMask&(x>c['wrist']+.005)
    arm=arms&sideMask&~hand
    linear(arm,['spine_05','clavicle_'+side,'upperarm_'+side,'upperarm_'+side,'lowerarm_'+side,'lowerarm_'+side,'hand_'+side],[c['sx']*.65,c['sx']*.85,c['sx']+.035,c['elbow']-.055,c['elbow']+.055,c['wrist']-.035,c['wrist']+.025],x[arm])
    palm=hand&(x<c['palm']+.030*c['size'])&(verts[:,1]>-.012*c['size'])
    add(palm,'hand_'+side,1)
    fingers=hand&~palm;rows=np.flatnonzero(fingers);v=verts[fingers]
    candidates=['hand_'+side]+[n for n in names if n.endswith('_'+side) and n.startswith(('thumb','index','middle','ring','pinky'))]
    distances=[]
    for n in candidates:
        a,b=segments[n];a=np.array(a);d=np.array(b)-a;t=np.clip(((v-a)@d)/(d@d),0,1)
        distances.append(np.linalg.norm(v-(a+t[:,None]*d),axis=1))
    distances=np.array(distances).T
    closest=np.argsort(distances,axis=1)[:,:2]
    for k in range(2):
        ds=distances[np.arange(len(v)),closest[:,k]]
        w=1/np.maximum(ds,.004)**4
        weights[rows,np.array([idx[n] for n in candidates])[closest[:,k]]]+=w

weights/=np.maximum(weights.sum(axis=1)[:,None],1e-15)
for side in ['l','r']:
    for limb in ['upperarm','lowerarm','thigh','calf']:
        n=limb+'_'+side;column=weights[:,idx[n]].copy()
        weights[:,idx[n]]*=.5
        for j in [1,2]:weights[:,idx[f'{limb}_twist_{j:02d}_{side}']]+=.25*column
assert np.all(np.abs(weights.sum(axis=1)-1)<1e-5),'Unweighted vertex'
# Bake the fitted source T-pose into the actual stock reference bind pose.
new=np.zeros_like(verts)
for n,i in idx.items():
    rows=weights[:,i]>.000001
    if not np.any(rows):continue
    m=np.array(ref[n]@source[n].inverted())
    new[rows]+=(verts[rows]@m[:3,:3].T+m[:3,3])*weights[rows,i,None]
    group=mesh.vertex_groups.new(name=n)
    for vi in np.flatnonzero(rows):group.add([int(vi)],float(weights[vi,i]),'REPLACE')
mesh.data.vertices.foreach_set('co',(new*100).astype(np.float32).ravel());mesh.data.update()
# Mesh and armature share centimetre-local coordinates and the same object
# scale, avoiding a compensating 100x mesh transform in the FBX bind pose.
mesh.parent=rig;mesh.matrix_parent_inverse=Matrix.Identity(4);mesh.matrix_basis=Matrix.Identity(4)
mod=mesh.modifiers.new('Manny skin','ARMATURE');mod.object=rig;mod.use_deform_preserve_volume=False
mesh['skeleton_target']='/Game/Characters/Mannequins/Meshes/SK_Mannequin'
rig['rig_status']='PROVISIONAL skinned concept; stock Manny reference hierarchy and animation axes'
rig['source_landmarks']=json.dumps(c)

socketdata={}
for ob in sockets:
    name=ob.get('attachment_id',ob.name);bone=ob.get('future_bone','pelvis')
    # Chest and back now follow upper thorax on Manny's five-spine layout.
    if name in ['SOCKET_stow_chest','SOCKET_stow_back']:bone='spine_05'
    world=ref[bone]@source[bone].inverted()@ob.matrix_world
    location,rotation,_=world.decompose();world=Matrix.LocRotScale(location,rotation,Vector((1,1,1)))
    ob.parent=rig;ob.parent_type='BONE';ob.parent_bone=bone;ob.matrix_world=world
    ob['bone']=bone;ob['attachment_id']=name
    local=ref[bone].inverted()@world
    flip=Matrix.Diagonal((1,-1,1,1));ueworld=flip@world@flip
    loc,quat,scale=ueworld.decompose()
    socketdata[name]={'bone':bone,'matrix_bone_local_metres':[list(r) for r in local], 'unreal_world':{'t':[v*100 for v in loc],'q':[quat.x,quat.y,quat.z,quat.w],'s':list(scale)}}
for ob in imported:
    if ob in bpy.data.objects.values() and ob.type=='EMPTY' and ob not in sockets:bpy.data.objects.remove(ob,do_unlink=True)

# Keep the supplied stock idle/walk clips as selectable actions.
rig.animation_data_create()
rig.animation_data.action=idle
rig.animation_data.action_slot=idle.slots[0]
scene=bpy.context.scene;scene.render.fps=30;scene.frame_start=1;scene.frame_end=int(idle.frame_range[1])
scene.frame_set(1)
# A selectable T-pose at stock limb lengths, retaining the original empty-hand workflow.
rig.animation_data.action=None
for pb in rig.pose.bones:pb.matrix_basis=Matrix.Identity(4)
tpose=bpy.data.actions.new('T_Pose');tpose.use_fake_user=True;rig.animation_data.action=tpose
for side,s in [('l',1),('r',-1)]:
    for n in ['upperarm_'+side,'lowerarm_'+side,'hand_'+side]:
        pb=rig.pose.bones[n]
        child=nextbone.get(n)
        target=ref[child].translation if child else ref['middle_01_'+side].translation
        direction=target-ref[n].translation
        rotate=direction.rotation_difference(Vector((s,0,0))).to_matrix().to_4x4()
        m=rotate@ref[n];m.translation=rig.matrix_world@pb.head
        if n.startswith(('lowerarm_','hand_')):
            parent=('upperarm_' if n.startswith('lowerarm_') else 'lowerarm_')+side
            length=(ref[n].translation-ref[parent].translation).length
            m.translation=rig.matrix_world@rig.pose.bones[parent].head+Vector((s*length,0,0))
        pb.matrix=rig.matrix_world.inverted()@m;bpy.context.view_layer.update()
for pb in rig.pose.bones:
    pb.keyframe_insert(data_path='rotation_quaternion',frame=1)
    pb.keyframe_insert(data_path='location',frame=1)
rig.animation_data.action=None
for pb in rig.pose.bones:pb.matrix_basis=Matrix.Identity(4)
rig.show_in_front=True
rig.data.display_type='STICK'
bpy.context.view_layer.update()
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);mesh.select_set(True)
bpy.context.view_layer.objects.active=rig
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(asset+'-rigged.blend')))
# FBX is the Unreal skeletal-mesh source. Named attachment transforms are in JSON
# and native mesh sockets, so no empty helper is imported as an extra bone.
bpy.ops.export_scene.fbx(filepath=str(OUT/(asset+'-rigged.fbx')),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,use_armature_deform_only=False,bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE')
for ob in sockets:ob.select_set(True)
rig.animation_data.action=walk;rig.animation_data.action_slot=walk.slots[0]
scene.frame_end=int(walk.frame_range[1]);scene.frame_set(1)
bpy.ops.export_scene.gltf(filepath=str(OUT/(asset+'-rigged.glb')),export_format='GLB',use_selection=True,export_extras=True,export_animations=True,export_animation_mode='ACTIVE_ACTIONS',export_all_influences=True)
(OUT/(asset+'-sockets.json')).write_text(json.dumps(socketdata,indent=2))
materials={}
for mat in mesh.data.materials:
    node=next((n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None)
    if node:materials[mat.name]={'color':list(node.inputs['Base Color'].default_value),'roughness':node.inputs['Roughness'].default_value,'metallic':node.inputs['Metallic'].default_value}
(OUT/(asset+'-materials.json')).write_text(json.dumps(materials,indent=2))
report={'asset':asset,'blender_bones':len(rig.data.bones),'unreal_root_node':'root','vertices':len(verts),'weighted_vertices':int(np.count_nonzero(weights.sum(axis=1))),'max_influences':int(np.max(np.count_nonzero(weights>1e-6,axis=1))),'weight_sum_max_error':float(np.max(abs(weights.sum(axis=1)-1))),'sockets':len(sockets),'actions':[idle.name,walk.name,tpose.name]}
(OUT/(asset+'-validation.json')).write_text(json.dumps(report,indent=2))
print('RIG_COMPLETE',json.dumps(report))


