import bpy,sys
from pathlib import Path
from mathutils import Vector
p=Path(__file__).resolve().parent
asset=sys.argv[sys.argv.index('--')+1]
bpy.ops.wm.open_mainfile(filepath=str(p/(asset+'-rigged.blend')))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
with bpy.data.libraries.load(str(p.parent/'roster-3d-v1'/(asset+'.blend')),link=False) as (src,dst):
    dst.collections=[n for n in src.collections if 'Presentation' in n]
for col in dst.collections:bpy.context.scene.collection.children.link(col)
scene=bpy.context.scene
cam=next(o for o in scene.objects if o.type=='CAMERA');scene.camera=cam
cam.location=(2.8,-6,2.4);cam.rotation_euler=(Vector((0,0,1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=2.4
scene.world=bpy.data.worlds.new('Studio');scene.world.color=(.08,.08,.08)
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.resolution_x=900;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
poses=[('Manny_Walk',9,'walk'),('T_Pose',1,'tpose'),('Manny_Idle',1,'idle')]
if '--quick' in sys.argv:poses=poses[:1]
for action,frame,suffix in poses:
    rig.animation_data_create();rig.animation_data.action=bpy.data.actions[action];rig.animation_data.action_slot=bpy.data.actions[action].slots[0]
    scene.frame_set(frame);bpy.context.view_layer.update()
    target=rig.matrix_world@rig.pose.bones['pelvis'].head;target.z=1.0
    cam.location=target+Vector((2.8,-6,1.4));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale=2.7
    scene.render.filepath=str(p/(asset+'-'+suffix+'.png'));bpy.ops.render.render(write_still=True)
print('RENDER_COMPLETE',asset)

