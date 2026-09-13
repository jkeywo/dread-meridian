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
  Relic rewards now create shared loot rolls. Real vision entitlement and basin
  navigation require scenario consumers; the core exposes completion results.

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

- Elder 2: normal encounter rosters now get exactly one actual-boss resonance.
  Version 2 changes Madness assignment intentionally; isolated smoke/network
  fixtures retain their explicit family coverage. Private resonance cues reuse
  owner-only Madness delivery, separate from the family symptom scheduler.
  Verified both bosses across 256 seed assignments, Foundation Test and Smoke.

- Elder 3 and 5-10: shared arena contract, threat API, permanent corruption,
  private genuine-growth membership/vulnerability, corpse markers, interruptible
  Broodling feeding/splitting, Black Goat charge, and Shub phases/attacks.
  Each step passed Foundation Test and Smoke; PIE covers growth privacy,
  Broodling counterplay, Goat charge/Break, and Shub attacks/phase/victory.
  Start a developer encounter with `-DMElderOne=Shub`, then `DMStartShub` during
  active combat. Existing arena art is provisional. No automatic Ritual hookup,
  complete swamp navigation, full-run balance or host-migration claim.
  Subsystem snapshots restore into an existing actor roster; they are not a
  replacement for a future full-run snapshot and actor reconstruction service.

- Relic infrastructure: native run inventory with a provisional replicated cap
  of two; named-stream shared Need/Greed/Pass rolls; authority validation and
  duplicate-award rejection; bot valuation; objective loot delivery; accepted
  combat, control, Break, healing, tier-entry and interaction hooks; public
  inventory/roll UI; private runtime snapshots for effect budgets and timers.
  Keyboard N/M/P votes directly; I or controller Menu opens the pending roll,
  Tab/right shoulder cycles, Space/south confirms, Escape/east closes.
  Controller bindings are compiled, not a manual parity acceptance result.

- Each of the eight relics has its own implementation commit and focused PIE
  test: Swagger threat/ally Break, Medal contribution/primed ability, Knot
  secondary control, overheal Shield accounting/decay, Morphine injury conversion
  and revive factors, Rosary across all four investigator resources, Coin
  movement wakes, and Gloves objective protection/nearby defense. Explicit
  interrupts still cancel Gloves interactions; its defense reduces both melee
  and ranged damage. All magnitudes and durations remain provisional.

- Integration review: Shub consumes the shared threat table and explicit target
  overrides; encounter spawns emit accepted combat spawn records; loot capacity
  replicates; the loopback network probe exercises a shared relic roll through
  real client votes and compares final winner, inventory and capacity. Combat
  capture version is 0.12.0, rules version objectives-shub-relics-v1. Named stream
  IDs retain their existing serialized order. This is not Play Trace ingestion
  acceptance or deterministic gameplay replay.
