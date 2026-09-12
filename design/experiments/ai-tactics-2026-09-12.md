# Companion tactics baseline — 2026-09-12

Baseline for the deliberate-movement / coordination / target-selection work. Captured on the metrics
commit, which adds counters only and changes no decision, so these numbers describe the AI as it has
behaved since the named-kits work.

All figures are sandbox tuning evidence against the smuggler encounter. **Nothing here is approved
balance**, and the counters are observations: they never feed back into play.

## Protocol

Six seeds, one run each, ~4 s of wall clock per run:

```
UnrealEditor-Cmd DreadMeridian.uproject /Game/DreadMeridian/Maps/L_CombatSandbox \
  -server -unattended -nop4 -nosplash -nullrhi -nosound -deterministic -fps=60 \
  -DMSmugglerSoak -DMSeed=<seed> -abslog=<log>
```

Seeds 1-6 are the campaign seeds; hold-out seeds stay unused until the tuning campaign.

## Baseline

| seed | outcome | tick | dmg taken | downs | kills | overkill | distinct | holders | hazard | peel | unanswered | trap kills | isolated |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | victory | 660 | 497 | 2 | 16 | 256 | 673 | 2191 | 40 | 152 | 213 | 6 | 0 |
| 2 | victory | 656 | 489 | 2 | 16 | 239 | 743 | 2287 | 76 | 278 | 288 | 5 | 0 |
| 3 | victory | 581 | 391 | 0 | 16 | 207 | 709 | 2184 | 67 | 182 | 126 | 5 | 0 |
| 4 | defeat  | 734 | 818 | 4 | 12 | 185 | 800 | 2316 | 74 | 687 | 712 | 4 | 1 |
| 5 | victory | 710 | 593 | 2 | 16 | 202 | 727 | 2438 | 56 | 392 | 396 | 7 | 0 |
| 6 | victory | 700 | 543 | 1 | 16 | 204 | 760 | 2560 | 72 | 155 | 172 | 4 | 0 |

Five victories, one defeat. Seed 4 is the useful outlier and should stay in every comparison.

## What the baseline already settles

**Focus concentration is not the problem.** `focus_holder_ticks / focus_distinct_ticks` is about 3.2:
roughly three companions hold a target each tick and they are nearly always the *same* target
(distinct averages ~1.05 per tick). Companions already focus-fire — not by deciding to, but because
they all walk to the leader and then pick the nearest enemy, and it is the same enemy for everyone.

Two consequences for the plan:

1. The `AlliesOnTarget` consideration must be judged on *which* enemy the squad picks, not on whether
   it converges. The risk it carries is the opposite of the one anticipated: reinforcing a convergence
   that is already total. A saturating curve and the anti-clump term matter more than expected, and
   "two bots converge" is too weak a test to prove anything.
2. `overkill_damage` (185-256, about 5-7% of all damage dealt) is what that accidental concentration
   costs, and it is the honest measure of the same behaviour. It is a headline metric.

An earlier attempt at this counter — "ticks where two or more companions share a target" — read
551-646 out of 580-734 ticks, over 90% saturated, and was replaced before it could mislead the tuning
campaign. `peel_ticks` has the mirror flaw: it is highest (687) on the run the squad *lost*, because
it rises whenever teammates are hurt. `unanswered_peril_ticks` was added to name the actual failure.

## Discriminating metrics, in the order they should be watched

| Metric | Baseline | Wanted | Why it is trustworthy |
|---|---|---|---|
| `overkill_damage` | 185-256 | down | Direct cost of piling onto a dying target |
| `unanswered_peril_ticks` | 126-712 | down | A companion in trouble with nobody answering |
| `hazard_ticks` | 40-76 | down | Standing in fire; today only reacted to, never avoided |
| `trap_ground_kills` | 4-7 of 16 | up | Prepared ground paying off |
| `isolated_downs` | 0-1 | down | Rare; a guard rather than a target |
| `focus_distinct/holder` | ~1.05 / ~3.2 | not worse | Guard against wrecking existing concentration |

`investigator_downs` and `tick` stay as outcome guards. Per-kit evidence comes from `kit_casts_by`;
the aggregate `w_casts` cannot show that every kit fired.

## Slice 1 — squad intent board and rescue arbitration

Claims published and read; the only consumer so far is rescue arbitration. Same six seeds.

| seed | outcome | tick | dmg taken | downs | revives | overkill | hazard | unanswered | isolated |
|---|---|---|---|---|---|---|---|---|---|
| 1 | victory | 610 | 376 | 2 | 2 | 198 | 26 | 156 | 0 |
| 2 | victory | 707 | 545 | 2 | 2 | 257 | 49 | 257 | 0 |
| 3 | victory | 581 | 391 | 0 | 0 | 207 | 67 | 126 | 0 |
| 4 | **victory** | 786 | 766 | 3 | 3 | 222 | 57 | 693 | 0 |
| 5 | victory | 671 | 533 | 1 | 1 | 206 | 41 | 252 | 0 |
| 6 | victory | 703 | 580 | 1 | 1 | 217 | 68 | 223 | 0 |

