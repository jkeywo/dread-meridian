import bpy,json,hashlib,shutil
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parent
report={}
for name in ['crawler','lurker','spitter','grasper','oldThing','broodling','blackgoat']:
    source=Path('C:/Users/jkeyw/Downloads')/(name+'.glb')
    (P/'source').mkdir(exist_ok=True);shutil.copy2(source,P/'source'/source.name)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    coords=[o.matrix_world@Vector(c) for o in meshes for c in o.bound_box]
    lo=Vector([min(v[i] for v in coords) for i in range(3)]);hi=Vector([max(v[i] for v in coords) for i in range(3)])
    report[name]={'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'min':list(lo),'max':list(hi),'meshes':[(o.name,len(o.data.vertices),len(o.data.polygons)) for o in meshes],'armatures':[o.name for o in bpy.data.objects if o.type=='ARMATURE'],'images':[(i.name,list(i.size)) for i in bpy.data.images]}
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=8
    scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.4,.4,.4,1)
    center=(lo+hi)/2;size=max(hi-lo)
    bpy.ops.object.camera_add(location=center+Vector((0,-size*2,.1*size)));cam=bpy.context.object;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=size*1.16;scene.camera=cam
    bpy.ops.object.light_add(type='AREA',location=center+Vector((-size,-size,size)));bpy.context.object.data.energy=1000*size*size;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=size
    scene.render.resolution_x=900;scene.render.resolution_y=900;scene.render.resolution_percentage=100;scene.render.filepath=str(P/(name+'-source.png'));bpy.ops.render.render(write_still=True)
    (P/'source-inventory.json').write_text(json.dumps(report,indent=2))
print('INSPECTION_COMPLETE')
