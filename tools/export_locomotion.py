"""Run in GameAnimationSample via Unreal's Python commandlet; source assets are read-only."""
import unreal,pathlib
out=pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/locomotion')
out.mkdir(parents=True,exist_ok=True)
base='/Game/Characters/UEFN_Mannequin/Animations/'
names=['Run/M_Neutral_Run_Start_F_Rfoot','Run/M_Neutral_Run_Stop_F_Rfoot','Idle/M_Neutral_Stand_Turn_090_L','Idle/M_Neutral_Stand_Turn_090_R']
for name in names:
 a=unreal.load_asset(base+name);assert a,name
 t=unreal.AssetExportTask();t.object=a;t.filename=str(out/(a.get_name()+'.fbx'))
 t.automated=True;t.prompt=False;t.replace_identical=True;t.options=unreal.FbxExportOption()
 t.options.set_editor_property('ascii',False)
 assert unreal.Exporter.run_asset_export_task(t),name