Totals against baseline: victories 5/6 to 6/6, downs 11 to 9, revives 7 to 9, damage taken 3331 to
3191, isolated downs 1 to 0. Seed 3 is byte-identical, as it should be: nobody goes down on it, so no
rescue decision is ever reached. Seeds 2 and 6 take slightly more damage; the gain is not uniform.

Seed 4 is the whole result. It was the baseline's only defeat, at 4 downs and **0 revives**: the squad
lost, having never completed a single revive. It now wins with 3 downs and 3 revives and all 16 kills.

The mechanism is worth recording, because the first attempt at this fix did nothing at all. Choosing
the *nearest* casualty instead of the first in roster order produced output identical to baseline on
every seed, because the sandbox rarely has two casualties at once. A probe log showed the actual
behaviour: at tick 448 of seed 1, **three** companions published a rescue claim on the same body.
Rescue is Locked on all three channels, so each of those bots had dropped out of the fight entirely to
crowd one revive that only one of them could perform - while the enemies that downed the ally kept
firing. The first rule ("a claim is a preference, not a prohibition") deliberately permitted this, to
avoid abandoning a lone casualty; it was the wrong call.

Yielding to a better-placed claimant - closer, ties broken on roster index so two equidistant rescuers
can neither both yield nor both go - leaves exactly one rescuer and keeps the rest shooting. If that
rescuer goes down, its claim ages off the board within two ticks and the next closest takes over.

Note what this says about the metric set: the change that flipped a loss into a win moved
`investigator_downs` and `revives`, both of which already existed. The new counters earned their place
differently - `isolated_downs` going 1 to 0 and `hazard_ticks` falling on four of six seeds are
consistent with a squad that no longer abandons its position to crowd a body, which is the behaviour
the change was actually aimed at.

## Slice 2 — why this target rather than the nearest

Companion focus scoring gains additive curved terms, in the same distance units the formula already
subtracts, so `TargetCommitment` and the ping bonuses keep the meaning they were tuned with. Enemies
are untouched. Six seeds:

| seed | outcome | tick | dmg taken | downs | overkill | hazard | trap kills |
|---|---|---|---|---|---|---|---|
| 1 | victory | 570 | 409 | 1 | 205 | 19 | 8 |
| 2 | victory | 637 | 421 | 1 | 203 | 34 | 6 |
| 3 | victory | 682 | 440 | 0 | 154 | 63 | 6 |
| 4 | victory | 636 | 436 | 1 | 201 | 23 | 8 |
| 5 | victory | 677 | 493 | 1 | 244 | 37 | 3 |
| 6 | victory | 667 | 449 | 2 | 202 | 32 | 6 |

Against the baseline: victories 5/6 to 6/6, downs 11 to 6, damage taken 3331 to 2648 (-21%), hazard
ticks 385 to 208 (-46%), overkill 1293 to 1209, trap kills 31 to 37, and every enemy dead on every
seed. Against slice 1: downs 9 to 6, damage 3191 to 2648.

### The first attempt was worse than no change at all

Six terms went in together on judgement: finish the wounded, kill what is hurting us, peel for an ally
in peril, prefer company, press a suppressed target, prefer elites. That set turned a 6/6 record into
5/6 and raised downs from 9 to 13. It did cut overkill by 12%, which is what the terms were aimed at,
so the headline metric alone would have called it a success.

Ablating one term at a time found the cost was not spread evenly.

| dropped | wins (of 3) | downs | overkill |
|---|---|---|---|
| nothing | 2 | 9 | 661 |
| finish the wounded | 2 | 10 | 568 |
| **kill what is hurting us** | **3** | **3** | 630 |
| peel | 2 | 9 | 607 |
| prefer company | 3 | 5 | 628 |
| press the suppressed | 2 | 9 | 646 |
| prefer elites | 3 | 5 | 761 |

Two terms were doing the damage:

- **Kill what is hurting us** (`TargetThreatToAllies`) was the worst single term. Nearly every living
  enemy is attacking somebody, so it does not identify a threat, it re-ranks targets by how far into
  the fight they are - and it keeps pulling bots off a target they are two hits from finishing.
- **Prefer elites** cost the most overkill of any variant. Elites are tanky; the squad spent longer
  under fire for the same kill. Elite worth already earns its keep in `EliteWorth` on cast valuation,
  which is a different decision.

Both are removed rather than zeroed. A weight-zero knob is a dead knob, and this work already found one
(`PingFocusScore`, declared and never read) that had been sitting in the tuning surface.

Note that **finish the wounded costs overkill** (dropping it lowered overkill from 661 to 568) while
saving downs. That is not a defect: everyone piling onto the nearly-dead target is what finishing *is*.
Overkill is the price of the behaviour, worth paying up to a point, which is what the company bell is
for.

### Two cautions about the method

Ablating over three seeds said prefer-company was harmful. Over six, with the threat term gone, it is
clearly helpful: adding it took downs from 11 to 7 and damage from 2937 to 2744. **Small ablations
disagree with each other**; single-term results only held up where the effect was large, as with the
threat term.

