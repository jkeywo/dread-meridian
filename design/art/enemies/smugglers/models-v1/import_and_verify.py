"""Final native asset import and readback in pinned Unreal 5.8.2."""
from pathlib import Path
import unreal
for command in ['Editor.AsyncSkinnedAssetCompilation 0','Editor.AsyncStaticMeshCompilation 0','s.AllowMultithreadedLoading 0']:
    unreal.SystemLibrary.execute_console_command(None,command)
p=Path(__file__).resolve().parent
for name in ['import_unreal.py','import_props_unreal.py','import_tpose_unreal.py','verify_unreal.py','import_players_unreal.py']:
    script=p/name
    exec(compile(script.read_text(encoding='utf-8-sig'),str(script),'exec'),{'__file__':str(script),'__name__':'__main__'})
