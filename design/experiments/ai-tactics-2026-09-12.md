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
