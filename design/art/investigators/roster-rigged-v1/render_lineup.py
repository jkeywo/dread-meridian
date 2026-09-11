import bpy
from pathlib import Path
from mathutils import Vector
p=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
for asset,x in [('sapper',-2.1),('photographer',-.7),('medium',.7),('smuggler',2.1)]:
    old=set(bpy.data.actions)
    with bpy.data.libraries.load(str(p/(asset+'-rigged.blend')),link=False) as (src,dst):dst.objects=src.objects;dst.actions=src.actions
    for ob in dst.objects:scene.collection.objects.link(ob)
    rig=next(o for o in dst.objects if o.type=='ARMATURE')
    action=next(a for a in set(bpy.data.actions)-old if a.name.startswith('Manny_Walk'))
    rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0]
    group=bpy.data.objects.new(asset+'_stage',None);scene.collection.objects.link(group);group.location=(x,0,0);rig.parent=group
with bpy.data.libraries.load(str(p.parent/'roster-3d-v1'/'sapper.blend'),link=False) as (src,dst):dst.collections=[n for n in src.collections if 'Presentation' in n]
for col in dst.collections:scene.collection.children.link(col)
scene.world=bpy.data.worlds.new('Studio');scene.world.color=(.08,.08,.08)
camera=next(o for o in scene.objects if o.type=='CAMERA');scene.camera=camera
camera.location=(1,-12,4);target=Vector((0,-.6,1));camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=6.6
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.resolution_x=2000;scene.render.resolution_y=850;scene.render.resolution_percentage=100
scene.frame_set(9)
for rig in [o for o in scene.objects if o.type=='ARMATURE']:
    # Compensate legacy reference-scene placement in the two unchanged male
    # previews; the revised female clips are already centred at the origin.
    rig.parent.location.y=-rig.location.y
bpy.context.view_layer.update()
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(p/'rigged-roster.png')
bpy.ops.render.render(write_still=True)
print('LINEUP_COMPLETE')

