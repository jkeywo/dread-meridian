# Swamp enemy vision integration

Scope: native vision entitlement for Swamp Things, reed concealment for Lurkers,
and the lighthouse objective's persistent vision reward. No new art is included.
GDD 3.3, L.3 and the Swamp Thing role definitions remain authoritative; sight
ranges and reveal durations below are provisional fixture tuning.

## Rules and boundaries

- Swamp Things opt into vision filtering. Investigator sight is shared by the
  living team, extends 1500 units and respects world geometry. Enemy observers
  use their own sight. Other existing enemy families retain their existing
  visibility until explicitly opted into the same service.
- Enabled reed areas conceal a waiting Lurker beyond 140 units. A nearby living
  investigator shares detection with the whole team. Commitment reveals it;
  recovery holds that reveal until its cooldown expires. Accepted damage reveals
  it for 30 combat ticks, allowing blind area attacks to expose an ambusher.
  A Lurker outside commitment range stalks toward a seen target instead of
  waiting indefinitely for a team that cannot see it.
- The lighthouse creates a persistent 5000-unit source, including detection in
  reeds. It supplies vision only. Completion/restoration is idempotent, and
  restoring an unfinished objective disables its previously granted source.
  Sources and reeds have native enabled/radius properties for authored changes.
- Known terrain and objective markers remain available for planning. Swamp enemy
  actor channels and signature tells use server-side connection relevancy.
  Owner-delivered visible-ID projections hide previously seen actors immediately
  on the client while their Unreal actor channels age out. Cached historical
  information cannot be erased from a client that previously saw an actor.
  Publication runs in the controller actor tick, including remote server
  controllers that Unreal excludes from the local-input PlayerTick path.
- Server basic/targeted ability checks, ongoing framing, target pings and bot
  inputs respect entitlement. Blind ground attacks can still hit and reveal.
  Hidden actors are removed from bot live-state inputs, not merely drawn hidden.
  Madness families only choose visible real enemies and stop refreshing a
  previously observed target's location while it is concealed.

This is an opt-in combat vision integration, not complete map-wide fog for every
existing patrol, hazard, corpse, subjective effect or interactable. In particular,
Spitter pools still use existing hazard presentation. No fog overlay art,
exploration map, final reed placement, Ritual source mutation policy, or
high-Madness photography exception is claimed by this change.

## Development and verification

`DMSpawnReeds [radius]` creates a known reed area 500 units ahead on world X.
`DMSpawnSwamp 2` places a Lurker in the same location. The existing lighthouse
objective can be completed through `DMSpawnObjective lighthouse` and normal
interactions. No actor is made invulnerable by concealment.

`DreadMeridian.Editor.Vision` covers concealment, network relevancy decisions,
target rejection, shared detection, commitment/damage reveal, source disabling,
and lighthouse completion/restoration. Full gameplay regression is also run.

`NetworkTest -SwampEnemies` places reeds around the Lurker and adds an out-of-sight
sentinel outside the gameplay roster. Both clients must validate the final
entitled state and must never receive that sentinel. The test-only expected-state
RPC also strips fields for hidden enemies rather than leaking the authority
roster through its verification payload.

The network probe continuously rejects the sentinel and requires a vision-filter
pass from both clients. Initial failures exposed a stationary concealed Lurker
outside commitment range and missing remote-controller vision publication; both
were fixed without changing the encounter timeout or granting extra sight.

Capture version 0.14.0 and combat rules swamp-vision-v1 distinguish this behavior.
Named RNG streams are unchanged. Validation is Win64/UE 5.8.2 only, with no
host-migration, deterministic replay or Play Trace ingestion acceptance claim.
