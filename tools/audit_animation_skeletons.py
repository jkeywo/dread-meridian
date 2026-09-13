"""Read back skeleton bindings for presentation clips; never modify source animations."""
import unreal,json
from pathlib import Path
P=Path(__file__).resolve().parents[1];report=[]
target=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
for path in unreal.EditorAssetLibrary.list_assets('/Game/DreadMeridian/Presentation/Animations',recursive=True,include_folder=False):
    if '/A_DM_' not in path:continue
    asset=unreal.load_asset(path)
    if not isinstance(asset,unreal.AnimSequence):continue
    sk=asset.get_editor_property('skeleton')
    report.append({'path':asset.get_path_name(),'skeleton':sk.get_path_name() if sk else None,'manny':sk==target,'duration':asset.get_play_length(),'root_motion':asset.get_editor_property('enable_root_motion')})
(P/'Saved/animation-skeleton-audit.json').write_text(json.dumps(report,indent=2))
assert report and all(row['manny'] for row in report), 'Presentation animation is missing or targets a different skeleton'