The whole-configuration comparison was run through `-DMAIWeights` with a `"*"` section, which applies
to *every* profile including the enemies - who have no focus terms in the shipped defaults. It ranked
configurations usefully but its absolute numbers are not comparable to a real run, and the final build
measured better than that test predicted (6 downs against 7). Prefer a rebuild for a final figure.

### What shipped

| term | input | curve | weight |
|---|---|---|---|
| finish the wounded | `TargetHealthFrac` | InverseQuadratic 0..1 | 260 |
| peel for an ally in peril | `AllyInPeril` | Linear 0..1 | 320 |
| join a pair, not a crowd | `AlliesOnTarget` | Bell 0..3, peak near 1 | 150 |
| press the suppressed | `TargetSuppressed` | Step | 110 |

`AllyInPeril` is measured against `ThreatenedAllyHealth`, not against full health. Scaling from full
health made it fire for any ally with a scratch and moved every time anyone took a hit, so the focus
was dragged around continuously and bots walked between targets instead of shooting one. Keyed to the
peril threshold it is silent until someone is actually dying.

`AlliesOnTarget` is a bell rather than a slope for the reason the baseline established: companions
already converge without being told to, so rewarding company on a slope reinforces a pile-on that is
already total. The peak sits near one other bot - pair up, but leave the third and fourth to find
their own target.

All four terms are hand-picked and remain untuned; they are new knobs for the tuning campaign.

## Slice 3 — choosing where to stand

Whiskers (12 traces per companion per tick) tell the pure scorer how far it can actually walk in each
direction; `ChoosePosition` scores sampled stand-points on incoming danger, hazards, the distance the
intent wants, teammate spacing, prepared ground and travel cost. Wired into fleeing, hazard evasion
and a new `Reposition` action. Six seeds:

| seed | outcome | tick | dmg taken | downs | overkill | hazard | trap kills |
|---|---|---|---|---|---|---|---|
| 1 | victory | 701 | 426 | 1 | 270 | 48 | 5 |
| 2 | victory | 599 | 293 | 0 | 156 | 31 | 6 |
| 3 | victory | 555 | 316 | 1 | 143 | 35 | 8 |
| 4 | victory | 625 | 372 | 1 | 102 | 19 | 6 |
| 5 | victory | 507 | 313 | 0 | 120 | 27 | 5 |
| 6 | victory | 579 | 293 | 0 | 216 | 29 | 4 |

### Approaching and repositioning are not the same problem

The first build scored the approach as well, and it was the worst result of the whole exercise: three
of six seeds timed out at 1801 ticks with **five kills between them**, taking almost no damage. Given a
say in whether to close, bots declined. Danger opposes engaging, and for an Engage intent engaging is
the job.

Constraining it so every candidate must close on the focus fixed the standing-off and was still worse
than walking straight in - 8 downs against 5, and 3019 damage taken against 2201.

| Engage | wins | downs | dmg taken |
|---|---|---|---|
| scored freely | 3 (3 timeouts, 5 kills total on two seeds) | - | - |
| scored, must close | 6 | 8 | 3019 |
| **straight at the focus** | **6** | **5** | **2201** |

This arena has no cover. The shortest path is therefore also the least time under fire, and every
prettier approach is only a longer one. `EDMAIIntent::Engage` was removed rather than left unused.

Repositioning *once already in range* is the opposite result, because it costs no shooting: the Move
channel is free while the Attack channel keeps firing. Adding it took downs from 5 to 2. The range
limit matters more than expected - clamping candidates to Engage's stop band rather than the full
attack range left so few options that the bot usually had nowhere to go, and gave back four of the six
downs it had saved.

| Reposition | downs | dmg taken |
|---|---|---|
| none | 5 | 2201 |
| clamped to the stop band | 6 | 2277 |
| **clamped to attack range** | **3** | **2013** |

### Against the baseline

| metric | baseline | now | |
|---|---|---|---|
| victories | 5/6 | **6/6** | |
| investigator downs | 11 | **3** | -73% |
| damage taken | 3331 | **2013** | -40% |
| overkill damage | 1293 | **1008** | -22% |
| hazard ticks | 385 | **189** | -51% |
| unanswered peril ticks | 1707 | **665** | -61% |
| trap ground kills | 31 | **34** | +10% |

Every enemy dies on every seed and all four investigators are standing at the end of all six.

Damage taken fell 40% while downs fell 73%: the squad is not merely taking less, it is spreading what
it takes, which is what the spacing terms are for.

`Foundation.UtilityAI.Positioning` covers the case this work started from - a companion falling back is
pulled toward a wire a *teammate* laid. Before the squad board that was not merely unchosen but
unknowable, since markers were private to the bot that placed them.

Positional weights are hand-picked and untuned. The danger weight is deliberately cut to 35% for
repositioning: standing in range of what you are shooting is dangerous by definition, and a bot that
weighed it the way a retreating one does would simply leave.
