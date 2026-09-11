# Foundation capture smoke protocol

Status: engineering experiment, not a gameplay experiment.

1. Record/preserve the source checkout and any uncommitted patch.
2. Run `tools/Unreal.ps1 -Action Test`.
3. Run `tools/Unreal.ps1 -Action Smoke -Seed 1927`.
4. Read the capture path from the smoke log and run
   `python tools/validate_capture.py <path> --require-provenance`.

Expected path: expedition start; reject negative ritual advance and early victory;
advance one stage; summon Apocalypse; finish victory; reject subsequent changes.
Expect exactly six emitted events: started, initial state, ritual advance,
summoning, terminal state, ended. Rejected actions emit no accepted-event records.

Pass requires engine tests, runtime marker, and evidence validation, including
complete provenance. Run UUID and wall-clock timing may differ across repeats.
Do not compare raw file hashes as a determinism test. Gameplay data/state should
match for the same seed and source, within this small harness's scope.

Keep selected observations under `design/evidence` with source/patch, seed,
engine version and the protocol version. Do not promote this run as evidence
that production bots, objectives, combat, host migration or balance are correct.
