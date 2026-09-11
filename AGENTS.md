# Dread Meridian working rules

This repository owns the game and its game-specific Play Trace design/evidence.
`C:/Coding/playtrace` owns the reusable application; do not copy game data into it.
The parent fleet conventions do not enroll this project in Vellum or PASM.

- Read the relevant sections of `gdd/Mythos_PvE_MOBA_Master_GDD_v0.3.md` before
  implementing gameplay. Preserve LOCKED/PROVISIONAL/OPEN/CONTENT TBD status.
  Do not silently edit the GDD or turn harness values into approved balance.
- Pin Unreal to 5.8.2. Use `tools/Unreal.ps1`; do not commit generated IDE/build files.
- Put authoritative rules in C++. Replicate public projections; keep hidden boss,
  RNG and subjective perception state off shared GameState.
- All designed randomness uses named streams. Append serialized stream IDs;
  version intentional reproducibility changes and test restoration.
- Capture observes accepted state changes; it cannot decide gameplay outcomes.
  Developer evidence is not player-visible replication or a host migration save.
- Do not claim four-bot simulation, deterministic gameplay replay, host migration,
  GAS combat, controller parity or Play Trace ingestion until each is implemented
  and verified. Enabled plugins are dependencies, not implemented features.
- Run `tools/Unreal.ps1 -Action Test` and `-Action Smoke` for runtime changes;
  `python -m unittest discover -s tests -v` for export contract changes.
  Validate a new captured log with `--require-provenance` after changing export.
- Keep irreplaceable evidence and experiment protocols under `design/`; keep
  transient captures/reports in ignored `Saved/` or `.playtrace/`.
- Report engine build/test limitations explicitly. Do not weaken tests to make
  unsupported semantics appear to work.
