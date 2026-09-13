import bpy,json,math,struct,sys
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parent;reports=json.loads((P/'blender-validation.json').read_text()) if (P/'blender-validation.json').exists() else {}
only=sys.argv[sys.argv.index('--')+1] if '--' in sys.argv else None
for asset in json.loads((P/'manifest.json').read_text()):
    if only and asset!=only:continue
    bpy.ops.wm.open_mainfile(filepath=str(P/f'{asset}-rigged.blend'));bpy.context.preferences.filepaths.save_version=0
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    assert all(v.groups and abs(sum(g.weight for g in v.groups)-1)<1e-5 for v in mesh.data.vertices)
    sockets=[o for o in bpy.context.scene.objects if o.name.startswith('SOCKET_')]
    assert len(sockets)>=8 and all(o.parent==rig and o.parent_bone in rig.data.bones for o in sockets)
    maxradius=0;samples=0
    for name in ['Manny_Idle','Manny_Walk','T_Pose']:
        action=bpy.data.actions[name];rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0]
        for fraction in [0,.25,.5,.75,1]:
            bpy.context.scene.frame_set(int(action.frame_range[0]+fraction*(action.frame_range[1]-action.frame_range[0])))
            ev=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get());origin=rig.matrix_world@rig.pose.bones['pelvis'].head
            coords=[ev.matrix_world@v.co for v in ev.data.vertices]
            assert all(math.isfinite(v) for co in coords for v in co)
            radius=max((co-origin).length for co in coords);assert radius<3,(asset,name,radius);maxradius=max(maxradius,radius);samples+=1
    rig.animation_data.action=bpy.data.actions['T_Pose'];rig.animation_data.action_slot=rig.animation_data.action.slots[0];bpy.context.scene.frame_set(1)
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.gltf(filepath=str(P/f'{asset}-rigged.glb'),export_format='GLB',use_selection=True,export_extras=True,export_animations=True,export_animation_mode='ACTIONS',export_all_influences=True)
    for im in bpy.data.images:
        if im.size[0] and im.type=='IMAGE':im.pack()
    bpy.ops.wm.save_as_mainfile(filepath=str(P/f'{asset}-rigged.blend'))
    raw=(P/f'{asset}-rigged.glb').read_bytes();size=struct.unpack_from('<I',raw,12)[0];d=json.loads(raw[20:20+size]);assert len(d['skins'])==1 and len(d['animations'])==3 and len(d.get('textures',[]))>=1
    assert all('TEXCOORD_0' in pr['attributes'] and 'WEIGHTS_0' in pr['attributes'] for m in d['meshes'] for pr in m['primitives'])
    reports[asset]={'passed':True,'sampled_poses':samples,'maximum_radius_m':maxradius,'joints':len(d['skins'][0]['joints']),'textures':len(d['textures']),'sockets':len(sockets),'animations':[a['name'] for a in d['animations']]}
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=12;scene.cycles.use_denoising=True
    scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.25,.25,.25,1)
    scene.render.resolution_x=700;scene.render.resolution_y=700;scene.render.resolution_percentage=100;scene.render.film_transparent=True;scene.render.image_settings.color_mode='RGBA'
    bpy.ops.object.camera_add();cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=2.55;scene.camera=cam
    for loc,power in [((2,-4,4),450),((-3,-2,2),220),((0,3,3),300)]:
        bpy.ops.object.light_add(type='AREA',location=loc);l=bpy.context.object;l.data.energy=power;l.data.size=3;l.rotation_euler=(Vector((0,0,1))-l.location).to_track_quat('-Z','Y').to_euler()
    for name,frame,suffix in [('T_Pose',1,'tpose'),('Manny_Walk',9,'walk')]:
        rig.animation_data.action=bpy.data.actions[name];rig.animation_data.action_slot=rig.animation_data.action.slots[0];scene.frame_set(frame)
        cam.location=(0,-6,1) if suffix=='tpose' else (2,-6,2);cam.rotation_euler=(Vector((0,0,1))-cam.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath=str(P/f'{asset}-{suffix}.png');bpy.ops.render.render(write_still=True)
    print('FINISHED',asset,flush=True)
(P/'blender-validation.json').write_text(json.dumps(reports,indent=2))
