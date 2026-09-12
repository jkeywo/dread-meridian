# Dread Meridian

A PC-first Unreal Engine co-op Mythos PvE game and Play Trace's video-game pilot.
The [master GDD](gdd/Mythos_PvE_MOBA_Master_GDD_v0.3.md) remains canonical.

## Play the combat sandbox

The playable slice has a native greybox arena, four investigators, a staged
smuggler encounter, GAS attacks, Health/Shield, downing, interruptible revives, basic
threat and bot control. Humans take over existing investigators; disconnects
return them to the same companion policy used in headless runs.

```powershell
.\tools\Unreal.ps1 -Action Play
```

Requires Unreal **5.8.2**, Visual Studio 2022 with Game development with C++, MSVC
and a Windows SDK. Tested toolchain: MSVC 14.44 / SDK 10.0.22621.0.
Set `UE_ROOT` or pass `-EngineRoot` if needed. Each launch/test builds first.
Save and close the editor before rebuilding so it releases the game DLL.
Use `-Action Editor` to edit the map and press Play inside Unreal.

| Action | Mouse/keyboard | Controller |
|---|---|---|
| Move | Right-click ground | Left stick |
| Select enemy | Right-click enemy or Tab | Right shoulder |
| Auto-attack/chase | Left/right-click enemy or Space | Bottom face button (A) |
| Aim an ability | Q / W / E / R | Left shoulder, triggers, D-pad down |
| Confirm / cancel an aimed ability | Left-click / right-click or Escape | A / B |
| Detonate placed satchels | F | Top face button (Y) |
| Revive nearby downed ally | V | Left face button (X) |
| Ping (tap for context, hold for the radial) | G | D-pad up |

Q/W/E/R take the conventional MOBA keys (GDD 3.2), so keyboard movement is
right-click only and revive moved to V.

Move to cancel auto-attack. Revival requires staying close and is interrupted by
damage; repeated Grievous Injuries lengthen the channel. Defeat occurs when all
four investigators are down. Clear the eleven occupation enemies, then defeat
the Gang Boss and all four posse members to win this **sandbox encounter**,
not a GDD scenario/boss. Smoke and network probes use a compact three-enemy
fixture. Restart play to reset the encounter.

Companions and enemies share one utility brain: every candidate action is ranked
and scored, each reserves only the move/attack/cast channels it needs, casts must
beat a scarcity-scaled threshold, and latches stop dithering. Companions rescue
downed allies, retreat toward the leader at low Health, step out of bomber
circles and answer [pings](docs/pings.md). Tuning lives in per-hero/role data
assets with C++ defaults. It is a per-bot combat policy, not the GDD team planner.
Ability evolutions, burst-window and
named Injury effects, Madness, HTN/objectives, bosses, matchmaking and host
migration remain explicit omissions in captures. Break/CC uses configurable Resolve and bounded control windows; see [Break/CC](docs/break-cc.md).
Madness remains a declared stub meter. Sandbox and smoke-profile values are provisional tuning.

## Verify

```powershell
.\tools\Unreal.ps1 -Action Test
.\tools\Unreal.ps1 -Action Smoke
.\tools\Unreal.ps1 -Action CombatSmoke -Outcome Victory
.\tools\Unreal.ps1 -Action CombatSmoke -Outcome Defeat
.\tools\Unreal.ps1 -Action CombatSmoke -Outcome Revive
.\tools\Unreal.ps1 -Action NetworkTest
.\tools\Unreal.ps1 -Action ShellSmoke -Outcome Victory
python -m unittest discover -s tests -v
```

`tools/tune_ai.py` tunes the bot and enemy AI weights with deterministic headless
soak runs; see [docs/ai-tuning.md](docs/ai-tuning.md). Its results are sandbox
tuning, not approved balance.

Combat smoke profiles use real actors, abilities and bot decisions with declared
test tuning. The network test starts hidden loopback processes, disconnects one
client, then checks two clients against final authoritative health/shield/state.
It requires a recorded handoff back to bot control. Reports: `Saved/NetworkTests`.
This does not verify matchmaking, host migration or physical controller feel.

## Front end

```powershell
.\tools\Unreal.ps1 -Action Shell
```

Opens the shell level: main menu, expedition lobby, then the mission streamed in on **Launch
Expedition**, then a case report showing the run's real Victory or Defeat. It is the same
encounter, bots and capture that `-Action Play` runs, started from the lobby instead of on load.
Settings, scenario and seat selection, Leads and the case report's consequence list are drawn as
explicit placeholders. See [shell flow](docs/shell-flow.md) for the full list and
`-Action ShellSmoke` for the headless check.

The native map is `Content/DreadMeridian/Maps/L_CombatSandbox.umap`.
`-Action GenerateMap` creates it if absent and preserves existing authored edits.
Install Git LFS before committing native assets. Editor Python authors the map;
the runtime game has no Python or Play Trace service dependency. Packaging the
dedicated Server target requires a source engine build.

## Import evidence into Play Trace

`playtrace.toml` declares a v2 local Unreal manifest, with no Google dependency.
The launcher records source/GDD digests, Git revision, dirty status, engine, seed,
profile and omissions. Captures are local server/developer artifacts under
`Saved/Playtrace/<run-id>/events.jsonl`.

With the sibling Play Trace environment installed:

```powershell
.\tools\ImportEvidence.ps1 -Capture Saved/Playtrace/<run-id>/events.jsonl
```

This creates a local snapshot and records hypothesis-linked `telemetry_run`
evidence in this game's `.playtrace` database. Its relation is context; it does
not approve the hypothesis or change design sources. Changed sources/engine or
missing provenance make evidence stale. Retain exact source/patch and selected
irreplaceable logs in `design/evidence`; a dirty digest cannot reconstruct bytes.

See [integration](docs/playtrace-integration.md), [architecture](docs/architecture.md)
and [remaining slices](docs/roadmap.md). Portable CI checks the original capture
contract; engine/network checks require the pinned Unreal installation.

## Investigator basics

The four named investigators use their supplied rigged models, held weapons, holstered gear, retargeted combat animations and attack/ability effects. See [combat presentation and asset sources](docs/combat-presentation.md). See [character rules and resource availability](docs/characters.md). Basic, Q and named W/E/R are implemented for all four at their A nodes; ability evolutions remain outstanding. See [named kits](docs/kits.md).
Non-original content is recorded in [third-party content](docs/third-party-content.md).

## Base Q abilities

Every investigator has Basic, Q and its named W/E/R. See [abilities](docs/abilities.md) for the shared targeting model and Q, and [named kits](docs/kits.md) for W/E/R. Select the investigator with the editor Play-as picker, then use Q/W/E/R (LB/RT/LT/D-pad down) to aim.
