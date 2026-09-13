# Injury verification protocol

Purpose: verify GDD Injury themes in the production C++ damage, cast, attack,
movement, revive and recovery APIs. Values are provisional, not balance evidence.

1. Run `tools/Unreal.ps1 -Action Test`. `Foundation.Injuries.Rules` covers rolling
   expiry, consumed windows, lethal coalescing, distinct slots and overflow,
   treatment order, effect boundaries, named-stream independence and restoration.
2. Run `tools/Unreal.ps1 -Action EditorTest`. `Editor.Injuries.Control` acquires
   each Injury through seeded real damage, checks gameplay effects and rejected
   casts, then checks repeated down/revive, finite treatment and timed food.
3. Run `-Action Smoke`, `-Action CombatSmoke -Outcome Revive`,
   `-Action SmugglerSmoke`, and `-Action NetworkTest`.
4. Validate a new log using `tools/validate_capture.py <log> --require-provenance`.
   Validate combat telemetry through the local Play Trace adapter. Keep generated
   reports and captures in ignored `Saved/`.

Network coverage compares final replicated Injury projections. It does not prove
every transient timer on a remote client. This protocol does not establish device
parity, balance, medical objective integration, gameplay replay or migration.

## Verification record — 2026-09-13

UE 5.8.2 Win64 build passed. Foundation: 28 passed; editor: 13 passed, including
the Injury control test. Export contract: 14 passed. Play Trace Unreal adapter:
40 passed. Required foundation smoke passed with complete provenance. Revive
smoke and Smuggler simulation ended in victory; both combat captures validated.
The network probe passed disconnect takeover and both client projection checks.

Local reports (ignored, reproducible using the protocol above):

- Foundation: `Saved/Automation/56c4357d50d94124978812009e617bd9`
- Editor: `Saved/Automation/ba666d964d5f49aea00b65966d0a3cfc`
- Network: `Saved/NetworkTests/826560e201bc40cb8dde41c3d599ec8f`
- Foundation capture: `Saved/Playtrace/2fddd6a3-4471-458b-b1af-b8a91c584224`
- Revive capture: `Saved/Playtrace/74dc4800-4c51-10d5-5ce5-c8a5b41be2d5`
- Smuggler capture: `Saved/Playtrace/978f04e4-4793-0dc3-8042-1c900286f0c9`

Live editor assertions cover food/treatment and all six effects. The captured
smoke scenarios observed Injury gains but did not exercise food or treatment;
healing transition rejection is separately covered by adapter fixtures. Visual
HUD layout and physical controller parity were not manually verified. Non-Win64
platform SDK diagnostics do not constitute builds or validation of those targets.
