import bpy,json
import numpy as np
from pathlib import Path
P=Path(__file__).resolve().parent
(P/'prepared').mkdir(exist_ok=True);(P/'textures').mkdir(exist_ok=True)
inventory=json.loads((P/'source-inventory.json').read_text());manifest={};landmarks={}
humanoids=['lurker','grasper','oldThing','blackgoat']
for asset,info in inventory.items():
    bpy.ops.wm.read_factory_settings(use_empty=True);bpy.context.preferences.filepaths.save_version=0
    bpy.ops.import_scene.gltf(filepath=str(P/'source'/(asset+'.glb')))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes:o.select_set(True)
    bpy.context.view_layer.objects.active=meshes[0];bpy.ops.object.join();mesh=bpy.context.object
    mw=mesh.matrix_world.copy();mesh.parent=None;mesh.matrix_world=mw;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    coords=np.array([v.co[:] for v in mesh.data.vertices]);lo=coords.min(0);hi=coords.max(0)
    H=1.9 if asset in humanoids else {'crawler':1.0,'spitter':1.25,'broodling':.9}[asset]
    coords=(coords-np.array([(lo[0]+hi[0])/2,0,lo[2]]))*(H/(hi[2]-lo[2]))
    if asset in humanoids:
        span=max(abs(coords[:,0]));arm=coords[abs(coords[:,0])>span*.5];coords[:,1]+=.025-float(np.median(arm[:,1]))
        shoulder=float(np.median(arm[:,2]));leg=float(np.median(abs(coords[coords[:,2]<H*.12,0])))
        sx=H*(.16 if asset in ['oldThing','blackgoat'] else .115);wrist=span*.79
        landmarks[asset]=dict(hip=H*.52,knee=H*.28,leg=leg,shoulder=shoulder,sx=sx,elbow=(sx+wrist)/2,wrist=wrist,palm=span*.87,head=H*.86,neck=H*.79,size=(span*.1)/.075)
        (P/(asset+'-sockets.json')).write_text('{}')
    mesh.data.vertices.foreach_set('co',coords.astype(np.float32).ravel());mesh.data.update();mesh.name='Source_'+asset
    textures={}
    for im in list(bpy.data.images):
        if im.size[0] and 'basecolor' in im.name.lower():
            dest=P/'textures'/(asset+'-basecolor.png');im.filepath_raw=str(dest);im.file_format='PNG';im.save();im.pack();textures['basecolor']=str(dest)
    assert textures,asset
    mat=bpy.data.materials.new('Supplied_'+asset);mat.use_nodes=True;bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.75
    im=bpy.data.images.load(textures['basecolor'],check_existing=False);im.pack();node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=im;mat.node_tree.links.new(node.outputs['Color'],bs.inputs['Base Color'])
    mesh.data.materials.clear();mesh.data.materials.append(mat)
    for poly in mesh.data.polygons:poly.material_index=0
    for attr in list(mesh.data.color_attributes):mesh.data.color_attributes.remove(attr)
    bpy.ops.wm.save_as_mainfile(filepath=str(P/'prepared'/(asset+'.blend')))
    bpy.ops.export_scene.gltf(filepath=str(P/'prepared'/(asset+'.glb')),export_format='GLB',use_selection=True)
    if asset not in humanoids:
        bpy.ops.export_scene.fbx(filepath=str(P/(asset+'-static.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
    group='Shub' if asset in ['broodling','blackgoat'] else 'SwampThings';name={'oldThing':'OldThing','blackgoat':'BlackGoat'}.get(asset,asset.title())
    manifest[asset]={'source_file':asset+'.glb','source_sha256':info['sha256'],'humanoid':asset in humanoids,'textures':textures,'native_path':f'/Game/DreadMeridian/Characters/Enemies/{group}/{name}/'+('SKM_' if asset in humanoids else 'SM_')+name}
(P/'manifest.json').write_text(json.dumps(manifest,indent=2));(P/'landmarks.json').write_text(json.dumps(landmarks,indent=2))
