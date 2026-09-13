# Supplied character rigs

Nine accepted user-supplied meshes replace the earlier procedural game models. Original input SHA-256 hashes are recorded in `source-inventory.json`. The committed art includes the final rigs, texture maps, sockets, previews and validation. Redundant raw input copies, intermediate models, previous-native backups and duplicate delivery ZIPs were removed after verifying that the original Downloads inputs and prior Git assets remain recoverable.

## Character mapping

| Supplied file | Character |
|---|---|
| historical+soldier+3d+model.zip | Player Sapper |
| female+adventurer+3d+model.zip | Player Photographer |
| fantasy+wizard+character+3d+model.zip | Player Medium |
| character+3d+model.zip | Player Smuggler |
| cowboy character 3d model.glb | Enemy Gunman |
| pirate 3d model.glb | Enemy Bruiser |
| adventurer female 3d model.glb | Enemy Lookout |
| female explorer 3d model.glb | Enemy Bomber |
| vintage gangster 3d model.glb | Enemy Gang Boss |

## Delivered

Each character has `*-rigged.blend`, `*-rigged.fbx`, `*-rigged.glb`, `*-sockets.json` and transparent T-pose/walk preview PNGs. Blender and GLB contain packed textures and T_Pose, Manny_Idle and Manny_Walk actions. FBX contains the mesh and reference-pose skeleton; texture files are under `textures/`.

The rigs use the existing stock Manny reference hierarchy and axes: 88 Blender bones, plus the Unreal root object. All vertices are weighted, with at most eight influences. Existing named hand, stowed-item and effect sockets are retained. Held weapons remain the existing separate game assets. Source UVs are retained; coincident vertices were welded before skinning to avoid cracks at texture seams. Joint weights are smoothed and the torso uses a consistent bind-space fit.

## Validation and completed integration

`blender-validation.json` records 15 sampled poses per character, finite/bounded deformation, weighted vertices, bone-parented sockets, three GLB clips and embedded texture presence. Walk and T-pose renders were visually inspected, including corrections to sleeve weights and seams.

**Native replacement completed on 2026-09-11 in Unreal 5.8.2.** All nine canonical mesh paths already referenced by DMCombatPresentation now contain the supplied rigs. Previous native mesh files are recoverable from Git history before commit `88ba447`. Supplied materials/textures and existing attachment sockets were saved and read back in a fresh Unreal process.

Runtime visual scale is 1.10 for Sapper, Gunman and Gang Boss, and 1.20 for Smuggler and Bruiser. The other characters use 1.00. Equipment inherits the visual scale, while gameplay collision and movement remain unchanged. Standalone rigs retain stock reference scale.

The initial import omitted the materials' skeletal-mesh usage flag, which made Unreal display its default material in game despite valid texture assignments. `fix_material_usage.py` repairs the nine materials; the importer now sets the flag and the readback check requires both skeletal usage and a base-colour texture. The original integration checks below did not detect that defect.

After the correction, readback with `-RenderOffscreen` passed with zero errors and warnings, including all nine saved skeletal usage flags and base-colour graph connections. Foundation tests passed (`Saved/Automation/b6f2127b6d5e4c75b31a8f32976547a9/index.json`) and smoke passed (`Saved/Logs/smoke-4eaae136563c4ff0acfbdeddc15ae1c0.log`). Detailed command output is local under `Saved/supplied-material-{fix,verify,tests,smoke}.log`.

`unreal-validation.json` records 89 native bones per character and reference-pose agreement within 0.018 degrees and 0.000744 cm. Unreal reconstructed the FBX bind poses during import; the resulting transforms passed the numeric checks. `unreal-readback-validation.json` verifies all nine saved meshes, materials, sockets, and idle/walk/rifle-idle clip evaluation.

The required `tools/Unreal.ps1 -Action Test` and `-Action Smoke` both passed after replacement. Foundation report: `Saved/Automation/6f8e979b21ae4c969a40340f627150fb/index.json`. Smoke log: `Saved/Logs/smoke-fa9e05023b8b4442b0952eb958d118c5.log`. Command logs: `Saved/supplied-unreal-import.log`, `Saved/supplied-unreal-readback.log`, `Saved/supplied-unreal-tests.log`, `Saved/supplied-unreal-smoke.log`.

## Limits

These are skinned supplied concept meshes, not a finished animation set. Dense clothing folds and long coats still need pose-specific polish; cloth simulation and facial expression controls are not included. Hands are weighted to the Manny finger hierarchy, but the supplied fused/coarse finger topology limits individual-finger articulation. The source colour textures contain painted shading. No missing normal/roughness maps were invented for files that only supplied base colour.

## Rebuild

Blender 5.0.1 Python: inspect_sources.py (copies the named Downloads inputs), prepare.py, build_rig.py -- CHARACTER for each manifest key, then finish.py. Use --python-exit-code 1 and stop on errors. The stock reference file is `../investigators/roster-rigged-v1/manny-reference.blend`.

## Smuggler bind correction (2026-09-13)

The apparent retargeting problem was a source-pose fitting error. All 71 presentation clips already target the stock Manny skeleton. A fresh UE4-to-Manny retarget of the fighting idle, right jab and superpunch produced the same sampled joint positions as the existing clips. The supplied Smuggler appears to be in a T-pose from the front, but its arms bend substantially forward in the top view. The previous planar landmarks placed its torso, elbows, wrists and fingers outside the actual geometry, producing inflated sleeves and displaced hands during animation.

`build_rig.py` now uses Smuggler-specific three-dimensional landmarks and inverts the blended forward skin transform per vertex. The stock Manny reference skeleton, runtime animation assets, eight attachment sockets, texture maps and 1.20 runtime scale remain intact. The correction applies only to the Smuggler. Before/after renders of three native fighting clips, the source top view, skeleton audit and import reference-transform checks are retained in `smuggler-bind-repair/`. These renders show a substantial improvement; coarse finger topology and clothing still limit close-up articulation, and facial/cloth animation remains outside this change.

To rebuild this character alone, extract the original `character+3d+model.zip` into an ignored working folder, then run Blender with `--python-exit-code 1 --python prepare_smuggler_source.py -- PATH_TO_EXTRACTED_FBX`. This reuses the accepted packed materials and writes a normalized GLB under `Saved/SmugglerAnimationProbe`. Run `build_rig.py -- smuggler --prepared-dir C:/Coding/dread-meridian/Saved/SmugglerAnimationProbe`, then `finish.py -- smuggler`. Import the resulting FBX at the canonical Smuggler path with the stock skeleton, no skeleton reference-pose update, and retain its sockets and material. Material slot assignment must be saved in a fresh Unreal process after import.

Saved-asset readback passed for all nine supplied characters, including textures, skeletal material usage, sockets and animation evaluation. All 37 foundation tests passed at `Saved/Automation/98b66e10bf6f41cb931ba1b8dcef8d20/index.json`. Unreal reconstructed the FBX bind pose; the imported reference transforms passed the numeric checks. Existing ChaosNiagara/plugin warnings remain in the commandlet output.
Smoke also passed: Saved/Logs/smoke-1167ed465ae7442f8900bb658f56aa92.log.
