import bpy,json,math,shutil
import numpy as np
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parent
mapping={1:'lookout',2:'bomber',3:'smuggler',4:'photographer',5:'medium',6:'sapper',7:'gunman',8:'bruiser',9:'gang-boss'}
inventory=json.loads((P/'source-inventory.json').read_text());manifest={};landmarks={}
(P/'prepared').mkdir(exist_ok=True);(P/'textures').mkdir(exist_ok=True)
for item in inventory:
    asset=mapping[item['id']];path=Path(item['path']);bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version=0
    if path.suffix=='.glb':bpy.ops.import_scene.gltf(filepath=str(path))
    else:bpy.ops.import_scene.fbx(filepath=str(path))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes:o.select_set(True)
    bpy.context.view_layer.objects.active=meshes[0];bpy.ops.object.join();mesh=bpy.context.object
    mw=mesh.matrix_world.copy();mesh.parent=None;mesh.matrix_world=mw;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    coords=np.array([v.co[:] for v in mesh.data.vertices]);lo=coords.min(0);hi=coords.max(0)
    H=2.08 if asset in ['smuggler','bruiser'] else 1.9
    coords=(coords-np.array([(lo[0]+hi[0])/2,0,lo[2]]))*(H/(hi[2]-lo[2]))
    span=max(abs(coords[:,0]));arm=coords[(abs(coords[:,0])>span*.5)]
    ay=float(np.median(arm[:,1]));coords[:,1]+=.025-ay
    shoulder=float(np.median(arm[:,2]));leg=float(np.median(abs(coords[coords[:,2]<H*.12,0])))
    sx=H*(.135 if asset in ['smuggler','bruiser'] else .115);wrist=span*.79
    cfg=dict(hip=H*.54,knee=H*.29,leg=leg,shoulder=shoulder,sx=sx,elbow=(sx+wrist)/2,wrist=wrist,palm=span*.87,head=H*.90,neck=H*.845,size=(span*.1)/.075)
    mesh.data.vertices.foreach_set('co',coords.astype(np.float32).ravel());mesh.data.update();mesh.name='Source_'+asset
    # Explicit semantic texture extraction, preserving delivered image pixels.
    textures={}
    if path.suffix=='.fbx':
        for file in path.parent.rglob('*'):
            if file.suffix.lower() in ['.jpg','.jpeg','.png']:bpy.data.images.load(str(file),check_existing=False)
    for im in list(bpy.data.images):
        low=im.name.lower()
        key='basecolor' if any(k in low for k in ['basecolor','rgb_']) else 'normal' if 'normal' in low else 'roughness' if 'roughness' in low else 'metallic' if 'metallic' in low else 'rm' if '_rm' in low else None
        if key and im.size[0]>0:
            dest=P/'textures'/f'{asset}-{key}.png';im.filepath_raw=str(dest);im.file_format='PNG';im.save();im.pack();textures[key]=str(dest)
    assert 'basecolor' in textures,(asset,[i.name for i in bpy.data.images])
    mat=bpy.data.materials.new('Supplied_'+asset);mat.use_nodes=True;bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.7
    for key,file in textures.items():
        im=bpy.data.images.load(file,check_existing=False)
        if key!='basecolor':im.colorspace_settings.name='Non-Color'
        im.pack();node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=im
        if key=='normal':
            nm=mat.node_tree.nodes.new('ShaderNodeNormalMap');mat.node_tree.links.new(node.outputs['Color'],nm.inputs['Color']);mat.node_tree.links.new(nm.outputs[0],bs.inputs['Normal'])
        elif key=='rm':
            sep=mat.node_tree.nodes.new('ShaderNodeSeparateColor');mat.node_tree.links.new(node.outputs['Color'],sep.inputs[0]);mat.node_tree.links.new(sep.outputs['Green'],bs.inputs['Roughness']);mat.node_tree.links.new(sep.outputs['Blue'],bs.inputs['Metallic'])
        else:mat.node_tree.links.new(node.outputs['Color'],bs.inputs[{'basecolor':'Base Color','roughness':'Roughness','metallic':'Metallic'}[key]])
    mesh.data.materials.clear();mesh.data.materials.append(mat)
    for poly in mesh.data.polygons:poly.material_index=0
    for attr in list(mesh.data.color_attributes):mesh.data.color_attributes.remove(attr)
    bpy.ops.wm.save_as_mainfile(filepath=str(P/'prepared'/f'{asset}.blend'))
    bpy.ops.export_scene.gltf(filepath=str(P/'prepared'/f'{asset}.glb'),export_format='GLB',use_selection=True,export_extras=True)
    old=P.parent/('investigators/roster-rigged-v1' if asset in ['sapper','photographer','medium','smuggler'] else 'enemies/smugglers/models-v1')
    shutil.copy2(old/f'{asset}-sockets.json',P/f'{asset}-sockets.json')
    ue=asset.title().replace('-','');group='Investigators' if asset in ['sapper','photographer','medium','smuggler'] else 'Enemies/Smugglers'
    manifest[asset]={'source_id':item['id'],'source_file':item['file'],'source_sha256':item['sha256'],'textures':textures,'native_path':f'/Game/DreadMeridian/Characters/{group}/{ue}/SKM_{ue}','vertices':len(mesh.data.vertices),'polygons':len(mesh.data.polygons)}
    landmarks[asset]=cfg;print('PREPARED',asset,cfg,flush=True)
(P/'manifest.json').write_text(json.dumps(manifest,indent=2));(P/'landmarks.json').write_text(json.dumps(landmarks,indent=2))
