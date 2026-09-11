import bpy
from pathlib import Path
from mathutils import Vector
p=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene
files=sorted((p/'equipped').glob('*-equipped.blend'))
for i,path in enumerate(files):
    with bpy.data.libraries.load(str(path),link=False) as (src,dst):
        dst.objects=[n for n in src.objects if n not in ['Studio ground','Camera','Key','Fill','Rim']];dst.actions=src.actions
    group=bpy.data.objects.new(path.stem+'_stage',None);scene.collection.objects.link(group);group.location=((i-3)*1.22,0,0)
    for ob in dst.objects:
        scene.collection.objects.link(ob)
        if ob.parent is None:ob.parent=group
with bpy.data.libraries.load(str(p/'source'/'gunman.blend'),link=False) as (src,dst):dst.collections=[n for n in src.collections if 'Presentation' in n]
for col in dst.collections:scene.collection.children.link(col)
scene.world=bpy.data.worlds.new('Studio');scene.world.color=(.15,.15,.15)
camera=next(o for o in scene.objects if o.type=='CAMERA');scene.camera=camera
camera.location=(1,-12,3.4);camera.rotation_euler=(Vector((0,-.12,1))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=9.0
for ob in scene.objects:
    if ob.type=='LIGHT':ob.data.energy*=3;ob.data.size=7
scene.frame_set(1);scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.resolution_x=2800;scene.render.resolution_y=850;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.filepath=str(p/'equipped'/'equipped-lineup.png')
bpy.ops.render.render(write_still=True)
