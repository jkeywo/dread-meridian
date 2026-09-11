"""Retarget the W/E/R kit source clips onto Manny using the retargeters tools/retarget_presentation.py already built.

Run in the editor: tools/Unreal.ps1 -Action Python -Script tools/retarget_kit_clips.py

Deliberately narrower than retarget_presentation.py, which rebuilds the IK rigs and retargeters and re-runs every
clip in migration.json's "selected" list. Rebuilding saves those assets again and re-retargeting the existing
clips would rewrite committed binaries for no behavioural gain, so this reads the "kit_animations" section and
batch-retargets only its clips through the existing RTG_DM_<pack> assets. Appends to retarget-validation.json.
"""
import json, pathlib, unreal

ROOT = '/Game/DreadMeridian/Presentation/Animations'
REPO = pathlib.Path('C:/Coding/dread-meridian')
MANIFEST = REPO / 'design/art/presentation/migration.json'
VALIDATION = REPO / 'design/art/presentation/retarget-validation.json'

manifest = json.loads(MANIFEST.read_text())
requested = manifest.get('kit_animations', {}).get('requested', [])
assert requested, 'migration.json has no kit_animations.requested list'

target = unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
assert target
target_rig = unreal.load_asset(ROOT + '/IK_DM_Manny')
assert target_rig, 'IK_DM_Manny missing; run tools/retarget_presentation.py first'

reports = []
for pack in ['AnimStarterPack', 'FightingAnimsetPro', 'OpenWorldAnimset']:
    paths = [p for p in requested if p.startswith('/Game/' + pack + '/')]
    if not paths:
        continue
    source = unreal.load_asset('/Game/' + pack + '/UE4_Mannequin/Mesh/SK_Mannequin')
    assert source, pack
    retargeter = unreal.load_asset(ROOT + '/RTG_DM_' + pack)
    assert retargeter, 'RTG_DM_%s missing; run tools/retarget_presentation.py first' % pack
    for path in paths:
        assert unreal.EditorAssetLibrary.does_asset_exist(path), path
    inputs = unreal.IKRetargetBatchOperationInputs()
    for key, value in dict(
        assets_to_retarget=[unreal.EditorAssetLibrary.find_asset_data(p) for p in paths],
        source_mesh=source, target_mesh=target, ik_retarget_asset=retargeter,
        target_path=ROOT, prefix='A_DM_', include_referenced_assets=False,
        overwrite_existing_files=True, retain_additive_flags=False,
    ).items():
        inputs.set_editor_property(key, value)
    results = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
    assert len(results) == len(paths), (pack, len(results), len(paths))
    for entry in results:
        asset = entry.get_asset()
        # Gameplay owns movement; a retargeted clip must never displace the actor.
        asset.set_editor_property('enable_root_motion', False)
        asset.set_editor_property('force_root_lock', True)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        reports.append({'path': asset.get_path_name(),
                        'skeleton': asset.get_editor_property('skeleton').get_path_name(),
                        'length': asset.get_play_length()})

existing = json.loads(VALIDATION.read_text()) if VALIDATION.exists() else []
kept = [r for r in existing if r['path'] not in {n['path'] for n in reports}]
VALIDATION.write_text(json.dumps(kept + reports, indent=2) + '\n')
unreal.log('KIT_RETARGET_COMPLETE count=%d' % len(reports))
