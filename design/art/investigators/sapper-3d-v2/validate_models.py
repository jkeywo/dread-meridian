"""Check the independent GLBs and render each after a clean reimport."""
import bpy
import json
import struct
from pathlib import Path
from mathutils import Vector

out=Path(__file__).resolve().parent
reports=[]
for name,rootname,forbidden in [('sapper-tpose','Sapper_Concept_Root','Carbine_Root'),('sapper-carbine','Carbine_Root','Sapper_Concept_Root')]:
    raw=(out/(name+'.glb')).read_bytes()
    magic,version,total=struct.unpack_from('<III',raw)
    assert magic==0x46546c67 and version==2 and total==len(raw)
    size,kind=struct.unpack_from('<II',raw,12)
    assert kind==0x4e4f534a
    doc=json.loads(raw[20:20+size])
    names={n.get('name') for n in doc['nodes']}
    assert rootname in names and forbidden not in names
    assert not doc.get('skins') and not doc.get('animations') and not doc.get('cameras')
    triangles=sum(doc['accessors'][p['indices']]['count']//3 for m in doc['meshes'] for p in m['primitives'])
    bpy.ops.wm.open_mainfile(filepath=str(out/(name+'.blend')))
    for ob in list(bpy.data.objects):
        if not any(col.name=='Presentation | not exported' for col in ob.users_collection):
            bpy.data.objects.remove(ob,do_unlink=True)
    bpy.ops.import_scene.gltf(filepath=str(out/(name+'.glb')))
    meshes=[ob for ob in bpy.context.selected_objects if ob.type=='MESH']
    assert meshes
    vertices=[ob.matrix_world@Vector(corner) for ob in meshes for corner in ob.bound_box]
    minimum=[min(v[i] for v in vertices) for i in range(3)]
    maximum=[max(v[i] for v in vertices) for i in range(3)]
    if name=='sapper-tpose':
        assert maximum[0]>.96 and minimum[0]<-.96
        assert maximum[2]>1.9 and minimum[2]<.02
        obj=meshes[0]
        handz={-1:[],1:[]}
        for mesh in meshes:
            for vert in mesh.data.vertices:
                p=mesh.matrix_world@vert.co
                if abs(p.x)>.84:handz[1 if p.x>0 else -1].append(p.z)
        assert all(handz.values())
        means={s:sum(values)/len(values) for s,values in handz.items()}
        assert abs(means[-1]-means[1])<.01
        assert all(1.42<v<1.47 for v in means.values())
    else:
        assert maximum[0]>.4 and minimum[0]<-.26
        assert maximum[2]-minimum[2]<.3
    bpy.context.scene.cycles.samples=24
    bpy.context.scene.render.filepath=str(out/(name+'-glb-preview.png'))
    bpy.ops.render.render(write_still=True)
    reports.append({'asset':name,'glb_import_verified':True,'triangles':triangles,'meshes':len(doc['meshes']),'materials':len(doc['materials']),'bounds_min_m':minimum,'bounds_max_m':maximum,'separate_root_verified':True,'rigged':False})
(out/'validation.json').write_text(json.dumps(reports,indent=2)+'\n')
print('BOTH_EXPORTS_VERIFIED',json.dumps(reports))
