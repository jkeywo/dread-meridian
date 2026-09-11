# Smuggler territory sandbox

Implements the representative Smugglers / Bootleggers roles in GDD section 4.11.
The signature identities are design-sourced; every numerical value below is
provisional sandbox tuning. This is a native-faction elite encounter, not the
Mythos boss system, scenario director or full Break/Resolve system.

## Occupation and finale

The existing three camps and one two-person patrol remain at their authored
locations. Their compositions are:

| Group | Composition |
| --- | --- |
| South camp | Gunman, Bruiser, Lookout |
| East camp | Gunman, Bomber, Bruiser |
| North camp | Gunman, Lookout, Bomber |
| Patrol | Gunman, Lookout |
| Final wave | Gang Boss, Gunman, Bruiser, Lookout, Bomber |

All eleven occupation enemies must be defeated, in any order. Clearing them
starts a three-second public warning. The five-member final wave enters from the
east and actively hunts the squad. It does not inherit a camp leash. Killing the
boss alone is insufficient: every posse member must also fall. A squad wipe at
any stage is defeat, including during the arrival warning. The finale spawns once.

## Signatures and counterplay

| Role | Signature | Response |
| --- | --- | --- |
| Gunman | Two seconds holding a firing position gives +50% basic damage. Moving or being displaced resets it. | Push him out of position or force pursuit. |
| Bruiser | Prioritises divers near a nearby gunline; a 0.6-second close-range shove deals 8 damage and pushes up to 230 cm. | Step outside 220 cm during the windup, use cover or restrain him. |
| Lookout | Marks a visible investigator for 5.5 seconds. Nearby smugglers prefer that target and deal 20% more basic damage to them. | Kill/restrain the lookout, separate beyond his command range or protect the marked ally. |
| Bomber | A fixed 180 cm red circle warns for 1.2 seconds before 14 damage, then three 4-damage burning-ground pulses. | Leave the circle; damage never tracks the target after the warning starts. Cover blocks damage. |
| Gang Boss | Mobile ranged commander. Focus Fire lasts 4.5 seconds and outranks normal threat and lookout marks for allies within 900 cm. | Break up the formation, use cover and defeat the commander. |

Bruiser, Lookout, Bomber and Boss signature cooldowns are respectively 6.5, 8.5,
9 and 9.5 seconds. Death or restraint cancels an enemy's active signature and
removes its marker, including the bomber's lingering denial zone. Common
smugglers can be Clinched. The Gang Boss is elite and requires Break-vulnerable
state for Clinch; this change does not implement a full Break system.

| Role | Health | Basic damage | Basic interval | Attack range |
| --- | ---: | ---: | ---: | ---: |
| Gunman | 180 | 7 | 1.6 s | 700 cm |
| Bruiser | 260 | 10 | 1.2 s | 155 cm |
| Lookout | 150 | 5 | 1.6 s | 600 cm |
| Bomber | 150 | 5 | 1.6 s | 550 cm |
| Gang Boss | 750 | 9 | 0.9 s | 700 cm |

Enemy basics retain their 0.3-second visible windup. Companions now step out of
bomber circles (the `DreadMeridian.Editor.UtilityAI` PIE check asserts it) and a
lone lookout no longer marks because the mark is worth less than its conservation
threshold (provisional). The squad still does not prioritise support roles; the
unassisted run is a mechanics/soak check, not a claim of balanced difficulty.

## Presentation and replication

Uses the supplied five skeletal meshes in
`/Game/DreadMeridian/Characters/Enemies/Smugglers`, with their authored weapon
grip/muzzle/support anchors. Binoculars and bomb satchels are stowed. Movement,
attacks and death reuse the existing retargeted animation library; signature
gestures are provisional. `tools/fix_smuggler_materials.py` saves skeletal-mesh
material usage after an art reimport.

Combat decisions, damage, cooldowns and wave advancement run on authority.
Clients receive role, observable signature state, order targets, markers, actor
movement and the encounter objective. Animation/particles cannot apply damage.
Developer capture adds `smuggler.signature` and `encounter.wave` events and
records late `combat.spawned` events for the posse. Capture version is 0.3.0;
the generic envelope remains version 1. This does not implement replay.

## Reproduction and evidence

- `tools/Unreal.ps1 -Action Test`: seven foundation tests passed.
- Full Unreal automation: eleven passed, zero warnings/failures, including the
  real PIE `DreadMeridian.Editor.SmugglerFaction` scenario. Report:
  `Saved/Automation/Smugglers/index.json`.
- The PIE scenario checks all four occupation roles, held meshes, set-position
  reset, mark pressure, bomb avoidance and area damage, interruption, shove,
  gated arrival, one elite boss, Focus Fire threat override and complete-posse
  victory. It uses controlled actor placement/health and is not a balance run.
- Foundation smoke passed and its six-event capture passed
  `tools/validate_capture.py --require-provenance`.
- All seven Python export-contract tests passed.
- `tools/Unreal.ps1 -Action SmugglerSmoke`: runs four unassisted bots against the
  complete encounter with provenance and capture. Victory or defeat is a valid
  completion; failure to finish within 180 seconds fails the check. Read event
  files only after completion: Windows readers can temporarily block append.
- `-DMSmugglerProbe -RenderOffscreen -unattended`: a cosmetic lineup probe writes
  `Saved/Screenshots/Smugglers-idle.png`, `-attack.png` and `-signatures.png`.
  The five role meshes, equipment, colours and effects were visually inspected.

The compact seven-actor legacy combat/network profiles are retained for existing
regression tests. Normal PIE and game play use the full smuggler encounter.

Final unassisted capture: Saved/Playtrace/62d1ad4a-457e-caab-5b24-c4b7714adf60/events.jsonl. Play Trace accepted all 1,227 events; all five signatures occurred, the eleventh occupation kill led to an arrival announcement at tick 583, and the boss/posse spawned at tick 613. The squad was defeated at tick 665 with the finale present. Summary: smuggler-validation.json. The additional legacy multiplayer recheck could not run because a concurrent art-generation process held its source log open during provenance hashing; no new multiplayer verification is claimed.

