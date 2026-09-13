"""Normalize the original supplied FBX; reuse the accepted packed textures."""
import bpy,numpy as np,sys
from pathlib import Path
P=Path(__file__).resolve().parents[3];out=P/'Saved/SmugglerAnimationProbe';out.mkdir(parents=True,exist_ok=True)
source=Path(sys.argv[sys.argv.index('--')+1]);assert source.is_file()
bpy.ops.wm.open_mainfile(filepath=str(P/'design/art/supplied-characters-v1/smuggler-rigged.blend'))
mat=next(o for o in bpy.data.objects if o.type=='MESH').data.materials[0];mat.use_fake_user=True
for o in list(bpy.data.objects):bpy.data.objects.remove(o,do_unlink=True)
bpy.ops.import_scene.fbx(filepath=str(source))
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];bpy.ops.object.select_all(action='DESELECT')
for o in meshes:o.select_set(True)
bpy.context.view_layer.objects.active=meshes[0];bpy.ops.object.join();mesh=bpy.context.object
mw=mesh.matrix_world.copy();mesh.parent=None;mesh.matrix_world=mw;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
coords=np.array([v.co[:] for v in mesh.data.vertices]);lo=coords.min(0);hi=coords.max(0)
coords=(coords-np.array([(lo[0]+hi[0])/2,0,lo[2]]))*(2.08/(hi[2]-lo[2]));span=max(abs(coords[:,0]));arm=coords[abs(coords[:,0])>span*.5];coords[:,1]+=.025-float(np.median(arm[:,1]))
mesh.data.vertices.foreach_set('co',coords.astype(np.float32).ravel());mesh.data.materials.clear();mesh.data.materials.append(mat)
for p in mesh.data.polygons:p.material_index=0
bpy.ops.export_scene.gltf(filepath=str(out/'smuggler.glb'),export_format='GLB',use_selection=True)
