# Play Trace integration

The shared application implements `snapshot --local`, `validate-telemetry` and
`record-experiment --telemetry`. The game owns its manifest, GDD, native assets,
hypotheses and actual captures; Play Trace owns generic readers and synthetic tests.

From the Play Trace repository:

```powershell
uv run playtrace snapshot C:\Coding\dread-meridian --local `
  --db C:\Coding\dread-meridian\.playtrace\playtrace.db `
  --output C:\Coding\dread-meridian\.playtrace\snapshot.json
uv run playtrace validate-telemetry path\to\events.jsonl
uv run playtrace record-experiment C:\Coding\dread-meridian `
  --snapshot C:\Coding\dread-meridian\.playtrace\snapshot.json `
  --hypothesis C:\Coding\dread-meridian\design\experiments\combat-capture-hypothesis.json `
  --telemetry path\to\events.jsonl --relation context `
  --rationale "Combat infrastructure observation; omitted GDD systems remain untested."
```

`tools/ImportEvidence.ps1` wraps this with the sibling installed Python environment.

The v2 local manifest supports repository-relative Markdown, text, Unreal
descriptors and binary inventories. Missing required sources, escaping paths and
changing inputs are rejected. Markdown fragments have stable content-based IDs,
exact digests/line anchors and explicit LOCKED/PROVISIONAL/OPEN/CONTENT TBD status.
Unmarked/mixed text remains unspecified. Code/Blueprint semantics are not inferred.

JSONL schema 1 uses a run UUID, contiguous sequence, event type, monotonic elapsed
seconds, server authority, developer visibility and data. Start/end occur once;
crashed/truncated captures fail. Orderly interruption is `aborted`, not defeat.

Combat headers include source/GDD/Git/engine identity, seed, profile, bot policy,
logical step duration and omissions. Spawn events record stable IDs, teams,
Health/Shield and damage/cooldown tuning. Damage records actor/target/ability and
before/after Health/Shield. Down, kill, revive and control changes use the same IDs.
Accepted shield gains emit `combat.shield_gained` with before/after Shield and
the actual amount granted. This keeps subsequent damage consistent with prior
recorded state; a gain at the shield cap emits nothing.
Accepted healing emits `combat.healed` with before/after Health and the actual
amount restored. Shared validation rejects healing Downed actors, over-healing
and inconsistent transitions. `injury.gained`, `injury.treated` and `recovery.used`
are developer observations outside the strict `combat.*` transition namespace.
The Injury stream is appended at ID 10, making RNG snapshot schema 2; historical
schema 1 capture provenance is still accepted. See [Injuries](injuries.md).
The [Madness core](madness-core.md) adds developer-only `madness.changed`,
`madness.threshold`, Crisis entry/exit and `madness.symptom` observations. Shared
ingestion accepts that namespace without claiming to replay family mechanics.
Private gameplay delivery uses the owning PlayerController, separately from capture.

Shared validation checks lifecycle, finite timing, sequences, actor references,
health/shield arithmetic and combat outcomes. Unsupported `combat.*` events fail.
The original game-owned validator still handles `foundation_harness`; combat
uses Play Trace's shared validator. `ai.decision` (a bot's focus or chosen
actions changed, with per-consideration scores) and `ping.created`,
`ping.responded`, `ping.ended` are developer events outside the `combat.*`
namespace; the shared validator accepts them while ticks do not decrease. They
are evidence of decisions, not player-visible state.

Import creates typed `telemetry_run` evidence and a hypothesis-linked claim with
an explicit relationship/rationale. Raw/config digests and omissions remain
visible. Captured repository/design/engine dependencies are checked against the
current snapshot; changes or unrecorded provenance are stale. Dirty sources can
be current when bytes match, but retain their exact source/patch separately.

Neither import nor validation approves hypotheses, edits sources or certifies
balance. Full scenario/boss semantics, native-asset patching and deterministic
replay remain future work.

Family versions add `madness.family` observations for assignments, fixations, urges,
echoes and Perception interactions. Cue IDs and subjective coordinates travel to
owners separately; the observations remain developer evidence. Final family rules
are `madness-families-v1` (capture `0.10.0`), with unchanged RNG stream schema 2.
