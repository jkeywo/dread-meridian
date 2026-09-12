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
