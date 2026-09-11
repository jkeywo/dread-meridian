# Investigator T-pose models and attachment points

PROVISIONAL stylized 3D interpretations of the investigator illustrations. This set includes three new characters and a Sapper update with the same attachment convention. The earlier Sapper files are preserved. No GDD status, gameplay, or runtime implementation changed.

![Roster: Sapper, Photographer, Medium, Smuggler](roster-lineup.png)

## Characters

| Character | GLB | Editable Blender | Preview | Export triangles |
|---|---|---|---|---:|
| Sapper | [Model](sapper.glb) | [Source](sapper.blend) | [Image](sapper-preview.png) | 107,465 |
| Expedition Photographer | [Model](photographer.glb) | [Source](photographer.blend) | [Image](photographer-preview.png) | 81,688 |
| Stage Medium | [Model](medium.glb) | [Source](medium.blend) | [Image](medium-preview.png) | 81,526 |
| Bare-Knuckle Smuggler | [Model](smuggler.glb) | [Source](smuggler.blend) | [Image](smuggler-preview.png) | 99,468 |

Every character has horizontal arms, open hands, and no held prop baked into its geometry. The GLBs retain named attachment nodes. Blender sources retain editable component meshes and a studio setup; studio objects are excluded from the GLBs.

## Separate items

| Item | GLB | Editable Blender | Preview | Export triangles |
|---|---|---|---|---:|
| Sapper carbine and sling | [Model](sapper-carbine.glb) | [Source](sapper-carbine.blend) | [Image](sapper-carbine-preview.png) | 2,126 |
| Photographer camera with flash | [Model](photographer-camera.glb) | [Source](photographer-camera.blend) | [Image](photographer-camera-preview.png) | 13,280 |
| Photographer precision rifle | [Model](photographer-rifle.glb) | [Source](photographer-rifle.blend) | [Image](photographer-rifle-preview.png) | 3,444 |
| Medium spirit wisp | [Model](medium-spirit-wisp.glb) | [Source](medium-spirit-wisp.blend) | [Image](medium-spirit-wisp-preview.png) | 1,988 |

The Medium's wisp is an effect concept mesh: it attaches at a palm while active and is hidden while inactive. It has no physical holster. The Smuggler is bare-knuckled and has no held item in the reference design. His attachment points reserve locations for shared props without inventing a starting weapon.

## Attachment points

The points are Blender empties exported as real named glTF nodes under each asset root. Each has an orientation as well as a position. They survive standalone export/import and can be used without guessing offsets from the rendered image.

| Character node | Intended use |
|---|---|
| `SOCKET_hand_r`, `SOCKET_hand_l` | Primary held-item placement at the anatomical right/left palm |
| `SOCKET_fx_palm_r`, `SOCKET_fx_palm_l` | Effect origin above each palm |
| `SOCKET_stow_back` | Diagonally stowed rifle/carbine |
| `SOCKET_stow_chest` | Camera carried against the chest |
| `SOCKET_holster_hip_r`, `SOCKET_holster_hip_l` | Reserved belt/holster positions for future shared equipment |

Physical items contain `ANCHOR_grip_r`, `ANCHOR_support_l`, and `ANCHOR_stow`. Rifles also contain `ANCHOR_muzzle`; the camera has `ANCHOR_optical_axis`. The spirit wisp uses `ANCHOR_emit`.

The complete per-character held/stowed state mapping is in [attachments.json](attachments.json). For example, the Photographer's `camera_held` state attaches the camera to the right hand and the rifle to the back; `rifle_held` swaps the camera to the chest. The `stowed` state places both on the torso.

Use the full transforms, not positions alone:

```text
item_root_world = character_socket_world × inverse(item_anchor_in_root)
```

[attachment_tools.py](attachment_tools.py) implements this by parenting an imported item root to the socket and applying the inverse anchor transform. Moving the socket in the example Blender scene moves the attached item with it. Each `*-locators.json` records the locator matrices in Blender coordinates.

Units are metres. In Blender the character faces -Y, up is +Z, and anatomical right is -X. Item forward is +X. glTF exports use the exporter's normal basis conversion; use loaded GLB node transforms directly and do not convert them twice.

## Checked examples

These scenes use the exported GLBs. Their item roots are parented to the appropriate sockets.

| State | Editable scene | Render |
|---|---|---|
| Sapper, carbine stowed | [Blender](examples/sapper-stowed-back.blend) | [Back view](examples/sapper-stowed-back.png) |
| Photographer, camera held | [Blender](examples/photographer-camera_held-front.blend) | [View](examples/photographer-camera_held-front.png) |
| Photographer, rifle held | [Blender](examples/photographer-rifle_held-front.blend) | [View](examples/photographer-rifle_held-front.png) |
| Photographer, both stowed | [Blender](examples/photographer-stowed-front.blend) | [Front](examples/photographer-stowed-front.png) / [Back](examples/photographer-stowed-back.png) |
| Medium, spirit active | [Blender](examples/medium-active-front.blend) | [View](examples/medium-active-front.png) |
| Four-character lineup | [Blender](examples/roster-lineup.blend) | [View](roster-lineup.png) |

[Validation results](validation.json): all eight GLBs imported; all required locators retained their hierarchy; mirrored hand positions were checked; all eight configured states and ten item placements passed, including the hidden inactive effect. Snap matrices aligned within a tolerance of 0.00001. Preview images were rendered from imported export geometry.

## Current limits

These are static, unrigged concept models. The locators are not yet skeletal sockets or an Unreal runtime attachment system. `future_bone` metadata describes intended rig parents; those bones do not exist yet. Finger poses and secondary-hand IK are not implemented, so a mounted prop in a T-pose is a placement demonstration, not a finished gripping animation. Body/prop clearance must be revisited for the final animated poses.

The Blender scenes have procedural surface detail; GLBs retain base PBR colours, roughness, metalness, and the wisp's emission/transparency. There is no unified UV atlas, baked texture set, production deformation topology, or LOD set. No Unreal import, rigging, or runtime behaviour has been claimed or tested.

## Rebuild and reuse

Built with Blender 5.0.1. `source/` contains the primitive helpers and earlier Sapper source scenes needed by the builder. No online service or paid dependency is required.

Run `build_roster.py` with one asset ID after `--`, using Blender background mode. Asset IDs are the filenames without their extensions. Rebuilding overwrites that asset in this folder. Then run `validate_assets.py` to refresh validation, and `preview_attachments.py -- photographer stowed back` to rebuild a sample state. `render_lineup.py` rebuilds the lineup. Keep transient logs outside this folder, e.g. under the repository's ignored `Saved/Art/` directory.
