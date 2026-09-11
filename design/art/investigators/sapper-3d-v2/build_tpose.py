"""Create a neutral T-pose and independently centred carbine from v1 source.

Uses the original procedural model as an immutable source and writes only v2.
Run with Blender 5.0 in background mode.
"""
from pathlib import Path

SOURCE = Path(__file__).resolve().parent.parent / 'sapper-3d-v1' / 'build_sapper.py'
code = SOURCE.read_text(encoding='utf-8')

def replace_section(text, start, end, replacement):
    assert text.count(start)==1 and text.count(end)==1
    a=text.index(start); b=text.index(end,a)
    return text[:a]+replacement+'\n\n'+text[b:]

# Symmetric stance and forward-facing eyes.
code=code.replace("[('Left',-.155),('Right',.17)]", "[('Left',-.16),('Right',.16)]")
code=code.replace("y=.035 if side=='Right' else -.025", "y=0")
code=code.replace("knee=(x+(.015 if x<0 else -.005),.045,.63)", "knee=(x,.025,.63)")
code=code.replace("s*.047-.006", "s*.047")
code=code.replace("u=-.068)","u=-.037)")
code=code.replace("u=.022)","u=.018)")
code=code.replace("[gp(-.234,0,.018),(-.10,-.287,.779),(.16,-.279,.645),gp(.296,0,.018)]", "[gp(-.234,-.028,.018),gp(-.14,-.105,.025),gp(.13,-.15,.025),gp(.296,-.024,.018)]")
code=code.replace('# Visible coat seams and restrained sculpted garment folds.', "bone('Front sling swivel',gp(.296,-.009,.008),gp(.296,-.025,.018),.004,.004,metal,16)\n\n# Visible coat seams and restrained sculpted garment folds.")

ARMS='''# Neutral T-pose: shoulders, elbows and wrists share one horizontal axis.
arm_height=1.449
for name,s in [('Listening',1),('Carbine',-1)]:
    shoulder=(s*.248,.025,arm_height)
    elbow=(s*.525,.025,arm_height)
    wrist=(s*.785,.025,arm_height)
    ell(name+' shoulder bridge',(s*.232,.025,arm_height),(.145,.128,.125),coat)
    bone(name+' upper sleeve',elbow,shoulder,.093,.125,coat)
    ell(name+' sleeve elbow',elbow,(.101,.098,.098),coat)
    bone(name+' forearm sleeve',wrist,elbow,.063,.093,coat)
    bone(name+' broad cuff',(s*.724,.025,arm_height),(s*.794,.025,arm_height),.074,.073,edge)
    for t in [.3,.52,.74]:
        x=s*(.525+t*.26)
        r=.093-t*.025
        pts=[(x+.005*math.cos(a),.025+r*math.cos(a),arm_height+r*math.sin(a)) for a in [j*math.pi*2/32 for j in range(33)]]
        tube(name+' tailored forearm fold',pts,.0045,coat,2)
    box(name+' cuff tab',(s*.759,-.05,arm_height),(.042,.012,.034),coat,.006)
    bone(name+' cuff button',(s*.763,-.054,arm_height),(s*.763,-.065,arm_height),.010,.010,brass,12)
'''
code=replace_section(code,'# Posed sleeves:', '# Face built', ARMS)
code=code.replace("upper_names=['Coat torso'", "upper_names=['Listening shoulder bridge','Carbine shoulder bridge','Coat torso'")
code=code.replace("(s*.224,.012,1.524)","(s*.224,.012,1.576)")
code=code.replace("(s*.193,-.001,1.533),(s*.193,-.001,1.54)","(s*.193,-.001,1.585),(s*.193,-.001,1.592)")

HANDS='''# Flat open hands, palms down; fingers relaxed and separated.
for name,s in [('Right',1),('Left',-1)]:
    z=1.449
    bone(name+' neutral wrist',(s*.776,.025,z),(s*.825,.025,z),.034,.033,skin)
    ell(name+' open palm',(s*.842,.025,z),(.054,.040,.022),skin)
    # The index finger is nearest the thumb, towards the front of the model.
    for i,(y,length) in enumerate([(-.004,.084),(.016,.095),(.037,.089),(.057,.071)]):
        start=(s*.88,y,z)
        middle=(s*(.88+length*.53),y-.001,z-.001)
        tip=(s*(.88+length),y-.002,z-.005)
        bone(name+' finger proximal '+str(i),start,middle,.0095,.008,skin,16)
        ell(name+' finger knuckle '+str(i),middle,(.009,.0087,.0087),skinlight,16,10)
        bone(name+' finger distal '+str(i),middle,tip,.008,.0065,skin,16)
        ell(name+' fingertip '+str(i),tip,(.0075,.0065,.0065),skin,16,10)
        ell(name+' fingernail '+str(i),(tip[0]-s*.007,tip[1],tip[2]+.006),(.008,.0045,.0015),skinlight,16,8)
    a=(s*.833,-.001,z-.003);b=(s*.86,-.041,z-.011);c=(s*.893,-.057,z-.016)
    bone(name+' thumb base',a,b,.014,.011,skin)
    ell(name+' thumb joint',b,(.012,.011,.010),skin)
    bone(name+' thumb tip',b,c,.011,.008,skin)
    ell(name+' thumb pad',c,(.009,.008,.008),skin)
    tube(name+' palm crease',[(s*.828,-.006,z-.02),(s*.843,.012,z-.024),(s*.861,.026,z-.019)],.001,lip,1)
'''
code=replace_section(code,'# Listening hand:', '# Belt, diagonal', HANDS)

