"""Shared, local Blender modelling/export helpers for the investigator concepts."""
import bpy
import math
import random
import json
from pathlib import Path
from mathutils import Vector, Matrix

OUT=Path(__file__).resolve().parent
SOURCE=OUT/'source'/'sapper-primitives.py'
if not SOURCE.exists():SOURCE=OUT.parent/'sapper-3d-v1'/'build_sapper.py'
# Reuse only the original primitive/material helpers, not the Sapper geometry.
exec(compile(SOURCE.read_text().split('# Boots, trouser volumes')[0],str(SOURCE),'exec'),globals())
bpy.context.preferences.filepaths.save_version=0
OUT=Path(__file__).resolve().parent

def set_asset(name):
    character.name=name+' | editable geometry'
    return character

def fuse(objects,name,voxel=.005):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objects:
        bpy.context.view_layer.objects.active=ob
        for mod in list(ob.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
    for ob in objects:ob.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.object.join();ob=bpy.context.object;ob.name=name
    rem=ob.modifiers.new('Sculpted union','REMESH');rem.mode='VOXEL';rem.voxel_size=voxel;rem.use_smooth_shade=True
    bpy.ops.object.modifier_apply(modifier=rem.name)
    sm=ob.modifiers.new('Sculpt finish','SMOOTH');sm.factor=.65;sm.iterations=4
    bpy.ops.object.modifier_apply(modifier=sm.name)
    return ob

def loop(name,center,radius,mat,axis='Y',wire=.003,n=40):
    x,y,z=center
    if axis=='Y':pts=[(x+radius*math.cos(a),y,z+radius*math.sin(a)) for a in [i*math.tau/n for i in range(n+1)]]
    elif axis=='X':pts=[(x,y+radius*math.cos(a),z+radius*math.sin(a)) for a in [i*math.tau/n for i in range(n+1)]]
    else:pts=[(x+radius*math.cos(a),y+radius*math.sin(a),z) for a in [i*math.tau/n for i in range(n+1)]]
    return tube(name,pts,wire,mat,2)

def face(kind,z,skinmat,hairmat,accent,wide=1):
    # Original faces for the three remaining investigators.
    female=kind!='smuggler'; w=wide
    pieces=[ell('Head cranium',(0,.003,z),((.095 if female else .102)*w,.086 if female else .089,.14),skinmat,32,22),
            ell('Defined jaw',(0,-.023,z-.077),((.059 if female else .073)*w,.056 if female else .067,.057 if female else .067),skinmat),
            ell('Chin',(0,-.050,z-(.121 if female else .125)),((.033 if female else .043)*w,.034 if female else .046,.020 if female else .025),skinmat)]
    for s in [-1,1]:
        pieces.append(ell('Cheek plane',(s*(.059 if female else .064)*w,-.061 if female else -.067,z-.029),((.022 if female else .027)*w,.017 if female else .022,.028 if female else .031),skinmat))
    pieces.append(ell('Nose bridge',(0,-.084 if female else -.09,z-.013),((.0105 if female else .014)*w,.022 if female else .025,.033 if female else .037),skinmat))
    pieces.append(ell('Nose tip',(0,-.104 if female else -.112,z-.036),((.016 if female else .023)*w,.017 if female else .024,.012 if female else .015),skinmat))
    for s in [-1,1]:pieces.append(ell('Nose wing',(s*(.012 if female else .015)*w,-.097 if female else -.101,z-.043),((.007 if female else .011)*w,.010 if female else .015,.008 if female else .011),skinmat))
    fuse(pieces,'Sculpted '+kind+' face',.0038)
    lips=material('Natural lip colour',(.39,.15,.10) if female else (.13,.035,.021),.65)
    ivory=material('Eyes ivory',(.52,.49,.39),.4)
    dark=material('Iris and pupils',(.042,.021,.012),.45)
    brows=hairmat
    for s in [-1,1]:
        x=s*.043*w
        ell('Ear',(s*.102*w,0,z-.028),(.019,.021,.037),skinmat)
        ell('Ear inset',(s*.11*w,-.017,z-.029),(.010,.006,.024),lips)
        ey=-.082 if female else -.095;ew=.020 if female else .025
        if not female:ell('Eye socket',(x,-.084,z+.018),(.028,.012,.013),lips)
        ell('Almond eye',(x,ey,z+.020),(ew,.006 if female else .010,.009 if female else .011),ivory)
        ell('Iris',(x,ey-.006,z+.02),(.0065 if female else .009,.002,.008 if female else .010),dark,20,12)
        ell('Eye catchlight',(x-.003,ey-.009,z+.024),(.0018,.001,.002),ivory,12,8)
        tube('Upper eyelid',[(x-ew,ey+.003,z+.019),(x,ey-.007,z+.029),(x+ew,ey+.003,z+.021)],.0025 if female else .0035,skinmat,2)
        tube('Lower eyelid',[(x-ew,ey+.004,z+.018),(x,ey-.005,z+.011),(x+ew,ey+.004,z+.018)],.002 if female else .003,skinmat,2)
        tube('Arched brow',[(s*.018*w,-.088,z+.047),(s*.042*w,-.087,z+.060),(s*.070*w,-.065,z+.048)],.0032 if female else .0065,brows,2)
        if female:tube('Fine upper lashes',[(x-ew,ey+.002,z+.019),(x,ey-.009,z+.028),(x+ew,ey+.002,z+.022)],.0013,hairmat,2)
        ell('Nostril',(s*.011 if female else s*.014,-.118 if female else -.126,z-.044),(.003 if female else .004,.002,.0025),lips,12,8)
    if female:
        tube('Upper lip',[(-.030,-.092,z-.083),(-.012,-.105,z-.08),(0,-.108,z-.082),(.012,-.105,z-.08),(.03,-.092,z-.083)],.004,lips,2)
        tube('Lower lip',[(-.028,-.096,z-.085),(0,-.109,z-.091),(.028,-.096,z-.085)],.006,lips,2)
    else:
        tube('Crooked smile',[(-.041,-.094,z-.079),(-.012,-.108,z-.075),(.025,-.1,z-.067),(.041,-.092,z-.06)],.007,lips,2)
        tube('Teeth in grin',[(-.022,-.110,z-.076),(.0,-.113,z-.071),(.027,-.104,z-.066)],.0045,ivory,2)
    if kind=='photographer':
        ell('Back of short black hair',(0,.055,z+.043),(.11,.062,.11),hairmat)
        ell('Crown of short black hair',(0,.009,z+.105),(.09,.071,.049),hairmat)
        # Bring the face back into view; curls follow sides/back/top, not forehead.
        random.seed(483)
        for i in range(95):
            a=random.uniform(0,math.tau)
            theta=random.uniform(.08,1.75)
            if math.sin(a)<-.4 and theta>.96:continue
            p=(.107*math.sin(theta)*math.cos(a),.011+.096*math.sin(theta)*math.sin(a),z+.148*math.cos(theta))
            ob=ell('Sculpted short curl',p,(.021,.021,.023),hairmat,16,10)
            if i%3==0:
                tube('Curl crest',[(p[0]-.008,p[1]-.014,p[2]),(p[0],p[1]-.02,p[2]+.014),(p[0]+.009,p[1]-.012,p[2]+.006)],.003,hairmat,2)
        for s in [-1,1]:
            loop('Brass spectacles rim',(s*.049,-.07,z+.126),.029,brass,wire=.004)
            glass=material('Smoky spectacle glass',(.18,.17,.12),.2,.25)
            bone('Round spectacle lens',(s*.049,-.068,z+.126),(s*.049,-.071,z+.126),.026,.026,glass,32)
        tube('Spectacles bridge',[(-.02,-.075,z+.132),(0,-.083,z+.14),(.02,-.075,z+.132)],.0035,brass)
    elif kind=='medium':
        # Waved bob, open over the face, with an asymmetric silver forelock.
        for s in [-1,1]:
            for i in range(5):
                x=s*(.085+.006*math.sin(i));zz=z+.092-i*.04
                ell('Bob wave',(x,.009,zz),(.046,.087,.037),hairmat)
                tube('Sculpted bob crest',[(x,-.077,zz+.026),(x+s*.02,-.065,zz),(x,-.066,zz-.025)],.006,accent if s==-1 else hairmat)
        ell('Back of bob',(0,.055,z+.038),(.104,.065,.102),hairmat)
        for i in range(12):
            x=-.075+i*.013
            tube('Swept forelock',[(x,.029,z+.137),(x-.018,-.034,z+.149),(x-.025,-.079,z+.123)],.008,accent if i<6 else hairmat)
        for s in [-1,1]:
            bone('Gold earring chain',(s*.112,-.01,z-.049),(s*.115,-.013,z-.081),.002,.002,brass)
            ell('Plum earring',(s*.115,-.014,z-.086),(.010,.006,.014),accent if False else material('Garnet earrings',(.18,.013,.045),.32,.1),16,12)
    else:
        # Bald head with beard following the jaw and mouth, leaving the scalp bare.
        for s in [-1,1]:
            for i in range(6):
                zz=z-.055-i*.013;x=s*(.074-i*.006)
                ell('Short beard clump',(x,-.072-i*.004,zz),(.023,.015,.020),hairmat,16,10)
        ell('Beard chin',(0,-.077,z-.131),(.052,.026,.026),hairmat)
        for s in [-1,1]:
            tube('Moustache',[(s*.005,-.113,z-.06),(s*.021,-.111,z-.059),(s*.036,-.099,z-.066)],.006,hairmat)

def hands(wrist,height,skinmat,size=1):
    palms={}
    for s,anatomical in [(-1,'r'),(1,'l')]:
        p=wrist+.045*size
        bone('Wrist '+anatomical,(s*(wrist-.012),.025,height),(s*(p-.016),.025,height),.028*size,.031*size,skinmat)
        ell('Palm '+anatomical,(s*p,.025,height),(.051*size,.037*size,.020*size),skinmat)
        for i,(dy,length) in enumerate([(-.027,.074),(-.008,.088),(.012,.081),(.03,.065)]):
            y=.025+dy*size;start=p+.036*size;end=start+length*size
            a=(s*start,y,height);b=(s*(start+length*.54*size),y,height-.002);c=(s*end,y,height-.006)
            bone('Finger '+anatomical,a,b,.009*size,.0077*size,skinmat,14)
            ell('Knuckle '+anatomical,b,(.009*size,.009*size,.009*size),skinmat,16,10)
            bone('Fingertip '+anatomical,b,c,.0077*size,.0065*size,skinmat,14)
            ell('Finger pad '+anatomical,c,(.0075*size,.0065*size,.007*size),skinmat,16,10)
        a=(s*(p-.01),-.005,height-.004);b=(s*(p+.008*size),-.037*size,height-.012);c=(s*(p+.039*size),-.045*size,height-.015)
        bone('Thumb '+anatomical,a,b,.012*size,.010*size,skinmat)
        bone('Thumb tip '+anatomical,b,c,.010*size,.008*size,skinmat)
        ell('Thumb pad '+anatomical,c,(.009*size,.008*size,.008*size),skinmat)
        palms[anatomical]=(s*p,.025,height-.012)
    return palms

def bootlegs(width,hipheight,kneeheight,bootheight,trousers,boots,chunk=1):
    parts=[]
    for s in [-1,1]:
        x=s*width
        ell('Boot sole',(x,-.052,.049),(.070*chunk,.13*chunk,.035),sole)
        ell('Boot foot',(x,-.067,.095),(.067*chunk,.113*chunk,.056),boots)
        ell('Boot shaft',(x,.006,bootheight*.60),(.059*chunk,.07*chunk,bootheight*.42),boots)
        box('Boot heel',(x,.067,.042),(.09*chunk,.07*chunk,.065),sole,.012)
        parts.append(bone('Trouser calf',(x,.025,bootheight*.88),(x,.03,kneeheight),.066*chunk,.083*chunk,trousers))
        parts.append(bone('Trouser thigh',(x,.03,kneeheight),(x*.84,.025,hipheight),.083*chunk,.103*chunk,trousers))
        parts.append(ell('Trouser knee',(x,.03,kneeheight),(.085*chunk,.09*chunk,.11),trousers))
        for i in range(7):
            z=.15+i*(bootheight-.15)/7
            tube('Crossed boot lace',[(x-.025*chunk,-.063,z),(x+.025*chunk,-.066,z+.021)],.0025,leatheredge,1)
            tube('Crossed boot lace',[(x+.025*chunk,-.063,z),(x-.025*chunk,-.066,z+.021)],.0025,leatheredge,1)
    return parts

def empty(name,parent,location=(0,0,0),rotation=None,role='attachment'):
    ob=bpy.data.objects.new(name,None);character.objects.link(ob)
    ob.empty_display_type='ARROWS';ob.empty_display_size=.07;ob.parent=parent;ob.location=location
    if rotation is not None:
        ob.rotation_mode='QUATERNION';ob.rotation_quaternion=rotation.to_quaternion()
    ob['role']=role;ob['units']='metres';ob['attachment_id']=name;return ob

def frame(forward,up=(0,0,1)):
    x=Vector(forward).normalized();y=Vector(up).cross(x).normalized();z=x.cross(y).normalized()
    return Matrix((x,y,z)).transposed()

def root_for(id):
    root=bpy.data.objects.new(id+'_root',None);character.objects.link(root)
    for ob in list(character.objects):
        if ob!=root:ob.parent=root
    root['status']='PROVISIONAL static concept; no animation rig or Unreal integration'
    root['units']='metres'
    return root

def character_sockets(root,palms,hip,back,chest,fx=False):
    rot=frame((0,-1,0))
    for side,pos in palms.items():
        ob=empty('SOCKET_hand_'+side,root,pos,rot)
        ob['future_bone']='hand_'+side
        ob=empty('SOCKET_fx_palm_'+side,root,(pos[0],pos[1],pos[2]+.045),rot,'effect')
        ob['future_bone']='hand_'+side
    for side,s in [('r',-1),('l',1)]:
        ob=empty('SOCKET_holster_hip_'+side,root,(s*hip[0],hip[1],hip[2]),rot)
        ob['future_bone']='pelvis';ob['usage']='reserve utility attachment'
    ob=empty('SOCKET_stow_back',root,back,frame((-.48,0,.877),(-.877,0,-.48)))
    ob['future_bone']='spine_03'
    ob=empty('SOCKET_stow_chest',root,chest,rot);ob['future_bone']='spine_03'

def studio(height=1.9,width=2.3,item=False):
    scene=bpy.context.scene
    for ob in list(stage.objects):bpy.data.objects.remove(ob,do_unlink=True)
    m=material('Studio floor',(.034,.042,.04),.9)
    bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,.004 if not item else -.25))
    floor=move(bpy.context.object,stage);floor.name='Studio ground';floor.data.materials.append(m)
    scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.07,.085,.08,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.4
    def aim(ob,point):ob.rotation_euler=(Vector(point)-ob.location).to_track_quat('-Z','Y').to_euler()
    for name,loc,power,color,size in [('Key',(-3,-4,5),450,(1,.83,.67),4),('Fill',(2,-3,3),190,(.79,.89,1),3),('Rim',(2,2,3),500,(.37,.78,.72),2.5)]:
        data=bpy.data.lights.new(name,'AREA');data.energy=power;data.color=color;data.size=size
        ob=bpy.data.objects.new(name,data);stage.objects.link(ob);ob.location=loc;aim(ob,(0,0,height/2))
    data=bpy.data.cameras.new('Camera');cam=bpy.data.objects.new('Camera',data);stage.objects.link(cam)
    cam.location=(0,-6,height*.60) if not item else (1,-2,1)
    aim(cam,(0,0,height*.51) if not item else (.03,0,0));data.type='ORTHO';data.ortho_scale=width;scene.camera=cam
    scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True
    scene.render.resolution_x=1300;scene.render.resolution_y=1300 if not item else 1000
    scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG'
    scene.view_settings.view_transform='AgX'
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=='VIEW_3D':
                area.spaces.active.region_3d.view_location=(0,0,height*.5) if not item else (0,0,0)
                area.spaces.active.region_3d.view_distance=width*1.5
                area.spaces.active.region_3d.view_rotation=cam.rotation_euler.to_quaternion()
                area.spaces.active.shading.color_type='MATERIAL'

