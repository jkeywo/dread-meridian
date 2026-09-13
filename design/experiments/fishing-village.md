# Fixed fishing-village scenario

User-authorized integration: fixed enemy placement and automatic boss summoning
after the core objectives, without waiting for a timed Ritual system or HTN.
The GDD remains unchanged. Locations, durations, health and encounter counts are
provisional scenario tuning.

## Composition

`L_FishingVillage` owns a native greybox village: landing jetty, fishing hut,
store, boathouse, church/graveyard, lighthouse, waterworks, marsh and ritual basin.
Nine fixed enemies use the existing five Swamp Thing roles and four Smuggler
roles. Three static marsh idols belong to the Disruption objective. No Local
Cult-specific role implementation is supplied by this integration.

The fixed chain is Impossible Catch -> Waterworks -> Main Pump -> Counter-Sigil.
Only the current core stage is spawned. Completing the pump removes the flooded
basin's collision and visual surface, exposing the mudflat and ritual stones.
The three required Disruptions are Bell Sequence, Counter-Ritualist and Marsh
Idols. Completing all four core stages and all three Disruptions summons the
existing Shub encounter once. Shub's death wins; a team wipe loses even before
manifestation. Clearing ordinary enemies does not win the scenario.

The lighthouse, surgery and smuggler cache are optional. They grant their native
vision, treatment and shared relic rewards. Food has fixed authored locations.
These opportunities have no time-pressure tradeoff until Ritual escalation exists.

The run stays in Expedition during objective work, then enters Apocalypse at
manifestation. Shub is fixed before Madness resonance assignment. The named boss
draw is still consumed; scenario metadata records boss selection version 2 and
combat rules `fishing-village-v1`. No timed escalation or random mission planner
is implied by the underlying run-state scaffold.

## Movement and presentation

An authored rectangle visibility graph routes click movement and bots around
solid buildings, deep marsh and the undrained basin. Drainage changes both the
collision and this route graph. This is a small native 2D route service, not an
Unreal NavMesh or full navigation acceptance claim. Reeds slow movement to 70%
and use the existing concealment service. All nine fixed enemies opt into vision;
the current broader hazard/corpse/perception fog limitations remain.

Known objectives appear on the minimap, with current task and gate progress in
the HUD. `I` interacts, `1/2/3` enters bell symbols. Human-led companions retain
their combat policy and leader tether. With no living human leader, bots can
approach and operate the fixed objective sequence between nearby fights. This
is a fixed sequence driver, not HTN or a claim of reliable autonomous victory.

## Launch and verification

`tools/Unreal.ps1 -Action GenerateVillage` authors the native map once, preserving
subsequent edits. `-Action Play` opens the village; `-Action Play -Sandbox` keeps
the old encounter available. The shell's normal Launch Expedition loads the
village. The existing ShellSmoke deliberately retains its compact sandbox probe.

`DreadMeridian.Editor.FishingVillage` loads the real map and drives accepted
objective interactions, checking order, optional rewards, all required gates,
actual collision removal, routing, one-time manifestation and boss victory.
It relocates/pauses actors and applies test damage: it is an integration test,
not a four-bot playthrough, balance result or proof of natural traversal.

Full-map fog, dynamic Ritual escalation, generated/repairable HTN plans, the
Nyarlathotep scenario, final environment art and full-run migration remain
outside this step. Test evidence must be reported separately after execution.

Validation is recorded in `fishing-village-validation.json`. An isolated source
copy under `Saved/VillageVerification` was built because an existing editor held
the project DLLs. It shares the real project Content directory; the generated
map and materials are in the main project. The retained overview is an actual
Unreal render. The integration test drives accepted interactions and test damage;
the networking and shell regression runs use their existing sandbox fixtures,
so they do not establish a two-client village playthrough or autonomous victory.
