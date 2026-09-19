# Test arena

The developer arena lets one local player test a selected investigator, optionally
with any of the other three investigators as autonomous AI companions. Smaller
parties are a testing exception; ordinary missions still field four investigators.
All numerical tuning is provisional. No GDD status or balance decision changes.

```powershell
.\tools\Unreal.ps1 -Action Arena -Investigator Smuggler -Seed 1927
```

The launcher builds with Unreal 5.8.2 and creates the arena map if needed. In the
editor, open `/Game/DreadMeridian/Maps/L_TestArena` and use **Standalone**, one-player
PIE. The existing **Play as** preference supplies the initial investigator. The
arena always gives that investigator to the local player, even if the editor's
bot-observation preference is enabled. Network sessions are rejected.

## Setting up a fight

The arena opens paused, with one investigator and no enemies. The overhead setup
camera frames the floor and the right-hand panel. Cover stands at the edges; the
marked reed circle supports the existing concealment and ambush rules.

- Select the playable investigator and toggle companion checkboxes. A party
  change reloads the arena, preserving the configured enemies and seed.
- Click an enemy name for a single-enemy selection, or use the count controls to
  build a custom group. Cycle the preset picker and click its name to load its
  composition into the count rows.
- Click **Place selected types / counts**, then click the floor. The preview is
  green when the entire formation fits and red when placement is invalid.
  Right-click cancels placement.
- Place as many groups as needed, up to 32 enemies in the saved setup. The whole
  formation must clear walls, cover, combatants, previous reset positions, and
  the reserved investigator starting area. Rejection leaves the roster unchanged.
- **Undo Last Placement** removes one complete formation; **Clear Enemies** removes
  all placements. Both are available only before the first fight.
- **Fight** returns to the normal character camera and combat controls. **Pause**
  freezes world simulation, movement, cooldowns, and hazards while leaving the
  arena controls usable. Place reinforcements while paused, then **Resume**.
- Eliminating every enemy or losing the party freezes the result inside the arena.
- **Reset Test** briefly reloads the map. It restores every authored placement,
  including reinforcements added while paused, with fresh health, resources,
  cooldowns, progression, and AI state. No combat snapshot is restored.

The ten types are Gunman, Bruiser, Lookout, Bomber, Gang Boss, Crawler, Lurker,
Spitter, Grasper, and Old Thing. Presets reproduce the existing three smuggler
camps, patrol pair, boss posse, and a Crawler/Lurker/Spitter/Grasper group. A preset
does not start mission waves. Smugglers use their existing role defaults; Swamp
Things use the existing swamp-probe health/damage tuning (90/8, or 350/14 for Old
Thing). Their normal role signatures, visibility rules, and AI remain active.

The setup is retained only within this arena session. There are no saved preset
files, multiplayer controls, main-menu entry, boss encounters, balance sliders,
or controller-navigation support in this version.

## Verification and evidence

```powershell
.\tools\Unreal.ps1 -Action Test
.\tools\Unreal.ps1 -Action Smoke
.\tools\Unreal.ps1 -Action ArenaSmoke
.\tools\Unreal.ps1 -Action ArenaSmoke -RenderOffscreen
.\tools\Unreal.ps1 -Action EditorTest -Filter DreadMeridian.Editor.TestArena
python -m unittest discover -s tests -v
python tools/validate_capture.py Saved/Playtrace/<run-id>/events.jsonl --require-provenance
```

`ArenaSmoke` uses real actors and reloads to check solo/four-person assignment,
all ten types, atomic placement rejection, removal controls, pause-time spawning,
victory, defeat, and fresh state after three resets. Its lethal probe hits are
explicit test inputs, not evidence of balance. The PIE test exercises the paused
panel and verifies movement, health, shield, attack timers, and a pending Bomber
signature remain frozen. The rendered probe saves screenshots under
`Saved/Screenshots/arena-stage-*.png`.

Captures use `scenario_id: test-arena-v1` and `run_kind: test_arena`, with actual
party metadata and accepted placement/phase changes. Each reset closes the prior
attempt and begins a new capture. An unfinished attempt is `aborted`; victory and
defeat retain their actual outcomes. The game-owned validator checks the envelope,
provenance, configuration, and arena lifecycle; it does not replay combat, certify
balance, or establish Play Trace ingestion or deterministic gameplay replay.

For an editor launch without launcher provenance, captures can legitimately say
`unrecorded`; collect provenance-required evidence through `Unreal.ps1`.
