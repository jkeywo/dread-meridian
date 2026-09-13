import unreal,json
from pathlib import Path
P=Path(__file__).resolve().parent;manifest=json.loads((P/'manifest.json').read_text());reports={}
E=unreal.EditorAssetLibrary;M=unreal.MaterialEditingLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
for command in ['Interchange.FeatureFlags.Import.FBX False','Editor.AsyncSkinnedAssetCompilation 0','Editor.AsyncStaticMeshCompilation 0']:
    unreal.SystemLibrary.execute_console_command(None,command)
skel=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin');assert skel
for asset,spec in manifest.items():
    folder,name=spec['native_path'].rsplit('/',1);matpath=folder+'/M_'+asset
    mat=unreal.load_asset(matpath)
    if not mat:
        mat=tools.create_asset('M_'+asset,folder,unreal.Material,unreal.MaterialFactoryNew())
        task=unreal.AssetImportTask();task.filename=spec['textures']['basecolor'];task.destination_path=folder;task.destination_name='T_'+asset+'_BaseColor';task.automated=True;task.save=True
        tools.import_asset_tasks([task]);tex=unreal.load_asset(task.imported_object_paths[0]);assert isinstance(tex,unreal.Texture2D)
        tex.set_editor_property('srgb',True);assert E.save_loaded_asset(tex)
        node=M.create_material_expression(mat,unreal.MaterialExpressionTextureSample);node.texture=tex;node.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
        M.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=M.create_material_expression(mat,unreal.MaterialExpressionConstant);rough.r=.75;M.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    if spec['humanoid']:M.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    M.recompile_material(mat);assert E.save_loaded_asset(mat)
    task=unreal.AssetImportTask();task.filename=str(P/(asset+('-rigged.fbx' if spec['humanoid'] else '-static.fbx')));task.destination_path=folder;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True
    opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=spec['humanoid'];opt.import_animations=False;opt.import_materials=False;opt.import_textures=False;opt.automated_import_should_detect_type=False
    opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH if spec['humanoid'] else unreal.FBXImportType.FBXIT_STATIC_MESH
    if spec['humanoid']:
        opt.skeleton=skel;opt.skeletal_mesh_import_data.set_editor_property('update_skeleton_reference_pose',False);opt.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose',False)
    else:opt.static_mesh_import_data.set_editor_property('combine_meshes',True);opt.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
    task.options=opt;tools.import_asset_tasks([task]);mesh=unreal.load_asset(spec['native_path']);assert mesh,asset
    if spec['humanoid']:
        assert mesh.skeleton==skel
        slots=mesh.get_editor_property('materials')
        for i,slot in enumerate(slots):slot.material_interface=mat;slots[i]=slot
        mesh.set_editor_property('materials',slots)
    else:
        slots=mesh.get_editor_property('static_materials')
        if not slots:slots=[unreal.StaticMaterial()]
        for i,slot in enumerate(slots):slot.material_interface=mat;slots[i]=slot
        mesh.set_editor_property('static_materials',slots)
    assert E.save_loaded_asset(mesh)
    reports[asset]={'mesh':mesh.get_path_name(),'material':mat.get_path_name(),'humanoid':spec['humanoid'],'saved':True}
(P/'unreal-import-validation.json').write_text(json.dumps(reports,indent=2))
assert len(reports)==7
