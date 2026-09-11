# Smugglers — Manny concept rigs

Five original, fully three-dimensional concept meshes based on the approved
2026-09-10 illustrations: Gunman, Bruiser, Lookout, Bomber and Gang Boss.
The Lookout and Bomber have revised oval faces, tapered jaws, defined eyes,
visible face-framing hair and fitted workwear following the approved female
references. The Photographer and Medium player models received the same
appearance refinement in `../../../investigators/roster-rigged-v1/`.
Their native revised meshes are `SKM_Photographer_Refined` and
`SKM_Medium_Refined` in the existing investigator role folders. The original
native files were locked by the running editor, so the revised variants are
available alongside them; runtime actor references are not switched here.

These are **PROVISIONAL procedural concept meshes**, suitable for animation
and attachment prototyping. They are not finished production sculpts or
texture-matched reconstructions of the painted illustrations.

## Files

- `<role>-rigged.blend`: skinned mesh, editable rig, bone-parented sockets;
  opens in T-pose.
- `<role>-rigged.fbx`: Unreal skeletal mesh import source.
- `<role>-rigged.glb`: portable skinned model with `T_Pose`, `Manny_Idle`
  and `Manny_Walk` actions, including all six skin influences where needed.
- Seven separate equipment models in `.blend` and `.glb`; their `.fbx`
  import sources and editable character geometry are in `source/`.
- `attachments.json`: held/stowed states and exact socket/anchor pairings.
- `<role>-sockets.json`: calibrated native Unreal socket transforms.
- `smugglers-tpose.fbx`: shared animation-only T-pose clip for Unreal.
- `smugglers-t_pose.png`, `smugglers-stowed.png` and
  `female-character-refinements.png`: renders of the delivered geometry.

## Rig target and coordinates

Unreal **5.8.2**, existing skeleton
`/Game/Characters/Mannequins/Meshes/SK_Mannequin`.
The mesh uses the Manny Simple subset: 88 Blender bones plus the exported
root node, 89 native bones. It retains fingers, limb twists and IK helpers.
The full shared skeleton's other corrective branches are not independently
weighted. The shared skeleton reference is not modified.

The bind pose stays stock Manny's A-pose for direct animation compatibility.
The saved Blender opening pose and supplied T-pose animation have horizontal
arms. Reference-scene object-placement tracks are removed from portable clips.

Blender coordinates are metres, Z-up, character forward -Y, anatomical right
-X. Equipment points forward +X. Unreal transforms use centimetres. glTF's
axis conversion is already present in the exported nodes.

## Equipment and sockets

| Character | Separate equipment | Stowed placement |
|---|---|---|
| Gunman | Straight lever-action rifle | Back |
| Bruiser | Wooden truncheon | Right hip, hanging down |
| Lookout | Binoculars and pistol | Chest and right hip |
| Bomber | Closed grenade and satchel | Pouch point and right hip |
| Gang Boss | Drum-magazine SMG | Back |

Each character has hand, palm-effect, hip, chest and back sockets. The Bomber
also has `SOCKET_grenade_pouch`: 41 character sockets in total. Equipment has
primary grip and stow anchors; rifles, SMG and binoculars include a left-hand
support target. Firearms also have muzzle anchors. Equipment remains separate
from character skins.

Snap the **item anchor**, not just the item origin, to the character socket:

`item_root_world = character_socket_world @ inverse(item_anchor_in_root)`

The same operation applies to native mesh sockets. Stow rotations are encoded
in the prop anchors. `validate_attachments.py` reloads actual exported GLBs
and verifies these pairings throughout T-pose, idle and walk.

The `equipped/` continuation adds seven equipped pose scenes: rifle, SMG,
truncheon, binoculars, pistol, grenade and satchel. Each `.blend` contains the
skinned character, separate equipment, a baked equipped action and wrist
target controls. Rifles, SMG and binoculars have both arms solved to their
grip/support anchors. Provisional finger flexion is included; exact finger
contact and thumb placement still need an animation polish pass.

Open an equipped `.blend`, run its embedded **Equipment Controls.py** text,
move `CTRL_wrist_r` and/or `CTRL_wrist_l`, then use F3 → **Update Equipment
Arms**. The operator solves the two-link arms and keys the current action at
the current frame. Targets outside arm reach are rejected. This is an
explicit authoring operator, not a continuously running constraint or a
runtime Unreal IK solver. The held prop follows the right-hand socket;
update the left target when repositioning two-handed equipment.

Equipped `.fbx` files are animation-only pose clips, imported in Unreal under
`/Game/DreadMeridian/Characters/Enemies/Smugglers/Poses/`. The neutral character
exports and their three original GLB clips are retained. These are static
holding poses, not equip transitions or combat animations.

`equipped/controls-validation.json` verifies moving wrist targets by 1 cm,
solving and checking skin bounds in all seven saved scenes.
`equipped/unreal-equipped-validation.json` compares imported hand positions
against Blender within 0.05 cm. The seven-clip import completed with zero
errors and warnings in Unreal 5.8.2.

Equip transitions, runtime two-hand IK, combat animations, facial animation,
simulated coat cloth, LODs and gameplay equipment logic are not included.
Long coat panels currently follow body/leg weights. These files do not change
enemy gameplay.

## Verification and native assets

`deformation-validation.json`, `attachment-validation.json` and
`gltf-validation.json` report saved-file skin/clip/socket checks.
`unreal-validation.json`, `unreal-props-validation.json`,
`unreal-tpose-validation.json` and `unreal-readback-validation.json` report
native import and readback checks when the final import completes.

Native destination:
`/Game/DreadMeridian/Characters/Enemies/Smugglers/`, with role folders,
`Equipment/` static meshes and the shared T-pose animation.
Blender FBX skeletal imports can report that their bind pose was recreated;
the numeric check requires each imported reference bone to agree with stock
Manny within 0.001 cm and 0.03 degrees. Material colours are native constants;
the procedural Blender bump detail is not baked into Unreal texture maps.

Build source geometry with Blender 5.0.1 and `build_models.py -- <asset>`,
then `build_rigs.py -- <role>`, `prepare.py` and `finalize_assets.py`.
`build_models.py` reuses the repository's investigator primitive helpers.
Use `tools/Unreal.ps1 -Action Python -Script` with `import_and_verify.py`
for native imports, after building the project successfully.

The importer disables asynchronous asset compilation for this commandlet and
uses stable material names. This avoids observed Unreal background-worker
crashes and an assertion when deleting expressions from rooted materials.
The final saved-asset readback is run separately with `verify_saved.py`.
