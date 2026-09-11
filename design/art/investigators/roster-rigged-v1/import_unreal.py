"""Import skinned FBXs against the existing stock skeleton, preserving it."""
import unreal,json,pathlib,re,math
p=pathlib.Path(__file__).resolve().parent
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX False')
skeleton=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
reports={}
stock=dict(zip(map(str,unreal.DMArtRigLibrary.get_skeleton_bone_names(skeleton)),unreal.DMArtRigLibrary.get_skeleton_reference_transforms(skeleton)))
def serial(t):
    return {'t':[t.translation.x,t.translation.y,t.translation.z],'q':[t.rotation.x,t.rotation.y,t.rotation.z,t.rotation.w],'s':[t.scale3d.x,t.scale3d.y,t.scale3d.z]}
(p/'stock-skeleton-transforms.json').write_text(json.dumps({n:serial(t) for n,t in stock.items()},indent=2))
for asset in ['photographer','sapper','medium','smuggler']:
    if not (p/(asset+'-rigged.fbx')).exists():continue
    task=unreal.AssetImportTask();task.filename=str(p/(asset+'-rigged.fbx'));task.destination_path='/Game/DreadMeridian/Characters/Investigators/'+asset.title()
    task.destination_name='SKM_'+asset.title();task.automated=True;task.replace_existing=True;task.save=True
    opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=True;opt.import_animations=False;opt.import_materials=False;opt.import_textures=False
    opt.skeleton=skeleton;opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH
    opt.skeletal_mesh_import_data.set_editor_property('update_skeleton_reference_pose',False)
    opt.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose',False)
    task.options=opt
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    meshes=[unreal.load_asset(n) for n in task.imported_object_paths if isinstance(unreal.load_asset(n),unreal.SkeletalMesh)]
    assert len(meshes)==1,task.imported_object_paths
    mesh=meshes[0];assert mesh.skeleton==skeleton,'Skeleton assignment failed'
    comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(mesh)
    bones={str(comp.get_bone_name(i)):str(comp.get_parent_bone(comp.get_bone_name(i))) for i in range(comp.get_num_bones())}
    world={};errors={}
    for i,(n,parent) in enumerate(bones.items()):
        t=comp.get_ref_pose_transform(i)
        world[n]=unreal.MathLibrary.compose_transforms(t,world[parent]) if parent in world else t
        expected=stock[n]
        q=t.rotation;e=expected.rotation
        dot=abs(q.x*e.x+q.y*e.y+q.z*e.z+q.w*e.w)
        errors[n]={'rotation_degrees':math.degrees(2*math.acos(min(1,dot))),'translation_cm':(t.translation-expected.translation).length()}
    socket_names=[]
    for name,d in json.loads((p/(asset+'-sockets.json')).read_text()).items():
        v=d['unreal_world'];t=unreal.Transform();t.translation=unreal.Vector(*v['t']);t.rotation=unreal.Quat(*v['q']);t.scale3d=unreal.Vector(*v['s'])
        local=unreal.MathLibrary.make_relative_transform(t,world[d['bone']])
        assert unreal.DMArtRigLibrary.set_mesh_socket(mesh,name,d['bone'],local)
        socket_names.append(name)
    matdata=json.loads((p/(asset+'-materials.json')).read_text());slots=mesh.get_editor_property('materials')
    for i,slot in enumerate(slots):
        # FBX sanitizes material names; resolve using the same alphanumeric key.
        key=lambda s:re.sub('[^a-z0-9]','',str(s).lower())
        spec=next((v for n,v in matdata.items() if key(n)==key(slot.material_slot_name)),None)
        if spec is None:continue
        name='M_'+asset.title()+'_'+key(slot.material_slot_name)
        path=task.destination_path+'/'+name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            slot.material_interface=unreal.load_asset(path);slots[i]=slot
            continue
        mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,task.destination_path,unreal.Material,unreal.MaterialFactoryNew())
        col=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
        col.constant=unreal.LinearColor(*spec['color']);unreal.MaterialEditingLibrary.connect_material_property(col,'',unreal.MaterialProperty.MP_BASE_COLOR)
        for prop,k in [(unreal.MaterialProperty.MP_ROUGHNESS,'roughness'),(unreal.MaterialProperty.MP_METALLIC,'metallic')]:
            node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant);node.r=spec[k];unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
        unreal.MaterialEditingLibrary.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
        slot.material_interface=mat
        slots[i]=slot
    mesh.set_editor_property('materials',slots)
    assert max(e['rotation_degrees'] for e in errors.values())<.03,'Reference bone orientation mismatch'
    assert max(e['translation_cm'] for e in errors.values())<.001,'Reference bone translation mismatch'
    assert all(s.material_interface is not None for s in slots),'Unassigned material slot'
    reports[asset]={'mesh':mesh.get_path_name(),'skeleton':mesh.skeleton.get_path_name(),'bones':bones,'bounds_extent':str(mesh.get_bounds().box_extent),'reference_errors':errors,'mesh_sockets':socket_names,'assigned_materials':sum(s.material_interface is not None for s in slots),'material_slots':[str(s.material_slot_name) for s in slots],'reference_transforms':{str(comp.get_bone_name(i)):serial(comp.get_ref_pose_transform(i)) for i in range(comp.get_num_bones())}}
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh),'Native mesh save failed: '+mesh.get_path_name()
(p/'unreal-validation.json').write_text(json.dumps(reports,indent=2))
unreal.log('RIG_IMPORT_COMPLETE')

