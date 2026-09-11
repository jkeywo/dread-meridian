"""Load every package a migration.json section requested and fail if any is missing or does not load.

Run inside the editor: tools/Unreal.ps1 -Action Python -Script tools/verify_presentation_assets.py

Checks every section that carries a "requested" list (the effect systems and the kit source clips), plus the
retargeted outputs recorded in retarget-validation.json. Byte-copying a package and its dependency closure can
miss a reference the scanner could not see, so this is what actually proves a migration landed intact.
Override the sections with DM_MIGRATION_SECTIONS (comma-separated).
"""
import json, os, pathlib, sys, unreal

REPO = pathlib.Path(__file__).resolve().parents[1]
manifest = json.loads((REPO / 'design/art/presentation/migration.json').read_text())
validation = REPO / 'design/art/presentation/retarget-validation.json'

wanted = os.environ.get('DM_MIGRATION_SECTIONS')
sections = [s.strip() for s in wanted.split(',')] if wanted else [
    name for name, body in manifest.items() if isinstance(body, dict) and body.get('requested')]
if not sections:
    unreal.log_error('migration.json has no section with a requested list')
    sys.exit(1)

targets = []
for section in sections:
    for package in manifest.get(section, {}).get('requested', []):
        targets.append((section, package))
if validation.exists():
    for entry in json.loads(validation.read_text()):
        # Recorded as an object path (/Game/X/Y.Y); load_asset wants the package path.
        targets.append(('retargeted', entry['path'].split('.')[0]))

# The result goes to a report file, not the log: a python commandlet's stdout and unreal.log output do not reach
# the console it was launched from, so a silent pass would be indistinguishable from having checked nothing.
REPORT = REPO / 'Saved/PresentationAssets.json'

checked, failures = [], []
for section, package in targets:
    if not unreal.EditorAssetLibrary.does_asset_exist(package):
        failures.append('%s: %s missing' % (section, package))
        continue
    asset = unreal.EditorAssetLibrary.load_asset(package)
    if asset is None:
        failures.append('%s: %s failed to load' % (section, package))
        continue
    checked.append({'section': section, 'package': package, 'class': asset.get_class().get_name()})

REPORT.parent.mkdir(parents=True, exist_ok=True)
REPORT.write_text(json.dumps({'sections': sections, 'checked': len(checked),
                              'failures': failures, 'assets': checked}, indent=2) + '\n')
for failure in failures:
    unreal.log_error(failure)
if failures:
    sys.exit(1)
unreal.log('PRESENTATION_ASSETS_VERIFIED count=%d' % len(checked))