# Capture only the gun-specific objects, never the character's wrist/hand.
code=code.replace('# One mechanically coherent carbine along a single diagonal local axis.', '''# Record the weapon boundary before creating its components.
before_gun=set(character.objects)
# One mechanically coherent carbine along a single diagonal local axis.''')
code=code.replace('# Visible coat seams and restrained sculpted garment folds.', '''# Separate and recenter the weapon: muzzle +X, top +Z, receiver at origin.
from mathutils import Matrix
gun=bpy.data.collections.new('Carbine | separate asset')
scene.collection.children.link(gun)
side=cross.cross(axis).normalized()
weapon_transform=Matrix([axis,side,cross]).to_4x4() @ Matrix.Translation(-gun_origin)
weapon_parts=[ob for ob in character.objects if ob not in before_gun]
for ob in weapon_parts:
    move(ob,gun)
    ob.matrix_world=weapon_transform @ ob.matrix_world
gun_root=bpy.data.objects.new('Carbine_Root',None);gun.objects.link(gun_root)
gun_root['forward_axis']='+X';gun_root['up_axis']='+Z';gun_root['origin']='receiver / grip region'
for ob in weapon_parts:ob.parent=gun_root
gun.hide_render=True
# Visible coat seams and restrained sculpted garment folds.''')
code=code.replace("[(side*.289,-.081,1.409-j*.016),(side*.319,-.085,1.395-j*.016),(side*.341,-.069,1.411-j*.016)]", "[(side*(.35+j*.016),-.085,1.476),(side*(.366+j*.016),-.089,1.449),(side*(.35+j*.016),-.085,1.422)]")
code=code.replace("'Continuous upper greatcoat and posed sleeves'", "'Continuous greatcoat | horizontal T-pose sleeves'")
code=code.replace("'PROVISIONAL stylized static concept; not rigged or production game-ready'", "'PROVISIONAL neutral T-pose, empty open hands; unrigged static concept'")

# Use the original lighting, but a direct front camera that shows arm symmetry.
code=code[:code.index('# Export geometry only, triangulating evaluated copies')]
code+='''
cam.location=(0,-6,1.16);aim(cam,(0,0,.99));camdata.ortho_scale=2.25
scene.render.resolution_x=1500;scene.render.resolution_y=1500
scene.cycles.samples=40
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_location=(0,0,.98)
            area.spaces.active.region_3d.view_distance=3.6
            area.spaces.active.region_3d.view_rotation=cam.rotation_euler.to_quaternion()
            area.spaces.active.shading.color_type='MATERIAL'

def export_parts(collection,root,name,ratio):
    bpy.ops.object.select_all(action='DESELECT')
    parts=[o for o in collection.objects if o.type in {'MESH','CURVE'}]
    for ob in parts:ob.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.convert(target='MESH')
    bpy.ops.object.join()
    mesh=bpy.context.object;mesh.name=name
    mod=mesh.modifiers.new('Portable mesh reduction','DECIMATE');mod.ratio=ratio
    bpy.ops.object.modifier_apply(modifier=mod.name)
    root.select_set(True)
    bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,export_apply=True,export_cameras=False,export_lights=False,export_extras=True)

# Temporary combined scene lets us create two truly independent Blender files.
scratch=OUT.parents[3]/'Saved'/'Art'/'Sapper'
scratch.mkdir(parents=True,exist_ok=True)
master=scratch/'sapper-v2-working.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(master))

# Character deliverables contain no gun, sling, or weapon hierarchy.
for ob in list(gun.objects):bpy.data.objects.remove(ob,do_unlink=True)
bpy.data.collections.remove(gun)
bpy.ops.object.select_all(action='DESELECT');root.select_set(True);bpy.context.view_layer.objects.active=root
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'sapper-tpose.blend'))
scene.render.filepath=str(OUT/'sapper-tpose-preview.png');bpy.ops.render.render(write_still=True)
export_parts(character,root,'sapper-tpose',.14)

# Independent gun scene and model, preserved at real scale and centred on grip.
bpy.ops.wm.open_mainfile(filepath=str(master))
scene=bpy.context.scene;cam=scene.camera;camdata=cam.data
character=bpy.data.collections['Sapper | character meshes']
for ob in list(character.objects):bpy.data.objects.remove(ob,do_unlink=True)
bpy.data.collections.remove(character)
gun=bpy.data.collections['Carbine | separate asset'];gun.hide_render=False
gun_root=bpy.data.objects['Carbine_Root']
bpy.data.objects['Studio ground'].location.z=-.26
cam.location=(.07,-2,.65);aim(cam,(.07,0,-.035));camdata.ortho_scale=.92
scene.render.resolution_x=1500;scene.render.resolution_y=900
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_location=(.07,0,0)
            area.spaces.active.region_3d.view_distance=1.2
            area.spaces.active.region_3d.view_rotation=cam.rotation_euler.to_quaternion()
bpy.ops.object.select_all(action='DESELECT');gun_root.select_set(True);bpy.context.view_layer.objects.active=gun_root
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'sapper-carbine.blend'))
scene.render.filepath=str(OUT/'sapper-carbine-preview.png');bpy.ops.render.render(write_still=True)
export_parts(gun,gun_root,'sapper-carbine',.5)
print('TPOSE_AND_SEPARATE_CARBINE_COMPLETE',OUT)
'''
exec(compile(code, str(SOURCE), 'exec'), globals())
