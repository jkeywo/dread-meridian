# Supplied Swamp Thing and Shub models

Source GLBs supplied by the user on 2026-09-13. Original inputs are preserved in `source/`; `source-inventory.json` records SHA-256 hashes, mesh counts, images and bounds. All seven inputs were unrigged and contain one 4096px base-colour texture; no extra texture maps were invented.

| Input | Runtime role | Presentation |
|---|---|---|
| crawler.glb | Swamp Crawler | Supplied resting static mesh |
| lurker.glb | Swamp Lurker | Manny skeletal mesh |
| spitter.glb | Swamp Spitter | Supplied resting static mesh |
| grasper.glb | Swamp Grasper | Manny skeletal mesh |
| oldThing.glb | Swamp Old Thing | Manny skeletal mesh |
| broodling.glb | Shub Broodling | Supplied resting static mesh |
| blackgoat.glb | Spawn of the Black Goat | Manny skeletal mesh |

`manifest.json` maps inputs to the native Unreal asset paths. Humanoids have editable packed-texture Blender rigs, skeletal FBX files and GLB exports containing T_Pose, Manny_Idle and Manny_Walk. Runtime uses the existing Manny idle, jog, melee, hit, death and get-up animation library. Four rigs use the stock 88-bone Blender hierarchy plus the Unreal root object. Source T-poses are fitted into the stock reference skeleton, so limb proportions are adjusted for stock animation compatibility. No weapons or equipment sockets are needed for these creatures.

Lurker and Grasper use a runtime visual scale of 1.10; Old Thing and Black Goat use 1.45. Crawler, Spitter and Broodling meshes are normalized to 1.0m, 1.25m and 0.9m high respectively. These are presentation choices, not balance or collision changes. Original meshes and textures are preserved.

`DMCombatPresentation` selects the supplied art from replicated Swamp role or Shub minion kind, clears human equipment and hides the placeholder skeletal mesh for nonhumanoids. Nonhumanoids remain unanimated resting meshes while moving; their bespoke locomotion, feeding and splitting animation rigs are future work. Humanoids are provisional concept skins: large hands/feet, hanging algae, shoulder growths and the Grasper's rear geometry need further deformation polish; facial animation is not included.

## Rebuild and validation

Use Blender 5.0.1 with `--python-exit-code 1`. Run `inspect.py`, `prepare.py`, `build_rig.py -- NAME` for each humanoid, and `finish.py`. The build/finish wrappers reuse the existing supplied-character Manny pipeline. Run the Unreal 5.8.2 wrapper with `-Action Python -Script design/art/creature-models-v1/import_unreal.py`, then run `assign_materials.py` in a fresh process and `verify_unreal.py -RenderOffscreen` via the same wrapper. The separate material assignment pass is required: FBX import can restore empty slot assignments after its initial save. Materials explicitly enable skeletal usage; static meshes explicitly populate their material slots.

`blender-validation.json` checks 15 sampled poses per humanoid, normalized bounded weights, finite deformation, UVs and packed GLB textures. `unreal-readback-validation.json` checks saved texture connections, skeletal usage, stock reference bone transforms and native animation evaluation. The foundation `CreaturePresentation` test exercises all seven runtime selections, human-gear cleanup, visibility, animation transitions and unchanged health/actor scale.

Completed 2026-09-13: saved-asset readback with rendering enabled passed; foundation tests passed (`Saved/Automation/96b55e1b38a14b65bfa9e10d131f39ff/index.json`); smoke passed (`Saved/Logs/smoke-d16b227de6dc4e319997961906184425.log`). FBX bind poses were reconstructed on import and verified against the stock skeleton. The rendering-enabled commandlet reports existing Niagara plugin warnings unrelated to these assets, including a missing ChaosNiagara script import; this is not a claim that every third-party effect is functional.
