import unreal,pathlib,json
p=pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/props');spec=json.loads((p/'props.json').read_text());root='/Game/DreadMeridian/Presentation/Props';tools=unreal.AssetToolsHelpers.get_asset_tools()
results={}
for name,data in spec.items():
 task=unreal.AssetImportTask();task.filename=str(p/(name+'.fbx'));task.destination_path=root;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True
 options=unreal.FbxImportUI();options.import_as_skeletal=False;options.import_mesh=True;options.import_animations=False;options.import_materials=False;options.import_textures=False
 options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.automated_import_should_detect_type=False
 options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.convert_scene=True;options.static_mesh_import_data.convert_scene_unit=True
 task.options=options;task.factory=unreal.FbxFactory();tools.import_asset_tasks([task]);mesh=unreal.load_asset(root+'/'+name);assert mesh,name
 slots=mesh.get_editor_property('static_materials')
 for i,slot in enumerate(slots):
  import re
  key=lambda n:re.sub('[^a-z0-9]','',str(n).lower())
  m=next((m for m in data['materials'] if key(m['name'])==key(slot.material_slot_name)),data['materials'][min(i,len(data['materials'])-1)])
  mn='M_'+name+'_'+str(i);mp=root+'/'+mn
  mat=unreal.load_asset(mp) if unreal.EditorAssetLibrary.does_asset_exist(mp) else tools.create_asset(mn,root,unreal.Material,unreal.MaterialFactoryNew())
  unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
  col=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector);col.constant=unreal.LinearColor(*m['color']);unreal.MaterialEditingLibrary.connect_material_property(col,'',unreal.MaterialProperty.MP_BASE_COLOR)
  for prop,k in [(unreal.MaterialProperty.MP_ROUGHNESS,'roughness'),(unreal.MaterialProperty.MP_METALLIC,'metallic')]:
   node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant);node.r=m[k];unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
  unreal.MaterialEditingLibrary.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat);mesh.set_material(i,mat)
 unreal.EditorAssetLibrary.save_loaded_asset(mesh);results[name]={'bounds':str(mesh.get_bounds().box_extent),'materials':len(slots)}
(p/'unreal-props.json').write_text(json.dumps(results,indent=2));unreal.log('PROP_IMPORT_COMPLETE')
