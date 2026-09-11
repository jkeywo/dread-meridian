"""Render delivered skinned geometry in T-pose and stock walk, with optional attachments."""
import bpy,sys,json
from pathlib import Path
from mathutils import Vector,Matrix
p=Path(__file__).resolve().parent
mode=sys.argv[sys.argv.index('--')+1] if '--' in sys.argv else 'T_Pose'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
roles=['gunman','bruiser','lookout','bomber','gang-boss']
manifest=json.loads((p/'attachments.json').read_text())
for i,asset in enumerate(roles):
    old=set(bpy.data.actions)
    with bpy.data.libraries.load(str(p/(asset+'-rigged.blend')),link=False) as (src,dst):dst.objects=src.objects;dst.actions=src.actions
    for ob in dst.objects:scene.collection.objects.link(ob)
    rig=next(o for o in dst.objects if o.type=='ARMATURE')
    prefix='Manny_Walk' if mode!='T_Pose' else mode
    action=next(a for a in set(bpy.data.actions)-old if a.name.startswith(prefix))
    rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0]
    group=bpy.data.objects.new(asset+'_stage',None);scene.collection.objects.link(group);group.location=((i-2)*(2.35 if mode=='T_Pose' else 1.48),0,0);rig.parent=group
    scene.frame_set(9 if mode!='T_Pose' else 1);bpy.context.view_layer.update()
    if mode=='stowed':
        for binding in manifest['characters'][asset]['states']['stowed']:
            before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=str(p/(binding['item']+'.glb')))
            obs=list(set(bpy.data.objects)-before)
            root=next(o for o in obs if o.parent is None)
            anchor=next(o for o in obs if o.get('attachment_id')==binding['anchor'])
            socket=next(o for o in dst.objects if o.get('attachment_id')==binding['socket'])
            local=root.matrix_world.inverted()@anchor.matrix_world
            root.parent=socket;root.matrix_parent_inverse=Matrix.Identity(4);root.matrix_basis=local.inverted()
with bpy.data.libraries.load(str(p/'source'/'gunman.blend'),link=False) as (src,dst):dst.collections=[n for n in src.collections if 'Presentation' in n]
for col in dst.collections:scene.collection.children.link(col)
scene.world=bpy.data.worlds.new('Studio');scene.world.color=(.12,.12,.12)
camera=next(o for o in scene.objects if o.type=='CAMERA');scene.camera=camera
camera.location=(.5,-15,3.8);target=Vector((0,0,1.0));camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=11.8 if mode=='T_Pose' else 8.2
for ob in scene.objects:
    if ob.type=='LIGHT':ob.data.energy*=3;ob.data.size=6
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.view_settings.view_transform='AgX'
scene.render.resolution_x=2600;scene.render.resolution_y=720 if mode=='T_Pose' else 900;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(p/('smugglers-'+mode.lower()+'.png'))
bpy.ops.render.render(write_still=True)
print('PREVIEW_COMPLETE',mode)
