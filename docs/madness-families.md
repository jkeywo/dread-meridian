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
