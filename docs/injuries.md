# Injuries

Implements GDD 4.7 and M.1–M.3 themes. The GDD is unchanged: these exact effects,
thresholds and timings are **AI-origin provisional sandbox tuning**, configurable
on `UDMInjuryComponent` and in `Config/DefaultGame.ini`.

Actual Health lost within the last 30 combat ticks (3 seconds) triggers an Injury
at 35% of maximum Health. Shield absorption and overkill do not count. Downing
also triggers an Injury. A lethal burst grants one Injury, and either trigger
consumes the accumulated window so the same damage cannot trigger repeatedly.
Healing does not erase recent damage or remove Injuries.

Selection is random among missing names until two distinct Injuries are held;
further triggers add Grievous. Treatment removes one Grievous stack first, then
the oldest named Injury. Nothing permanently incapacitates an investigator.
Revive duration remains `20 + 5 * Grievous²` combat ticks, with safe saturation.
Revival restores half Health without removing Injuries; ordinary healing cannot
revive a Downed investigator.

| Injury | Provisional effect |
|---|---|
| Broken Ribs | A heavy hit with at least 15% maximum Health unshielded potential damage deals 1.25× damage when another heavy Health loss occurred within 30 ticks. |
| Concussion | Two accepted casts less than 15 ticks apart impose a 10-tick casting pause after the second cast. |
| Wounded Arm | Three landed basics, each less than 15 ticks after the previous, impose a 10-tick attack pause. |
| Twisted Knee | Travel of 300 units within a 10-tick accumulation period, or actual forced displacement, causes a 10-tick 40% limp. |
| Deep Cut | Healing is halved for 30 ticks after a burst threshold is crossed. |
| Burns | Persistent hazard damage is multiplied by 1.5 when another hazard caused Health loss within 20 ticks. |

The two damage multipliers stack. Existing defenses apply first; amplified
damage then passes through Shield. Smuggler burning ground is marked as a
persistent hazard; its initial explosion is not. Cast and attack gates apply
to the same authoritative APIs used by humans and companions. HUD conditions
show the held names and their effects.

The encounter has a two-charge shared treatment fixture at `(-2200,350)` and a
one-charge food fixture at `(-1900,-350)`. Within 160 units and line of sight,
press **T / D-pad left** for treatment. Nearby companions use the same acceptance
API automatically. Charges are spent only on accepted use. Food auto-collects
for hurt investigators and heals 6 Health each second for five pulses. It cannot
stack with active food healing, never treats Injuries, and stops on downing.
These fixtures do not implement medical objectives or the GDD food-placement
subset selection system.

`FDMInjuryState` holds authoritative rolling history; clients receive names,
counts and active effect projections. The appended Injury RNG stream has ID 10;
earlier stream IDs and seed mixing are unchanged. RNG snapshot schema 2 requires
all eleven streams and rejects schema 1 atomically. Historical capture provenance
with RNG schema 1 remains readable. Rules copy/restore tests are not a full run
save, deterministic gameplay replay or host migration implementation.

Captures identify combat rules `injuries-v1`, capture version `0.5.0`, and RNG
schema 2. Accepted changes emit `injury.gained`, `injury.treated`, `recovery.used`
and `combat.healed`. Play Trace validates healing against preceding Health and
maximum Health; Injury events remain observations, not replay instructions.
