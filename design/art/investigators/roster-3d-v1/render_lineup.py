import bpy
import sys
from pathlib import Path
from mathutils import Vector
out=Path(__file__).resolve().parent
sys.path.insert(0,str(out))
from attachment_tools import import_asset
bpy.ops.wm.open_mainfile(filepath=str(out/'sapper.blend'))
for ob in list(bpy.data.objects):
    if not any(col.name=='Presentation | not exported' for col in ob.users_collection):bpy.data.objects.remove(ob,do_unlink=True)
for name,x in [('sapper',-3.3),('photographer',-1.1),('medium',1.1),('smuggler',3.3)]:
    item=import_asset(name);item['root'].location.x=x
scene=bpy.context.scene;cam=scene.camera
cam.location=(0,-12,2.65);cam.rotation_euler=(Vector((0,0,1.02))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.ortho_scale=9.05
scene.render.resolution_x=2400;scene.render.resolution_y=760;scene.cycles.samples=32
bpy.ops.wm.save_as_mainfile(filepath=str(out/'examples'/'roster-lineup.blend'))
scene.render.filepath=str(out/'roster-lineup.png');bpy.ops.render.render(write_still=True)
