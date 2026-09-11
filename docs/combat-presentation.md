# Investigator combat presentation

The four playable investigators use the supplied `roster-rigged-v1` Manny-compatible native meshes. Sapper holds the carbine; Photographer holds the rifle with the camera on his chest, switching to camera-held/rifle-on-back during Frame. Medium's wisp appears with supernatural activity. Smuggler remains unarmed. Weapons use authored grip, support, stow and muzzle anchors; game actor collision is unaffected by gear.

## Animation and effects

| Use | Current presentation |
|---|---|
| Armed movement and idle | Animation Starter Pack rifle idle/jog retargeted to Manny |
| Carbine / precision rifle | Hip / ironsights firing, muzzle flash and short tracer from the weapon muzzle |
| Bare-knuckle basics | Alternating Fighting Animset Pro jabs, compact warm impact sparks |
| Spirit Lash | Retargeted casting gesture and violet travelling particles |
| Satchel Q | Throw/place gesture, persistent armed marker, bounded Niagara explosion |
| Frame Q | Camera equipped, steady framing pose and existing repeated flash feedback |
| Bind Q | Casting gesture, hand wisp, persistent spirit markers and radius rings |
| Clinch / throw Q | Hold pose and throwing gesture with impact burst |
| Wounded | Brief hit reaction and restrained red particles |
| Down/death | Non-looping collapse retained at its final pose |
| Revive | Get-up clip; a looping interaction pose is available for revive projection |

This is a provisional presentation pass. Q gestures reuse available clips; the camera grip and paired Clinch interaction need bespoke animation polish. Support-axis alignment and aiming are implemented, but finger grip poses and left-hand IK constraints are not. Actions currently play as full-body clips without an upper-body blend graph. Timing is cosmetic: animation/root motion never applies damage, spends resources or moves the gameplay actor. Transient cues use server multicasts; persistent gear/channel/down states are reconstructed from replicated gameplay state.

## Sources and reproducibility

`design/art/presentation/migration.json` records the 17 selected assets and their dependency files with SHA-256 hashes; its `kits` section records the effect systems staged for the W/E/R abilities (42 packages, 35 MB) with the same hashes, the requested list and the references that did not resolve. Those unresolved names are stale editor-only links in the vendor packs (pre-conversion Cascade systems and deleted parent Niagara systems); `tools/verify_presentation_assets.py` loads every requested package in the editor so a genuinely broken import cannot pass unnoticed. `tools/migrate_presentation.py` copies a package and its dependency closure from a read-only source project, preserving package paths and refusing to overwrite a differing existing file. The NiagaraExamples weapon and explosion systems were measured and rejected for size: each drags 20-95 MB of shared textures, against 1-5 MB for the Explosions_W3Vol1 and BigNiagaraBundle systems chosen instead. Original `/Game/AnimStarterPack`, `/Game/FightingAnimsetPro`, `/Game/OpenWorldAnimset`, `/Game/Explosions_W3Vol1`, `/Game/BigNiagaraBundle` and `/Game/NiagaraExamples` package paths are retained for references. The source HoldingProject was not edited. The imported NS_Burst1 was auditioned but rejected for gameplay because its scale/lifetime overwhelmed fist impacts; the compact local burst is used instead. Imported NS_ImpactExplosion is used for satchels with a bounded cosmetic lifetime.

GameAnimationSample was inspected for locomotion/landing/traversal assets. Its motion-matching framework and traversal content are not needed by the present walk-and-fight sandbox. Directional starts/stops, turns and landing reactions are useful future additions once the movement model calls for them.

Retargeted clips live under `/Game/DreadMeridian/Presentation/Animations`; aligned props under `/Game/DreadMeridian/Presentation/Props`. Rebuild helpers are `tools/retarget_presentation.py`, `export_presentation_props.py`, `import_presentation_props.py`, `fix_presentation_materials.py`, and the muzzle/support anchor export/import scripts. Run Blender export helpers in Blender; Unreal helpers through `tools/Unreal.ps1 -Action Python -Script ...`. Retargeting is a one-time authoring recipe; review retargeter settings before regenerating existing assets. Some source props have no UV tangent data; their current constant-color materials do not use normal maps.

The developer-only `-DMPresentationProbe` produces close-up idle, attack, Q and collapse screenshots under `Saved/Screenshots`, with gameplay frozen for presentation inspection. `DreadMeridian.Foundation.CombatPresentation` checks all four native skins, attachments, camera switching, action/collapse/get-up selection, and absence of gameplay side effects. Existing BaseQ, encounter and multiplayer checks remain applicable.
