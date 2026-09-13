# Madness families

Family content builds on [Madness core](madness-core.md), following GDD M.6–M.9.
All exact values below are AI-origin provisional sandbox tuning. The GDD is unchanged.
The server assigns one supported family per investigator using the named Madness
stream. Assignment persists through human/bot handoff; it is private. This sandbox
assignment does not claim hidden Elder One selection or resonance.

## Obsession

At the early threshold, one visible living enemy within 1000 units becomes a
fixation. Its private label and last-seen position appear only on the owner's HUD.
Three Health-damaging hits, or killing it, resolve the fixation. Each hit reduces
current Madness by 2, bounded by the floor. Ignoring it adds 5 pressure every 40
ticks. Fixations last 100 ticks; an invalid/dead target is replaced without blame.

Crisis creates one overwhelming fixation requiring five hits or a kill. Each hit
adds a further 10% damage benefit against that target, up to 50%; resolving it ends
Crisis with normal recovery and respite. Movement, attacks and casting stay under
player control. Actual combat damage is shared; fixation ownership and cues are not.
Family intervals, pressure, recovery and selection range live in component settings.

Boss-specific reproductive-node targeting remains pending Shub content.

## Compulsion

At an unlocked threshold, a visible enemy becomes an urge to strike; when none is
available, clear nearby ground becomes an urge to visit. Indulging reduces current
Madness by 4. High intensity or Crisis also grants 5 Shield, using the ordinary
authoritative shield API. A location requires approaching within 65 units.

Crisis offers three simultaneous urges: an enemy and two clear ground locations
when available. Each can be addressed independently. An unfulfilled urge expires
after 100 ticks and adds 5 pressure; an enemy removed by somebody else is discarded
without punishment. Normal Crisis timing remains intact. These are choices, not
movement orders or input restrictions. Objective/corpse urge variants remain future
content; the implemented contextual variants are combat targets and locations.

## Dissociation

Accepted casts create predictable, weaker signature echoes after 20 ticks. Early
Madness echoes Q, mid adds W, and high/Crisis echoes Q/W/E. R and rejected casts do
not queue echoes. Early/mid footprints move with the caster's displacement; high
and Crisis footprints retain the original cast position, aim and sampled strength.
Owner-only markers show where and when they resolve. At most eight can be pending;
downing clears them. Recovery does not cancel already accepted echoes.

Echoes use explicit weak signature effects, not recursive ability activation:

| Investigator | Q echo | W echo | E echo |
|---|---|---|---|
| Sapper | 19.25 damage in a 150-unit blast | 1.4 damage and brief slow in the suppression cone | 5.25 damage and brief stagger/slow along the recorded wire |
| Photographer | 8 Exposure at the framed location | 8.75 Exposure and weaker flash slow/stagger | 35% of Develop's recorded Exposure-based damage at the photographed location |
| Medium | Small shield/slow pulse at the binding | Small shield/slow pulse at the call location | 7 damage and 3 Shield intervention pulse around the original caster position |
| Smuggler | Short control shove | 5.25 damage and short push along the charge path | 3.5 damage and short counter-shove |

These signatures do not create another persistent summon, force another caster
movement, spend resources, reset cooldowns or echo themselves. Existing line of
sight, team, Shield, Injury and Break/control rules govern accepted effects.
The Medium echoes are deliberately brief pulses rather than duplicate spirits.
