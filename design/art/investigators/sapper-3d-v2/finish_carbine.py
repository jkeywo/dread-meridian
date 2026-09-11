"""Apply the final small attachment corrections without rebuilding the Sapper.

These same corrections are included in build_tpose.py for future rebuilds.
"""
import bpy
from mathutils import Vector
from pathlib import Path
out=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(out/'sapper-carbine.blend'))
gun=bpy.data.collections['Carbine | separate asset']
root=bpy.data.objects['Carbine_Root']
sight=bpy.data.objects['Front sight'];sight.location.z=.018
a=Vector((.296,-.008,-.009));b=Vector((.296,-.018,-.025))
bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=.004,depth=(b-a).length,location=(a+b)/2)
obj=bpy.context.object;obj.name='Front sling swivel'
for col in list(obj.users_collection):col.objects.unlink(obj)
gun.objects.link(obj);obj.parent=root
obj.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler()
obj.data.materials.append(bpy.data.materials['Worn gunmetal'])
for poly in obj.data.polygons:poly.use_smooth=True
bpy.ops.wm.save_as_mainfile(filepath=str(out/'sapper-carbine.blend'))
bpy.context.scene.render.filepath=str(out/'sapper-carbine-preview.png')
bpy.ops.render.render(write_still=True)
bpy.ops.object.select_all(action='DESELECT')
parts=[o for o in gun.objects if o.type in {'MESH','CURVE'}]
for obj in parts:obj.select_set(True)
bpy.context.view_layer.objects.active=parts[0]
bpy.ops.object.convert(target='MESH');bpy.ops.object.join()
obj=bpy.context.object;obj.name='sapper-carbine'
mod=obj.modifiers.new('Portable mesh reduction','DECIMATE');mod.ratio=.5
bpy.ops.object.modifier_apply(modifier=mod.name)
root.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(out/'sapper-carbine.glb'),export_format='GLB',use_selection=True,export_apply=True,export_cameras=False,export_lights=False,export_extras=True)
