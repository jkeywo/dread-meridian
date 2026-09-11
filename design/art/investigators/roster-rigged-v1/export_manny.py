"""Export the project's actual stock Manny reference; run through tools/Unreal.ps1."""
import unreal
import pathlib
import json

out = pathlib.Path(__file__).resolve().parent
mesh = unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
assert mesh, 'Stock Manny mesh missing'
def export(asset, name):
    task = unreal.AssetExportTask()
    task.object = asset
    task.filename = str(out / name)
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.options = unreal.FbxExportOption()
    task.exporter = unreal.SkeletalMeshExporterFBX() if isinstance(asset, unreal.SkeletalMesh) else unreal.AnimSequenceExporterFBX()
    task.options.set_editor_property('export_preview_mesh', False)
    task.options.set_editor_property('level_of_detail', False)
    task.options.set_editor_property('bake_material_inputs', unreal.FbxMaterialBakeMode.DISABLED)
    task.options.set_editor_property('export_source_mesh', True)
    task.options.set_editor_property('export_morph_targets', False)
    assert unreal.Exporter.run_asset_export_task(task), name
# Mesh export crashes in UE 5.8.2; animation FBX carries the same reference skeleton.
for name, path in [('idle','/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle'), ('walk','/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd')]:
    export(unreal.load_asset(path), 'manny-'+name+'.fbx')
(out/'unreal-reference.json').write_text(json.dumps({'engine':unreal.SystemLibrary.get_engine_version(),'mesh':mesh.get_path_name(),'skeleton':mesh.get_editor_property('skeleton').get_path_name()},indent=2))
unreal.log('MANNY_EXPORT_COMPLETE')

