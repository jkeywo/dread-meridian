# AI tuning harness

`tools/tune_ai.py` tunes the utility-AI weights with headless soak runs. It is
sandbox tuning against the smuggler encounter, not approved balance.

## How a run works

Each evaluation launches `UnrealEditor-Cmd` on `L_CombatSandbox` with
`-server -nullrhi -DMSmugglerSoak -deterministic -fps=60 -DMSeed=<seed>
-DMAIWeights=<candidate.json>`. The fixed time step makes the world advance
exactly 1/60 s per frame with no frame pacing, so a full encounter simulates in
about 15 s of wall clock while movement integrates as it does in real play. The
same candidate and seed reproduce the same `DREAD_AI_RESULT` line; seeds differ
only through the spawn jitter the game mode draws from the RunGeneration stream.

`DREAD_AI_RESULT` is one JSON log line written by `ADMCombatGameMode::LogResult`
at victory, defeat or the 1800-tick soak timeout: outcome, tick, standing counts,
damage per team, downs, kills, revives, signature and Q activations, ping count
and damage per entity.

It also carries tactical-quality counters, sampled by `StepMetrics` after each
Think loop. Outcome counters say who won; these say whether the squad played
well, which damage and downs alone cannot distinguish from luck.
`overkill_damage` is damage past what was needed to kill. `hazard_ticks` counts
companion-ticks standing inside a hostile circle. `peel_ticks` counts attacking
an enemy that is beating a companion in peril, and `unanswered_peril_ticks` the
case nobody answered. `trap_ground_kills` counts enemies killed inside an
investigator's armed trap, wire or zone. `isolated_downs` counts downs with no
living companion nearby. `focus_distinct_ticks` and `focus_holder_ticks` are a
pair: their ratio is how many companions share each target.

Read the pair as a ratio, not a total, and be careful what you conclude from it.
At the 2026-09-12 baseline it sits near 3.2 companions per ~1.05 distinct
targets, so companions already converge on one enemy simply by being near it —
a plain "two bots share a target" counter is over 90% saturated and cannot
measure an improvement. `design/experiments/ai-tactics-2026-09-12.md` records
the baseline and which counters actually discriminate.

## Search

Phases alternate: even phases mutate only the enemy profiles (Gunman, Bruiser,
Lookout, Bomber, GangBoss), odd phases only the investigator bots (Sapper,
Photographer, Medium, Smuggler). Each phase is a (1+7) hill climb: every
generation mutates the incumbent on two to four knobs, evaluates every
candidate on every seed, keeps the best only if it beats the incumbent, and
stops after two stale generations. Enemy fitness rewards damage to
investigators, downs and a defeat and penalises damage taken and long fights;
bot fitness rewards damage to enemies, kills, a victory and a fast finish and
penalises damage and downs taken.

Companion knobs also cover where to stand and which target to pick: the positional
weights (`Position*`), the `Reposition` action's weight and latches, and the four
focus-term weights. Focus terms live in an array, which a dotted knob path cannot
address, so the search tunes `FocusTerm.<index>.Weight` and `expand_focus_terms`
rewrites those into the array the profile loader reads. The inputs and curve shapes
are fixed by design, so `FOCUS_TERMS` in the harness must stay in step with the
terms `DMUtilityAI::DefaultWeights` builds, or a candidate quietly tunes a
different curve from the one that ships. Enemies have neither system yet.

Knobs are chosen for effect, not coverage. With rank ordering and one option per
channel, an action's weight only matters when it competes inside the same rank
and channel: an enemy's Engage, Signature or Strafe weight and a companion's
BasicAttack weight never change a decision, and an ability's `Base` only matters
at the cast-or-hold boundary. The knob table therefore holds latch thresholds
(keep-distance, flee), runtime and decision-cooldown caps, ranges (leash, sight,
ally radius, strafe geometry), worth multipliers that gate casts, conservation
constants and, for companions, ping compliance.

## Outputs and reuse

`--out` holds `runs.csv` (one row per run), `candidates/`, `logs/`, `best.json`
(the winning overrides, rewritten after every generation), `state.json` and
`report.md`. `--resume` continues from `best.json`. `--dry-run` prints the
commands. Apply a result without rebuilding through `-DMAIWeights=best.json`;
baking copies the values into `DMUtilityAI::DefaultWeights`, after which
`tools/Unreal.ps1 -Action GenerateAIProfiles` writes them into the
`/Game/DreadMeridian/AI/AIP_*` data assets.

Hold-out checks use `--evaluate <json|none> --seeds ...` on seeds no campaign
touched; `--validation-seeds`, `--min-gain` and `--sigma` harden a campaign
against seed-fitting. The first campaigns and their hold-out tables are recorded
in `design/experiments/ai-tuning-2026-09-11.md`.

Timeouts: the in-game soak timeout still reports real metrics as
`outcome=timeout`; a run killed at the 900 s process timeout scores zero and
disqualifies its candidate. Read result files only after a run finishes.
