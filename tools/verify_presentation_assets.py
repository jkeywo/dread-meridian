"""Load every package a migration.json section requested and fail if any is missing or does not load.

Run inside the editor: tools/Unreal.ps1 -Action Python -Script tools/verify_presentation_assets.py
Section defaults to 'kits'; override with the DM_MIGRATION_SECTION environment variable.
"""
import json, os, pathlib, sys, unreal

section = os.environ.get("DM_MIGRATION_SECTION", "kits")
manifest = json.loads((pathlib.Path(__file__).resolve().parents[1] / "design/art/presentation/migration.json").read_text())
requested = manifest.get(section, {}).get("requested", [])
if not requested:
    unreal.log_error(f"migration.json has no '{section}' section with requested packages")
    sys.exit(1)
failures = []
for package in requested:
    if not unreal.EditorAssetLibrary.does_asset_exist(package):
        failures.append(f"{package}: missing")
        continue
    asset = unreal.EditorAssetLibrary.load_asset(package)
    if asset is None:
        failures.append(f"{package}: failed to load")
        continue
    unreal.log(f"{package}: {asset.get_class().get_name()} ok")
if failures:
    for f in failures:
        unreal.log_error(f)
    sys.exit(1)
unreal.log(f"all {len(requested)} '{section}' packages load")
