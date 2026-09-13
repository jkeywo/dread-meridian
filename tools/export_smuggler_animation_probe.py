import unreal
from pathlib import Path
P=Path(__file__).resolve().parents[1]/'Saved/SmugglerAnimationProbe';P.mkdir(exist_ok=True)
for name in ['KB_Idle_1','KB_p_Jab_R_1','KB_Superpunch']:
    a=unreal.load_asset('/Game/DreadMeridian/Presentation/Animations/A_DM_'+name)
    task=unreal.AssetExportTask();task.object=a;task.filename=str(P/(name+'.fbx'));task.automated=True;task.prompt=False;task.replace_identical=True
    task.options=unreal.FbxExportOption();task.options.export_preview_mesh=False;task.exporter=unreal.AnimSequenceExporterFBX()
    assert unreal.Exporter.run_asset_export_task(task)
