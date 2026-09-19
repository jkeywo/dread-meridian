# Fixed fishing-village scenario

Fixed enemy placement, timed Ritual pressure and explicit or premature manifestation.
This is a fixed scenario integration, without HTN.
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
Idols. Completing all four core stages and all three Disruptions unlocks
Force Manifestation at the basin: approach and press I to channel for three
seconds, or continue preparing. This summons the existing Shub encounter once.
Shub's death wins after mandatory work; a team wipe loses even before manifestation. Clearing ordinary enemies does not win the scenario.

The lighthouse, surgery and smuggler cache are optional. They grant their native
vision, treatment and shared relic rewards. Food has fixed authored locations.
These opportunities now compete with the advancing Ritual for time.

The run stays in Expedition during objective work, then enters Apocalypse at
manifestation. Shub is fixed before Madness resonance assignment. The named boss
draw is still consumed; scenario metadata records boss selection version 2 and
combat rules `fishing-village-ritual-v2`.

## Ritual integration (provisional v2)

The server advances one Ritual point per 30 accepted combat ticks (three seconds).
At the default 100 points per stage this gives five minutes per stage and natural
Apocalypse after twenty minutes. Paused simulation does not advance the clock.
Repeated/out-of-order clock calls add nothing; excess ticks carry forward.

Unfinished mandatory objectives become harder with the public stage: longer work,
longer real bell sequences, slower escort travel, additional authored ward/bell
sites, and the existing surviving-idol protection aura. Completed steps and rewards
stay complete. Failed mandatory work is repaired with twenty extra work ticks per
remaining step and ten Ritual points, once per failure; this is a fixed repair,
not a generated HTN replacement. No deadlines are added by this integration.

Natural Apocalypse manifests Shub even with the basin flooded. In that case the
boss arena uses the dry southern approach; the pump still needs completing to
open the basin. Existing and subsequently spawned core objectives become harder
Apocalypse versions. The objective HUD continues to show mandatory work and
explains that Shub endures until it is complete. Provisional damage gating keeps
Shub at at least one health while mandatory work remains, preserving active boss
pressure and preventing a premature victory. Completed rites release that gate.
The five-stage Ritual HUD remains visible throughout the scenario.

This step does not implement the full encounter director, mixed candidate-boss
intrusions, stage-driven scenery transformation or a random mission planner.

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
actual collision removal, routing, clock boundaries, failed-objective repair,
explicit summon choice, premature manifestation, future objective conversion,
mandatory-work boss gating, one-time manifestation and both routes to boss victory.
It relocates/pauses actors and applies test damage: it is an integration test,
not a four-bot playthrough, balance result or proof of natural traversal.

Full-map fog, Ritual encounter direction, generated/repairable HTN plans, the
Nyarlathotep scenario, final environment art and full-run migration remain
outside this step. Test evidence must be reported separately after execution.

Validation is recorded in `fishing-village-validation.json`. An isolated source
copy under `Saved/VillageVerification` was built because an existing editor held
the project DLLs. It shares the real project Content directory; the generated
map and materials are in the main project. The retained overview is an actual
Unreal render. The integration test drives accepted interactions and test damage;
the networking and shell regression runs use their existing sandbox fixtures,
so they do not establish a two-client village playthrough or autonomous victory.

Ritual v2 validation is recorded in `ritual-scenario-validation.json`. The current
Win64 project built directly under Unreal 5.8.2; both village routes and the
objective catalogue passed PIE integration tests. The earlier isolated-build
record above describes the original village integration only.
