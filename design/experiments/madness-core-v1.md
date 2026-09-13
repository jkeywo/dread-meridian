# Madness core verification

Purpose: check the GDD current/floor/Crisis rules and private information boundary
without counting unimplemented family content as complete.

1. Run `tools/Unreal.ps1 -Action Test`. `Foundation.Madness.Rules` verifies exact
   thresholds, finite input rejection, irreversible floor, recovery bounds,
   Crisis timing/respite (including floor 100), and rules-state restoration.
2. Run `tools/Unreal.ps1 -Action EditorTest`. `Editor.Madness.Core` uses a real R,
   contextual private cue API, grounding and live Crisis clock. It checks that
   shared summaries/other HUD rows omit Madness and Crisis permits attacks.
3. Run `tools/Unreal.ps1 -Action NetworkTest`. Both clients must validate their
   private snapshot and confirm no combatant contains replicated Madness state.
   The existing probe also checks disconnect takeover and public projections.
4. Run `tools/Unreal.ps1 -Action Smoke`, the Python export contract tests, and
   validate the new foundation capture with `--require-provenance`. Validate the
   network combat capture through the local Play Trace telemetry adapter.

Keep generated logs/reports in ignored `Saved/`. This is correctness evidence,
not balance, full family behavior, controller parity, or migration evidence.

## Verification record — 2026-09-13

UE 5.8.2 Win64 build passed. Final foundation suite: 29 passed. Editor suite:
14 passed, including real R pressure, quiet/interrupted grounding, private HUD
filtering, contextual cue expiration, and live Crisis recovery while attacks remain
available. Python export tests: 14 passed. Foundation smoke passed and its capture
validated with complete provenance. Network probe passed both private snapshots,
absence of combatant replication, disconnect takeover and public state checks.

The network fixture creates distinct, explicitly test-only cues through `Manifest`;
it does not stand in for authored family symptoms. Its validated capture contains
six Madness changes, six threshold events and four private symptom observations.
Crisis timing was exercised in PIE and pure rules, not in this network capture.

Local reproducible reports and captures (ignored):

- Foundation: `Saved/Automation/e97eef470a2a4006b191a527109a99e9`
- Editor: `Saved/Automation/5ae117fee57b4050935c0c4505b93994`
- Network: `Saved/NetworkTests/42addbf893454cc990af8e6875096293`
- Foundation capture: `Saved/Playtrace/cca81b94-4045-e128-6454-518054548ba8`
- Network capture: `Saved/Playtrace/cc26ded7-492c-e2b2-d579-369d0cea6703`

Manual HUD layout and physical controller parity were not verified. Other platform
SDK diagnostics do not constitute builds/tests for those platforms. No engine
limitation prevented the required Win64 checks.
