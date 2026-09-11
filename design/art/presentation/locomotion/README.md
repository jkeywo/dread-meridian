# Movement transitions

Four GameAnimationSample clips drive short cosmetic starts, stops and left/right
turns on all four investigator skins. Sapper and Photographer use baked rifle
upper-body variants. The existing idle/jog loops remain the steady states.

Movement remains authoritative and immediate. These clips cannot move the actor,
apply damage or hold input. Speed hysteresis ignores small velocity fluctuations.
Heading changes of at least 45 degrees within a short window select a directional
turn, with a brief cooldown. Mesh yaw eases toward the new heading. Attacks, Q,
restraint, down and recovery poses take priority and clear old locomotion state.

The current layer uses single-node clips, with 0.35-second starts, 0.55-second
stops and 0.45-second turns. It is a first animation pass: it does not provide
motion matching, stride warping, foot IK or a full directional locomotion blend
graph. Small direction changes use the existing gait; large turns reuse the
90-degree left/right clips.

## Asset provenance and reproduction

Source project: `C:/Users/jkeyw/Documents/Unreal Projects/GameAnimationSample`.
`manifest.json` records FBX hashes and final assets. Original animation packages
are under `/Game/Characters/UEFN_Mannequin/Animations/Run` and `Idle`.

1. Run `tools/export_locomotion.py` in the sample's Unreal Python commandlet.
2. Run `tools/build_locomotion_reference_rig.py` in Blender 5.0.1 background mode.
   This recovers the FBX reference skeleton and binds a tiny triangle to it.
   The triangle is an authoring reference only; it never appears in the game.
   Unreal 5.8.2's source mannequin geometry exporter asserted, so this avoids
   importing the sample's large preview graph/audio dependency tree.
3. Run `tools/Unreal.ps1 -Action Python -Script tools/import_locomotion.py`.
   This imports the reference rig, retargets through IK rigs to Manny, crops
   transition windows, locks root motion, and bakes supported-rifle variants.
   Uncut retargeted references are saved and reused. Selected bone keyframes are copied into existing final outputs without deleting loaded clips.

## Validation

Foundation automation: 6 passed, 0 warnings/failures, including real component
selection for all four characters and isolated transition edge cases.
Report: `Saved/Automation/9b781d7756c4484999074669f996af08/index.json`.
Foundation smoke passed: `Saved/Logs/smoke-73d4de1d6c9947888418963e5791ab1e.log`.

Launch the game with `-DMLocomotionProbe -RenderOffscreen -unattended` to reproduce
the cosmetic lineup. It logs each selected clip and writes five screenshots to
`Saved/Screenshots/Presentation-motion-*.png`, then exits. The probe freezes
gameplay and supplies cosmetic velocities; it is not a network movement test.

Fresh-load asset validation: all eight clips passed root-lock, skeleton, crop-pose and rifle-pose checks. Run tools/validate_locomotion.py via the Unreal Python action; results are in validation.json.

