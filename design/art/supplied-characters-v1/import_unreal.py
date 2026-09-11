"""Replace canonical runtime meshes, retaining the stock skeleton and sockets."""
import unreal,json,math,shutil
from pathlib import Path
P=Path(__file__).resolve().parent;ROOT=P.parents[2]
manifest=json.loads((P/'manifest.json').read_text());reports={}
for command in ['Interchange.FeatureFlags.Import.FBX False','Editor.AsyncSkinnedAssetCompilation 0','Editor.AsyncStaticMeshCompilation 0','s.AllowMultithreadedLoading 0']:
    unreal.SystemLibrary.execute_console_command(None,command)
skel=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin');assert skel
stock=dict(zip(map(str,unreal.DMArtRigLibrary.get_skeleton_bone_names(skel)),unreal.DMArtRigLibrary.get_skeleton_reference_transforms(skel)))
tools=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.EditorAssetLibrary;M=unreal.MaterialEditingLibrary
for asset,spec in manifest.items():
    native=spec['native_path'];folder,name=native.rsplit('/',1)
    # Preserve the previous native asset before replacing it at its referenced path.
    relative=native.removeprefix('/Game/')+'.uasset';old=ROOT/'Content'/relative;backup=P/'previous-native'/relative
    if old.exists() and not backup.exists():backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(old,backup)
    materialpath=folder+'/M_Supplied_'+asset.replace('-','_')
    mat=unreal.load_asset(materialpath)
    if not mat:
        mat=tools.create_asset(materialpath.rsplit('/',1)[1],folder,unreal.Material,unreal.MaterialFactoryNew())
        for key,file in spec['textures'].items():
            task=unreal.AssetImportTask();task.filename=file;task.destination_path=folder+'/SuppliedTextures';task.destination_name='T_'+asset.replace('-','_')+'_'+key;task.automated=True;task.replace_existing=True;task.save=True
            tools.import_asset_tasks([task]);tex=unreal.load_asset(task.imported_object_paths[0]);assert isinstance(tex,unreal.Texture2D)
            tex.set_editor_property('srgb',key=='basecolor')
            tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP if key=='normal' else unreal.TextureCompressionSettings.TC_DEFAULT if key=='basecolor' else unreal.TextureCompressionSettings.TC_MASKS)
            if key=='normal':tex.set_editor_property('flip_green_channel',True)
            assert E.save_loaded_asset(tex)
            n=M.create_material_expression(mat,unreal.MaterialExpressionTextureSample);n.texture=tex
            n.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if key=='normal' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if key=='basecolor' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
            if key=='rm':
                M.connect_material_property(n,'G',unreal.MaterialProperty.MP_ROUGHNESS);M.connect_material_property(n,'B',unreal.MaterialProperty.MP_METALLIC)
            else:M.connect_material_property(n,'RGB' if key in ['basecolor','normal'] else 'R',{'basecolor':unreal.MaterialProperty.MP_BASE_COLOR,'normal':unreal.MaterialProperty.MP_NORMAL,'roughness':unreal.MaterialProperty.MP_ROUGHNESS,'metallic':unreal.MaterialProperty.MP_METALLIC}[key])
        if not any(k in spec['textures'] for k in ['roughness','rm']):
            n=M.create_material_expression(mat,unreal.MaterialExpressionConstant);n.r=.7;M.connect_material_property(n,'',unreal.MaterialProperty.MP_ROUGHNESS)
        M.recompile_material(mat);assert E.save_loaded_asset(mat)
    M.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    M.recompile_material(mat);assert E.save_loaded_asset(mat)
    task=unreal.AssetImportTask();task.filename=str(P/f'{asset}-rigged.fbx');task.destination_path=folder;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True
    opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=True;opt.import_animations=False;opt.import_materials=False;opt.import_textures=False;opt.skeleton=skel;opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH
    opt.skeletal_mesh_import_data.set_editor_property('update_skeleton_reference_pose',False);opt.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose',False);task.options=opt
    tools.import_asset_tasks([task]);mesh=unreal.load_asset(native);assert isinstance(mesh,unreal.SkeletalMesh) and mesh.skeleton==skel
    slots=mesh.get_editor_property('materials')
    for i,slot in enumerate(slots):slot.material_interface=mat;slots[i]=slot
    mesh.set_editor_property('materials',slots)
    comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(mesh);world={};maxangle=0;maxtranslation=0
    for i in range(comp.get_num_bones()):
        bone=str(comp.get_bone_name(i));parent=str(comp.get_parent_bone(comp.get_bone_name(i)));t=comp.get_ref_pose_transform(i)
        world[bone]=unreal.MathLibrary.compose_transforms(t,world[parent]) if parent in world else t
        expected=stock[bone];q=t.rotation;e=expected.rotation;dot=abs(q.x*e.x+q.y*e.y+q.z*e.z+q.w*e.w)
        maxangle=max(maxangle,math.degrees(2*math.acos(min(1,dot))));maxtranslation=max(maxtranslation,(t.translation-expected.translation).length())
    assert maxangle<.03 and maxtranslation<.001,(asset,maxangle,maxtranslation)
    sockets=json.loads((P/f'{asset}-sockets.json').read_text())
    for socket,d in sockets.items():
        v=d['unreal_world'];t=unreal.Transform();t.translation=unreal.Vector(*v['t']);t.rotation=unreal.Quat(*v['q']);t.scale3d=unreal.Vector(*v['s'])
        local=unreal.MathLibrary.make_relative_transform(t,world[d['bone']]);assert unreal.DMArtRigLibrary.set_mesh_socket(mesh,socket,d['bone'],local)
    assert E.save_loaded_asset(mesh),'Failed to save replacement '+native
    reports[asset]={'mesh':mesh.get_path_name(),'skeleton':mesh.skeleton.get_path_name(),'bones':comp.get_num_bones(),'sockets':len(sockets),'material':mat.get_path_name(),'reference_rotation_error_degrees':maxangle,'reference_translation_error_cm':maxtranslation,'saved':True}
    (P/'unreal-validation.json').write_text(json.dumps(reports,indent=2));unreal.log('REPLACED_SUPPLIED_CHARACTER '+asset)
assert len(reports)==9
unreal.log('SUPPLIED_CHARACTERS_IMPORT_COMPLETE')
