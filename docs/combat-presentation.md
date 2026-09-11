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

## Named-kit assets

The clips and effect systems the W/E/R abilities will use are migrated and verified, but no ability plays them yet; each investigator's cues land with that investigator's abilities.

| Clip (`A_DM_`) | Source | Intended use |
|---|---|---|
| `KB_Projectile_1`, `KB_Projectile_Up` | Fighting Animset Pro | Medium Intercession and Beckon casting gestures |
| `KB_Block_Start`, `KB_Block_End` | Fighting Animset Pro | Smuggler Dig In brace entry and release |
| `KB_Superpunch`, `KB_SkipFwd_1` | Fighting Animset Pro | Smuggler Shoulder Through launch and carry |
| `KB_m_Backelbow_R`, `KB_GroundAttack` | Fighting Animset Pro | Smuggler counter-shove; a heavy impact held in reserve |
| `Anim_IN_check_CO` | Open World Animset | Photographer camera gesture |
| `MG_shoot` | Open World Animset | Sapper Suppressing Fire |
| `Anim_EM_call_out` | Open World Animset | Shout/marking gesture held in reserve |

`Anim_IN_check_CO` runs 21 seconds. `Action()` time-scales a clip to the duration it is given, so using it for a
0.6-second Flashbulb would play it at 35x. The Photographer slice needs a shorter gesture, a trimmed clip or a
sub-range; the asset is migrated but the mapping is not settled.

Effect systems come from `Explosions_W3Vol1` and `BigNiagaraBundle` rather than `NiagaraExamples`, whose weapon
and explosion systems each pull 20-95 MB of shared textures against 1-5 MB for the chosen equivalents; only the
4.4 MB `FX_Markers/NS_Marker_Target` is taken from that pack.

`tools/migrate_presentation.py` copies a package and its dependency closure from a read-only source project,
preserving package paths, skipping byte-identical files and refusing to overwrite a differing one.
`tools/retarget_kit_clips.py` retargets only the `kit_animations` clips through the retargeters
`retarget_presentation.py` already built, so re-running it does not rewrite the existing clips or rigs.
`tools/verify_presentation_assets.py` loads every requested package and every retargeted output and writes
`Saved/PresentationAssets.json`; it reports through that file because a python commandlet's output does not
reach the console that launched it, and a silent pass would be indistinguishable from checking nothing.

This is a provisional presentation pass. Q gestures reuse available clips; the camera grip and paired Clinch interaction need bespoke animation polish. Support-axis alignment and aiming are implemented, but finger grip poses and left-hand IK constraints are not. Actions currently play as full-body clips without an upper-body blend graph. Timing is cosmetic: animation/root motion never applies damage, spends resources or moves the gameplay actor. Transient cues use server multicasts; persistent gear/channel/down states are reconstructed from replicated gameplay state.

## Sources and reproducibility

`design/art/presentation/migration.json` records the 17 selected assets and their dependency files with SHA-256 hashes. Its `kits` section records the effect systems staged for the W/E/R abilities and `kit_animations` their source clips, each with the same hashes, the requested list and the references that did not resolve. Those unresolved names are stale editor-only links in the vendor packs (pre-conversion Cascade systems, deleted parent Niagara systems and old package roots); the editor verification is what proves a migration landed, not the scanner. Original `/Game/AnimStarterPack`, `/Game/FightingAnimsetPro`, `/Game/OpenWorldAnimset`, `/Game/Explosions_W3Vol1`, `/Game/BigNiagaraBundle` and `/Game/NiagaraExamples` package paths are retained for references. The source HoldingProject was not edited. The imported NS_Burst1 was auditioned but rejected for gameplay because its scale/lifetime overwhelmed fist impacts; the compact local burst is used instead. Imported NS_ImpactExplosion is used for satchels with a bounded cosmetic lifetime.

GameAnimationSample was inspected for locomotion/landing/traversal assets. Its motion-matching framework and traversal content are not needed by the present walk-and-fight sandbox. Directional starts/stops, turns and landing reactions are useful future additions once the movement model calls for them.

Retargeted clips live under `/Game/DreadMeridian/Presentation/Animations`; aligned props under `/Game/DreadMeridian/Presentation/Props`. Rebuild helpers are `tools/retarget_presentation.py`, `export_presentation_props.py`, `import_presentation_props.py`, `fix_presentation_materials.py`, and the muzzle/support anchor export/import scripts. Run Blender export helpers in Blender; Unreal helpers through `tools/Unreal.ps1 -Action Python -Script ...`. Retargeting is a one-time authoring recipe; review retargeter settings before regenerating existing assets. Some source props have no UV tangent data; their current constant-color materials do not use normal maps.

The developer-only `-DMPresentationProbe` produces close-up idle, attack, Q and collapse screenshots under `Saved/Screenshots`, with gameplay frozen for presentation inspection. `DreadMeridian.Foundation.CombatPresentation` checks all four native skins, attachments, camera switching, action/collapse/get-up selection, and absence of gameplay side effects. Existing BaseQ, encounter and multiplayer checks remain applicable.
