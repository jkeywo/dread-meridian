"""Inspect GLB data, then verify it imports and renders in a fresh scene."""
import bpy
import json
import struct
from pathlib import Path
from mathutils import Vector

out=Path(__file__).resolve().parent
raw=(out/'sapper-concept.glb').read_bytes()
magic,version,length=struct.unpack_from('<III',raw,0)
assert magic==0x46546c67 and version==2 and length==len(raw)
json_len,json_type=struct.unpack_from('<II',raw,12)
assert json_type==0x4e4f534a
doc=json.loads(raw[20:20+json_len])
assert doc.get('meshes') and doc.get('materials')
assert not doc.get('cameras') and not doc.get('skins') and not doc.get('animations')
triangles=0
for mesh in doc['meshes']:
    for primitive in mesh['primitives']:
        assert primitive.get('mode',4)==4
        triangles+=doc['accessors'][primitive['indices']]['count']//3
bpy.ops.wm.open_mainfile(filepath=str(out/'sapper-concept.blend'))
for ob in list(bpy.data.collections['Sapper | character meshes'].objects):
    bpy.data.objects.remove(ob,do_unlink=True)
bpy.ops.import_scene.gltf(filepath=str(out/'sapper-concept.glb'))
imported=[ob for ob in bpy.context.selected_objects if ob.type=='MESH']
assert imported
corners=[ob.matrix_world@Vector(v) for ob in imported for v in ob.bound_box]
minimum=[min(v[i] for v in corners) for i in range(3)]
maximum=[max(v[i] for v in corners) for i in range(3)]
report={'format':'glTF 2.0 binary','import_verified':True,'triangles':triangles,'meshes':len(doc['meshes']),'materials':len(doc['materials']),'bounds_min_m':minimum,'bounds_max_m':maximum,'rigged':False,'animated':False,'stage_in_export':False,'notes':['Static stylized concept interpretation, not a reconstructed scan.','Procedural micro-bump in Blender is not baked into GLB. GLB uses base PBR materials.','No Unreal integration, production retopology, LODs, or game runtime validation.']}
(out/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
bpy.context.scene.cycles.samples=32
bpy.context.scene.render.filepath=str(out/'sapper-glb-preview.png')
bpy.ops.render.render(write_still=True)
print(json.dumps(report))
