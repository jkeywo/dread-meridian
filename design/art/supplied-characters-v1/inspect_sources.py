import bpy,json,zipfile,shutil,sys,hashlib
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parent;D=Path('C:/Users/jkeyw/Downloads')
names=['adventurer female 3d model.glb','female explorer 3d model.glb','character+3d+model.zip','female+adventurer+3d+model.zip','fantasy+wizard+character+3d+model.zip','historical+soldier+3d+model.zip','cowboy character 3d model.glb','pirate 3d model.glb','vintage gangster 3d model.glb']
reports=[]
for idx,name in enumerate(names):
    folder=P/'source'/str(idx+1);folder.mkdir(parents=True,exist_ok=True)
    src=D/name;shutil.copy2(src,folder/name)
    if src.suffix=='.zip':
        with zipfile.ZipFile(src) as z:
            for entry in z.infolist():
                dest=(folder/entry.filename).resolve()
                assert dest.is_relative_to(folder.resolve())
                if dest.suffix.lower() in ['.fbx','.png','.jpg','.jpeg']:
                    dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(z.read(entry))
        path=next(folder.glob('*.fbx'))
    else:path=folder/name
    bpy.ops.wm.read_factory_settings(use_empty=True)
    if path.suffix=='.glb':bpy.ops.import_scene.gltf(filepath=str(path))
    else:bpy.ops.import_scene.fbx(filepath=str(path))
    objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
    corners=[o.matrix_world@Vector(c) for o in objects for c in o.bound_box]
    lo=Vector([min(v[i] for v in corners) for i in range(3)]);hi=Vector([max(v[i] for v in corners) for i in range(3)])
    centre=(lo+hi)/2;size=hi-lo
    info={'id':idx+1,'file':name,'path':str(path),'sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'min':list(lo),'max':list(hi),'size':list(size),'meshes':[(o.name,len(o.data.vertices),len(o.data.polygons)) for o in objects],'armatures':[o.name for o in bpy.context.scene.objects if o.type=='ARMATURE'],'materials':[m.name for m in bpy.data.materials],'images':[(i.name,i.filepath,list(i.size)) for i in bpy.data.images]}
    reports.append(info)
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=12;scene.cycles.use_denoising=True
    scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.4,.4,.4,1)
    scale=max(size);scene.render.resolution_x=800;scene.render.resolution_y=800;scene.render.resolution_percentage=100
    bpy.ops.object.camera_add();cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=scale*1.18;scene.camera=cam
    for pos,power in [((1,-2,3),180),((-2,-1,1),100),((0,2,2),100)]:
        bpy.ops.object.light_add(type='AREA',location=centre+Vector(pos)*scale);l=bpy.context.object;l.data.energy=power*scale**2;l.data.size=scale*2;l.rotation_euler=(centre-l.location).to_track_quat('-Z','Y').to_euler()
    for view,direction in [('front',(0,-3,.05)),('back',(0,3,.05))]:
        cam.location=centre+Vector(direction)*scale;cam.rotation_euler=(centre-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(P/f'source-{idx+1}-{view}.png');bpy.ops.render.render(write_still=True)
    print('INSPECTED',json.dumps(info),flush=True)
(P/'source-inventory.json').write_text(json.dumps(reports,indent=2))
