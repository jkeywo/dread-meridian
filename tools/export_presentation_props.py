import bpy,json,pathlib
p=pathlib.Path('C:/Coding/dread-meridian/design/art/investigators/roster-rigged-v1')
out=pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/props');out.mkdir(exist_ok=True)
report={}
for item,title,anchors in [('sapper-carbine','SapperCarbine',['ANCHOR_grip_r','ANCHOR_stow']),('photographer-rifle','PhotographerRifle',['ANCHOR_grip_r','ANCHOR_stow']),('photographer-camera','PhotographerCamera',['ANCHOR_grip_r','ANCHOR_stow']),('medium-spirit-wisp','MediumWisp',['ANCHOR_emit'])]:
 for anchor in anchors:
  bpy.ops.wm.open_mainfile(filepath=str(p/(item+'.blend')))
  a=bpy.data.objects.get(anchor);assert a,(item,anchor)
  inv=a.matrix_world.inverted();meshes=[o for o in a.parent.children_recursive if o.type in {'MESH','CURVE'}]
  bpy.ops.object.select_all(action='DESELECT')
  for o in meshes:
   world=o.matrix_world.copy();o.parent=None;o.matrix_world=inv@world;o.select_set(True)
  bpy.context.view_layer.objects.active=meshes[0];bpy.ops.object.convert(target='MESH');bpy.ops.object.join();obj=bpy.context.object
  name='SM_'+title+('_Stowed' if anchor=='ANCHOR_stow' else '_Held');obj.name=name
  bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
  mats=[]
  for mat in obj.data.materials:
   bs=next((n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None) if mat.use_nodes else None
   mats.append({'name':mat.name,'color':list(bs.inputs['Base Color'].default_value if bs else mat.diffuse_color),'roughness':float(bs.inputs['Roughness'].default_value) if bs else .7,'metallic':float(bs.inputs['Metallic'].default_value) if bs else 0})
  bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z')
  report[name]={'source':item,'anchor':anchor,'materials':mats,'dimensions_m':list(obj.dimensions)}
(out/'props.json').write_text(json.dumps(report,indent=2))
print('PROP_EXPORT_COMPLETE')
