# Sapper T-pose and separate carbine — v2

PROVISIONAL static concept assets. The original listening-pose files in `../sapper-3d-v1/` are preserved.

| Asset | Portable model | Editable source |
|---|---|---|
| Sapper, neutral T-pose | [GLB](sapper-tpose.glb) | [Blender](sapper-tpose.blend) |
| Separate carbine and sling | [GLB](sapper-carbine.glb) | [Blender](sapper-carbine.blend) |

The Sapper faces forward with symmetric feet, horizontal arms, and open hands with palms down. The character files contain no gun or sling. The gun files contain no character geometry. Each Blender file retains separate editable component geometry and a studio setup; each GLB contains only its asset geometry and root.

Both assets use metres. The carbine's local origin is at the receiver/grip region, with the muzzle along +X and the top along +Z in Blender. The glTF exporter performs the standard coordinate-system conversion. The carbine's magazine attachment and sling curve have been adjusted for the separate asset.

These are unrigged static models. A T-pose does not add a skeleton, skin weights, animation, production retopology, or game integration. Base PBR materials export to GLB; procedural Blender micro-bump is not baked.

[Character preview](sapper-tpose-preview.png) · [Gun preview](sapper-carbine-preview.png)

[Character after GLB reimport](sapper-tpose-glb-preview.png) · [Gun after GLB reimport](sapper-carbine-glb-preview.png) · [Validation results](validation.json)

`build_tpose.py` derives the model from the unchanged v1 source script and generates this version. It also leaves a temporary combined working scene under `Saved/Art/Sapper/`. `validate_models.py` checks the two exported hierarchies, dimensions, horizontal symmetric hand positions, and clean GLB imports, then renders the imported assets. Run both scripts with Blender 5.0 in background mode. Rebuilding overwrites the v2 generated outputs. No Unreal gameplay or runtime files were changed.
