# Shell flow: main menu, lobby, mission, case report

The shell is a persistent level, `Content/DreadMeridian/Maps/L_Shell.umap`, that hosts the front
end and streams the mission in without a level transition. One world, one game mode, one player
controller across the whole session.

```powershell
.\tools\Unreal.ps1 -Action Shell
```

`-Action Play` still opens the combat sandbox map directly and behaves exactly as before.

## Screens

| Phase | Screen | What it does |
|---|---|---|
| MainMenu | Title, Start, Settings, Exit to desktop | Start opens the lobby. Settings is drawn disabled. Exit quits. |
| Lobby | Scenario, four seats, Leads, Launch Expedition | Launch begins streaming the mission sublevel. |
| Loading | Streaming notice | No input. |
| Mission | The ordinary combat HUD | `ADMShellHUD` defers to `ADMCombatHUD`. |
| CaseReport | Victory or Defeat, recorded metrics, consequences | Return to main menu reopens the shell level. |

`FDMShellState` owns the legal transitions and is a plain struct with no engine dependency, so the
flow is covered by `DreadMeridian.Foundation.ShellState` rather than only by play testing. Every
request is rejected in a phase that does not allow it: a duplicate streaming callback cannot spawn
a second roster, and a second completion cannot rewrite the recorded outcome.

## How the mission starts

`ADMCombatGameMode::StartPlay` used to spawn the roster and start the combat timer directly. That
body is now `BeginEncounter()`, and `ShouldBeginEncounterOnStartPlay()` decides when it runs. The
sandbox map answers true and is unchanged. `ADMShellGameMode` answers false, then calls
`BeginEncounter()` after `ULevelStreamingDynamic` reports the mission sublevel visible.

Because `ADMShellGameMode` derives from `ADMCombatGameMode`, the encounter, bots, pings, capture and
smoke behaviour are the same code that the sandbox map runs. If the sublevel fails to load the
encounter still starts: `BeginEncounter` spawns `ADMSandboxArena` when the map supplied none.

`CompleteCombat` calls the new `OnEncounterComplete(bVictory)` hook, which moves the shell to the
case report. Victory and defeat both arrive through it, so the end screen shows the real outcome.

## Returning from a finished run

Dismissing the case report reopens `L_Shell`. A finished run leaves spawned combatants, pickups and
ability markers in the persistent level, and the encounter state inside `ADMCombatGameMode` has no
in-place reset. Reopening the level is the honest teardown until a real one exists; replaying
without a level reopen is not implemented.

## Verification

```powershell
.\tools\Unreal.ps1 -Action ShellSmoke -Outcome Victory
.\tools\Unreal.ps1 -Action ShellSmoke -Outcome Defeat
```

Drives the front end headlessly with `-DMShellProbe`: main menu, Start, lobby, Launch Expedition,
streamed mission, combat, case report. It requires all of `DREAD_SHELL_PROBE_MENU`,
`DREAD_SHELL_MISSION_SHOWN`, `DREAD_SHELL_PROBE_COMPLETE` and `DREAD_COMBAT_SMOKE_PASSED`, so a
mission that never streamed in or a run that never reached the case report fails the action.

Adding `-RenderOffscreen` paces the probe and writes `shell-mainmenu`, `shell-lobby` and
`shell-casereport` to `Saved/Screenshots`. This exercises the flow, not balance or feel.

## Placeholders

The shell draws an amber `PLACEHOLDER` tag on anything with no system behind it. Nothing below is
implemented, and none of it is invented data on screen:

- **Settings.** No options of any kind. The menu row is disabled, not hidden.
- **Scenario selection, factions, mutators, the hidden Elder One.** The lobby names the swamp
  scenario from the GDD, but the mission that streams in is always the combat sandbox encounter.
- **Seats, hero preferences, readiness, invites, matchmaking, backfill.** Seats are locked. The
  encounter always fields the four investigators; the local player takes one and bots take the rest,
  exactly as `-Action Play` does.
- **Leads, the investigation archive and new evidence.** Nothing is recorded when a run ends.
- **Case report consequences.** Elder One faced, objectives, ritual stage, injuries and Crises,
  scenario consequences and Lead completions are all listed as missing. What the case report does
  show is real: the outcome, plus `FDMCombatMetrics` from the run that just finished.
- **Multiplayer shell.** The phase is replicated and the buttons are server RPCs, but only a local
  session has been exercised. Encounter metrics on the case report are server-side and are not
  replicated, so a client sees a placeholder there.
- **Controller navigation.** The screens are pointer-driven. Gamepad focus and navigation are not
  wired, so the GDD's controller parity does not hold in the shell.

Screen layouts follow `design/ui-mockups`. The canvas screens are a functional recreation, not a
pixel match: they use the engine's default font rather than the mockups' type, and the mockups'
sample party, scenario and case-report content is deliberately not reproduced as if it were real.