def save_export(id,root,ratio=.3):
    bpy.ops.object.select_all(action='DESELECT');root.select_set(True);bpy.context.view_layer.objects.active=root
    bpy.context.view_layer.update()
    locators={ob.name:{'matrix_local_blender': [list(row) for row in ob.matrix_local], 'role':ob.get('role'),'future_bone':ob.get('future_bone')} for ob in character.objects if ob.type=='EMPTY' and ob!=root}
    (OUT/(id+'-locators.json')).write_text(json.dumps(locators,indent=2)+'\n')
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(id+'.blend')))
    # Convert just geometry. Preserve attachment empties as actual glTF nodes.
    parts=[ob for ob in character.objects if ob.type in {'MESH','CURVE'}]
    bpy.ops.object.select_all(action='DESELECT')
    for ob in parts:ob.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.convert(target='MESH');bpy.ops.object.join()
    ob=bpy.context.object;ob.name=id+'_mesh'
    mod=ob.modifiers.new('Static concept reduction','DECIMATE');mod.ratio=ratio
    bpy.ops.object.modifier_apply(modifier=mod.name)
    for ob in character.objects:ob.select_set(True)
    bpy.ops.export_scene.gltf(filepath=str(OUT/(id+'.glb')),export_format='GLB',use_selection=True,export_apply=True,export_extras=True,export_cameras=False,export_lights=False)
    # Render the actual exported geometry/materials after a clean import.
    for ob in list(character.objects):bpy.data.objects.remove(ob,do_unlink=True)
    bpy.ops.import_scene.gltf(filepath=str(OUT/(id+'.glb')))
    bpy.context.scene.render.filepath=str(OUT/(id+'-preview.png'))
    bpy.ops.render.render(write_still=True)
    print('ASSET_COMPLETE',id)
