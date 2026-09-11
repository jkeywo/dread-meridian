# Sapper — 3D concept v1

PROVISIONAL art concept based on the Sapper reference illustration at `../2026-09-10-v1/sapper-full.png`. This is a stylized modeled interpretation, not a photogrammetric reconstruction. It does not change the GDD or establish final character appearance.

- [Editable Blender scene](sapper-concept.blend): separate named geometry, curve details, materials, lights, and a framed camera. Created in Blender 5.0.1. Units are metres; the character is approximately 1.91 m tall.
- [Portable GLB model](sapper-concept.glb): reduced static character geometry with PBR base materials. Studio ground, camera, and lights are excluded.
- [Front render](sapper-preview.png), [face detail](sapper-detail.png), and [back render](sapper-back.png) show the Blender source.
- [GLB round-trip render](sapper-glb-preview.png) shows the exported model after reimport, so its appearance can be checked separately.
- [Validation report](validation.json) records the final export triangle count, materials, dimensions, and import check.

The listening pose, heavy olive greatcoat, aged angular face, grey hair, leather harness, cartridge pouches, demolition satchel, wire spool, and diagonal carbine follow the illustration. Back details and hidden surfaces are design interpretations.

This is a static, unrigged concept with intersecting component geometry. It has no animation, deformation retopology, unified UV atlas, baked texture maps, or production LODs. Blender procedural micro-bump is not baked into the GLB; the GLB retains base material colours, roughness, and metalness. It is not prepared as a watertight print. No Unreal integration or runtime changes were made or tested.

## Rebuild

Run `build_sapper.py` with Blender in background mode, then run `validate_export.py` the same way. Rebuilding replaces the generated deliverables in this directory. Keep transient logs under `Saved/Art/Sapper/`.

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.0/blender.exe' --background --factory-startup --python 'C:/Coding/dread-meridian/design/art/investigators/sapper-3d-v1/build_sapper.py'
& 'C:/Program Files/Blender Foundation/Blender 5.0/blender.exe' --background --factory-startup --python 'C:/Coding/dread-meridian/design/art/investigators/sapper-3d-v1/validate_export.py'
```
