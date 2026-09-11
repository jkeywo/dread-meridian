"""Close-up review of the four revised female rigs, using delivered geometry."""
import bpy
from pathlib import Path
from mathutils import Vector
p=Path(__file__).resolve().parent
players=p.parents[2]/'investigators'/'roster-rigged-v1'
bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene
for i,(asset,folder) in enumerate([('lookout',p),('bomber',p),('photographer',players),('medium',players)]):
    old=set(bpy.data.actions)
    with bpy.data.libraries.load(str(folder/(asset+'-rigged.blend')),link=False) as (src,dst):dst.objects=src.objects;dst.actions=src.actions
    for ob in dst.objects:scene.collection.objects.link(ob)
    rig=next(o for o in dst.objects if o.type=='ARMATURE')
    action=next(a for a in set(bpy.data.actions)-old if a.name.startswith('Manny_Idle'))
    rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0]
    group=bpy.data.objects.new(asset+'_stage',None);scene.collection.objects.link(group);group.location=((i-1.5)*.68,0,0);rig.parent=group
scene.frame_set(1);bpy.context.view_layer.update()
with bpy.data.libraries.load(str(p/'source'/'gunman.blend'),link=False) as (src,dst):dst.collections=[n for n in src.collections if 'Presentation' in n]
for col in dst.collections:scene.collection.children.link(col)
scene.world=bpy.data.worlds.new('Studio');scene.world.color=(.15,.15,.15)
cam=next(o for o in scene.objects if o.type=='CAMERA');scene.camera=cam
cam.location=(0,-7,1.65);cam.rotation_euler=(Vector((0,0,1.40))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=2.78
for ob in scene.objects:
    if ob.type=='LIGHT':ob.data.energy*=1.7
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.view_settings.view_transform='AgX';scene.render.resolution_x=2400;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(p/'female-character-refinements.png')
bpy.ops.render.render(write_still=True)
