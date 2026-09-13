# Objectives, Shub and relic implementation

Scope authorized September 13 2026: objective foundation and authored swamp set;
Elder One steps 1-10 excluding Ritual integration (step 4); relic foundation and
all eight current mechanics. HTN planning and Nyarlathotep encounter content are
outside this change. GDD statuses and balance remain unchanged.

## Step ledger

- Objective core: native authority actor, public state, interaction verbs,
  deadlines, explicit repair preserving completed steps, reward idempotence,
  snapshot validation, combat tick integration and server interaction requests.
  Development console: `DMObjectiveInteract [symbol]`, `DMObjectiveRelease`.
  Relic reward delivery, real vision entitlement and basin navigation require
  their respective integrations; the core exposes accepted world-state results.

All fixture durations, distances, XP and treatment charges are provisional.
These subsystem snapshots are not a complete run or host-migration save.

- Authored catalogue: all 16 variants run through real interactions in the
  ObjectiveCatalogue PIE test; static idols register in the combat roster and
  surviving late-form idols protect nearby enemies. Bell sequences have an
  observation phase with truthful visual symbols. `DMSpawnObjective <id> [0..4]`
  creates a developer fixture; I / controller Menu interacts, 1/2/3 enter bells.
  No HTN selection or automatic Ritual subscription. Escort presentation and
  layouts are greybox fixtures, not the finished swamp scenario or navigation
  acceptance. Lighthouse/drain completion projections await scenario consumers.
  Verified: Foundation Test, ObjectiveCatalogue EditorTest (16 variants), Smoke.

- Elder 1: server-only seeded identity and encounter lifecycle, guarded phase and
  terminal transitions, versioned authority snapshot, RNG restoration coverage.
  `-DMElderOne=Shub` or `Nyarlathotep` forces developer selection while consuming
  the same named selection draw. Capture records boss_selection_version=1, never
  a player-visible secret projection. Foundation Test and Smoke passed.
