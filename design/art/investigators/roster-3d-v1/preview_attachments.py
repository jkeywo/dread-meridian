"""Create a review scene from the GLBs with the chosen attachment state.

Example: blender -b --python preview_attachments.py -- photographer stowed back
"""
import sys
import json
import bpy
from pathlib import Path
from mathutils import Vector
out=Path(__file__).resolve().parent
sys.path.insert(0,str(out))
from attachment_tools import import_asset,snap,set_visible
args=sys.argv[sys.argv.index('--')+1:]
name,state=args[:2];view=args[2] if len(args)>2 else 'front'
manifest=json.loads((out/'attachments.json').read_text())
spec=manifest['characters'][name]
bpy.ops.wm.open_mainfile(filepath=str(out/(name+'.blend')))
bpy.context.preferences.filepaths.save_version=0
# Retain the studio, but use the actual exported character and item assets.
for ob in list(bpy.data.objects):
    if not any(col.name=='Presentation | not exported' for col in ob.users_collection):
        bpy.data.objects.remove(ob,do_unlink=True)
character=import_asset(name)
for mount in spec['states'][state]:
    item=import_asset(mount['item'])
    if mount.get('visibility')=='hidden':set_visible(item,False)
    else:snap(character,item,mount['socket'],mount['anchor'])
scene=bpy.context.scene
cam=scene.camera
height=2.0 if name=='smuggler' else (1.91 if name=='sapper' else 1.81)
cam.location=(3.1,6,2.65) if view=='back' else (3.1,-6,2.65)
cam.rotation_euler=(Vector((0,0,height*.52))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.ortho_scale=2.55 if name!='smuggler' else 2.75
scene.render.resolution_x=1300;scene.render.resolution_y=1300;scene.cycles.samples=28
stem=name+'-'+state+'-'+view
(out/'examples').mkdir(exist_ok=True)
scene['attachment_state']=name+':'+state
scene['note']='Static T-pose mounting demonstration. Fingers and secondary hands are not animated into grips.'
bpy.ops.wm.save_as_mainfile(filepath=str(out/'examples'/(stem+'.blend')))
scene.render.filepath=str(out/'examples'/(stem+'.png'))
bpy.ops.render.render(write_still=True)
print('ATTACHMENT_EXAMPLE_COMPLETE',stem)
