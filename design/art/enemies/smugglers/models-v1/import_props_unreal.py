"""Import the separate props and persist their anchor sockets in native static meshes."""
import unreal,json,pathlib,re
p=pathlib.Path(__file__).resolve().parent
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX False')
report={}
for asset,spec in json.loads((p/'attachments.json').read_text())['items'].items():
    safe=asset.title().replace('-','');dest='/Game/DreadMeridian/Characters/Enemies/Smugglers/Equipment'
    task=unreal.AssetImportTask();task.filename=str(p/'source'/(asset+'.fbx'));task.destination_path=dest;task.destination_name='SM_'+safe;task.automated=True;task.replace_existing=True;task.save=True
    opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=False;opt.import_animations=False;opt.import_materials=False;opt.import_textures=False;opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opt.static_mesh_import_data.set_editor_property('combine_meshes',True)
    task.options=opt;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh=unreal.load_asset(dest+'/'+task.destination_name);assert isinstance(mesh,unreal.StaticMesh)
    locators=json.loads((p/'source'/(asset+'-locators.json')).read_text())
    for name,loc in locators.items():
        m=loc['matrix_local_blender'];point=unreal.Vector(m[0][3]*100,-m[1][3]*100,m[2][3]*100)
        assert unreal.DMArtRigLibrary.set_static_mesh_socket(mesh,name,point)
        mesh.find_socket(name).set_editor_property('relative_rotation',unreal.Quat(*loc['unreal_quaternion_xyzw']).rotator())
    mats=json.loads((p/'source'/(asset+'-materials.json')).read_text());slots=mesh.get_editor_property('static_materials')
    for i,slot in enumerate(slots):
        key=lambda s:re.sub('[^a-z0-9]','',str(s).lower())
        mat_spec=next((v for n,v in mats.items() if key(n)==key(slot.material_slot_name)),None);assert mat_spec,(asset,str(slot.material_slot_name))
        name='M_'+safe+'_'+key(slot.material_slot_name);path=dest+'/'+name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            slot.material_interface=unreal.load_asset(path);slots[i]=slot
            continue
        mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew())
        col=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector);col.constant=unreal.LinearColor(*mat_spec['color']);unreal.MaterialEditingLibrary.connect_material_property(col,'',unreal.MaterialProperty.MP_BASE_COLOR)
        for prop,k in [(unreal.MaterialProperty.MP_ROUGHNESS,'roughness'),(unreal.MaterialProperty.MP_METALLIC,'metallic')]:
            node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant);node.r=mat_spec[k];unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
        unreal.MaterialEditingLibrary.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat);slot.material_interface=mat;slots[i]=slot
    mesh.set_editor_property('static_materials',slots);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    comp=unreal.StaticMeshComponent();comp.set_static_mesh(mesh)
    for name,loc in locators.items():
        assert comp.does_socket_exist(name)
        expected=loc['matrix_local_blender'];v=comp.get_socket_location(name)
        assert (v-unreal.Vector(expected[0][3]*100,-expected[1][3]*100,expected[2][3]*100)).length()<.001
    report[asset]={'mesh':mesh.get_path_name(),'anchors':list(locators),'materials':len(slots),'passed':True}
(p/'unreal-props-validation.json').write_text(json.dumps(report,indent=2))
unreal.log('PROP_IMPORT_COMPLETE')
