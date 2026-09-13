# Swamp Thing mechanics

Scope: the five Swamp Thing enemies only, without new art. GDD section
"Swamp MVP factions" remains the design source; numbers below are provisional.
No Local Cult roles, full faction director, map, fog or navigation work is added.

## Implemented roles

| Role | Native behavior |
| --- | --- |
| Crawler | 520 movement speed, melee range, faster attacks, 30% basic damage bonus against isolated targets. Grouping within 400 removes the bonus. |
| Lurker | Waits for an ambush opportunity within 500; readable 12-tick commitment, modest damage and 40% slow. Moving out of the marked area evades it. Remains targetable; actual reed concealment awaits scenario vision. |
| Spitter | Ranged attacks and a telegraphed pool lasting 80 ticks, at most one active pool per caster. Persistent-hazard damage and slow, bounded radius and cadence, geometry checks, bot hazard avoidance through existing markers. Pools end when the caster dies or combat ends. |
| Grasper | Telegraphed targeted pull with range and sight checks. Evading the aim point, explicit interrupt, stun/hold or significant caster displacement counters the cast. Existing displacement resistance reduces the pull. |
| Old Thing | Protected elite Resolve, territorial targeting and radial displacement. Isolated victims are pushed farther. Break cancels the windup. |

The native role state machine owns targeting and movement, prefers isolation,
consumes threat and explicit overrides, and returns to its home when no valid
target remains. Targets outside the territory cannot drag it across the map.
Existing basic attacks, damage, CC, resistance and Break remain authoritative.
Public role/cast/pool state replicates; versioned subsystem snapshots restore
into existing actors and rebuild tells. No new RNG draws are required.

Swamp/native-human hostility is symmetric and applies to basic attacks, damage,
control, Smuggler signatures and AI target selection. Swamp Things cannot damage
each other. This uses the existing human-enemy classification; it is not a
general diplomatic faction registry or a Ritual relationship-override system.

## Running and checking

- `DMSpawnSwamp 1` through `5` creates Crawler, Lurker, Spitter, Grasper or
  Old Thing near the authority player during active sandbox combat.
- `tools/Unreal.ps1 -Action SwampSmoke` runs four native companion bots against
  one of each role in the existing arena, capturing an outcome or failing on
  timeout. This is an enemy encounter fixture, not the full swamp scenario.
- `tools/Unreal.ps1 -Action NetworkTest -SwampEnemies` checks all five role and
  faction projections on two real clients alongside inventory, owner privacy,
  final combat state and disconnect takeover.
- `DreadMeridian.Editor.Swamp` tests isolation/grouping, mutual faction damage,
  friendly-fire rejection, evasion, modest slowing opener, persistent pool
  cadence/expiry/restoration, displacement resistance and interrupt/Break
  counterplay. Full Foundation/Editor regression and Smoke remain required.

Capture observes `swamp.action` and accepted combat events. Combat rules version
is swamp-things-v1; capture version is 0.13.0. This does not establish Play Trace
ingestion, deterministic gameplay replay, host migration, manual controller
parity or approved balance. Existing meshes and markers are reused unchanged.
