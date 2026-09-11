"""Import all equipped pose clips and compare native hand positions to Blender."""
import unreal,json,pathlib
p=pathlib.Path(__file__).resolve().parent/'equipped'
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX False')
skeleton=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin');reports={}
for source in sorted(p.glob('*-equipped.fbx')):
    spec=json.loads(source.with_name(source.stem+'-validation.json').read_text())
    task=unreal.AssetImportTask();task.filename=str(source);task.destination_path='/Game/DreadMeridian/Characters/Enemies/Smugglers/Poses';task.destination_name='A_'+source.stem.title().replace('-','');task.automated=True;task.replace_existing=True;task.save=True
    opt=unreal.FbxImportUI();opt.import_mesh=False;opt.import_as_skeletal=True;opt.import_animations=True;opt.skeleton=skeleton;opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION;task.options=opt
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    clips=[unreal.load_asset(n) for n in task.imported_object_paths if isinstance(unreal.load_asset(n),unreal.AnimSequence)];assert len(clips)==1
    anim=clips[0];assert anim.get_editor_property('skeleton')==skeleton
    assert unreal.EditorAssetLibrary.save_loaded_asset(anim)
    pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim,0,unreal.AnimPoseEvaluationOptions());assert unreal.AnimPoseExtensions.is_valid(pose)
    errors={}
    for side,expected in spec['unreal_hand_world'].items():
        actual=unreal.AnimPoseExtensions.get_bone_pose(pose,'hand_'+side,unreal.AnimPoseSpaces.WORLD)
        error=(actual.translation-unreal.Vector(*expected['t'])).length();assert error<.05,(source.name,side,error)
        errors[side]=error
    reports[source.stem]={'animation':anim.get_path_name(),'hand_error_cm':errors,'passed':True}
assert len(reports)==7
(p/'unreal-equipped-validation.json').write_text(json.dumps(reports,indent=2))
unreal.log('EQUIPPED_POSES_VERIFIED')
