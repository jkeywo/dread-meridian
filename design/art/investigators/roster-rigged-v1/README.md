# Investigator animation rigs — provisional v1

Sapper, Photographer, Medium and Smuggler are skinned to the existing Unreal 5.8.2 **SK_Mannequin** skeleton used by Manny. Native skeletal meshes and materials are imported under `/Game/DreadMeridian/Characters/Investigators/<Character>/SKM_<Character>`.

Each character includes an editable `*-rigged.blend`, a skeletal-mesh `*-rigged.fbx` for Unreal, and an animated `*-rigged.glb`. Blender files open in the selectable **T_Pose** action. The underlying bind pose matches the stock skeleton; **Manny_Idle** and **Manny_Walk** are also available as actions and GLB clips. FBX contains the mesh and bind skeleton without duplicate animation assets; use the existing stock Unreal clips.

The 89-node Manny Simple hierarchy is retained (88 Blender bones plus the armature object exported as `root`), and assigned directly to the project's 161-bone shared skeleton. Spine, neck, fingers, limb twists and IK helper branches are present. The extra corrective branches of the full mannequin are not newly skinned or authored. Character limb lengths are fitted to the stock reference while retaining their individual clothing, heads and body volume.

## Attachments

All four characters have eight real bone-parented attachment nodes and eight native **mesh-specific Unreal sockets**:

| Sockets | Bone |
|---|---|
| `SOCKET_hand_r`, `SOCKET_fx_palm_r` | `hand_r` |
| `SOCKET_hand_l`, `SOCKET_fx_palm_l` | `hand_l` |
| `SOCKET_holster_hip_r`, `SOCKET_holster_hip_l` | `pelvis` |
| `SOCKET_stow_back`, `SOCKET_stow_chest` | `spine_05` |

The separate carbine, camera/flash, straight rifle and spirit-wisp models are included. Their grip, support, stow and muzzle/effect anchors retain their stable names. `attachments.json` maps every held/stowed state. Align the item **anchor**, not its object origin:

`item_root_world = character_socket_world × inverse(item_anchor_in_root)`

Per-character socket JSON includes Blender bone-local matrices and the Unreal reference-world transforms used to create the native sockets. Unreal uses centimetres; Blender/glTF presentation uses metres. Socket scale is one. The wisp becomes hidden when inactive; the Smuggler remains unarmed.

## Validation

Appearance revision, 2026-09-10: the Photographer and Medium have narrower,
softer jaw/chin shapes, refined almond eyes and brows, slimmer necks and more
fitted costume silhouettes following their approved female references.
Their curls, goggles, silver forelock, velvet costume and equipment are
preserved. Both meshes were reskinned to the same stock reference; their
portable clips no longer include the reference scene's placement offset.
Native appearance revisions are saved as `SKM_Photographer_Refined` and
`SKM_Medium_Refined` alongside the original role assets, which were locked by
the running editor. The revised standalone exports keep their existing
filenames. Runtime references are not switched to the new variants here.

- All vertices weighted and normalized; up to six influences, including stock limb twists. GLB retains its second weight set.
- Three animation clips and one skin per GLB.
- Idle and walk sampled at nine points each, plus T-pose: finite deformation and stable bone-relative sockets on all four characters. See `deformation-validation.json`.
- Native import uses the existing `SK_Mannequin`. Maximum reference difference is below **0.001 cm** translation and **0.03 degrees** rotation. See `unreal-validation.json`.
- Saved native meshes reloaded successfully: 89 bone nodes each, 32 sockets total, all 69 material slots assigned; both stock clips evaluated successfully in Unreal. See `unreal-readback-validation.json`.
- Unreal editor module built successfully. The final readback commandlet completed with zero errors and warnings. FBX import still reports a bind-pose reconstruction warning; Unreal reconstructs it successfully, and the resulting reference transforms pass the numeric comparison above.

These are animation-ready **concept skins**, not finished production characters. Cloth simulation, facial rigs, custom grip poses, two-hand IK constraints and animation polish are not authored. Long coats currently follow leg/pelvis weights. The rigs are imported art assets; this change does not switch the playable runtime character visuals or add animation gameplay. Runtime Test/Smoke were not required for these asset/editor-only changes.

## Rebuild

Use Blender 5.0.1 and the repository's pinned Unreal wrapper. `export_manny.py` exports the stock idle/walk references without preview meshes. `inspect_reference.py` creates their initial Blender reference. `calibrate_reference.py` fits that reference to `stock-skeleton-transforms.json` using the preserved initial FBX round-trip measurement in `fbx-calibration-source.json`; run it once after a fresh inspection, not repeatedly on the calibrated file.

Run `build_rigs.py -- <character>` for each character, followed by `finalize_assets.py` and `validate_rigs.py`. The builders consume the previous `roster-3d-v1` source assets. `render_rig.py -- <character> --quick` renders the walk preview, and `render_lineup.py` renders the roster.

Run the native importer and readback through:

```powershell
./tools/Unreal.ps1 -Action Python -Script design/art/investigators/roster-rigged-v1/import_unreal.py
./tools/Unreal.ps1 -Action Python -Script design/art/investigators/roster-rigged-v1/verify_unreal.py
```

`DMArtRigLibrary` belongs to the editor module and creates mesh-only sockets; it does not add sockets to the shared stock skeleton. The stock reference skeleton and sample clips originate from the project's Epic mannequin assets. Original static character/prop assets remain in `roster-3d-v1`.
