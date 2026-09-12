# Abilities

Every investigator now has Basic, Passive, Q and the named W/E/R of GDD Appendix K.
This page covers the shared targeting model and the four Q abilities; the named kits
have their own page, [named kits](kits.md). Evolution branches remain deferred.

## Q

Q (LB) enters targeting; LMB (A) confirms; RMB (B/Escape) cancels. A controller uses the selected target or left-stick ground aim, with Tab/RB cycling targets. Medium targeting includes allies. Q is server-authoritative GAS; invalid targets, cooldowns, range, line of sight and resource failures have explicit feedback and spend nothing.

* **Satchel Charge:** place at valid ground within 650 units; costs one Prepared Charge, arms in 0.5 seconds. Armed charges automatically detonate when a living enemy enters their unobstructed blast radius; only the triggered charge is spent. F/Y can still detonate all owned armed charges manually. Each persistent charge hits visible enemies within 220 units for 55 damage. Placement cooldown 0.8 seconds. Scavenged components refill the finite stock.
* **Frame the Subject:** stationary channel up to 3 seconds, within 850 units. Builds 20 Exposure/second, amplified by Perfect Moment during the enemy's visible attack windup. Movement, invalid target or broken sight interrupts; completion/interruption starts a 2-second cooldown. Basic attacks pause during framing.
* **Bind Spirit:** bind an ally, enemy or ground point within 650 units. Three persistent spirits, replacing the oldest at capacity; 2.5-second cooldown. Friendly bindings reduce incoming damage; hostile bindings slow; ground bindings protect allies within 180 units. Attention scales strength from 10% to 30%. Spirit Lash and Thin Places feed matching/nearby spirits.
* **Clinch:** grab within 180 units for up to 1.5 seconds. Recast Q to throw 300 units in the aimed direction, using collision sweeps. Held enemies cannot move or attack; common enemies are eligible, elites require an explicit Break-vulnerable state. Release starts a 4-second cooldown. Grabbing/throwing builds Momentum. Downing either participant clears the hold. On an elite, the hold and throw control cannot outlast its Broken window.

Bot Q casts follow one conservation rule. A cast scores its expected value (effect magnitude across affected targets, extra worth for elites and marked targets, hit probability for delayed casts) against a threshold that rises as Prepared Charges run low and with cooldown length; casts that are out of range, redundant (a satchel already placed, an ally already bound, Exposure already high, a restrained target) or aimed at a target that will die first are vetoed. A Sapper therefore keeps its last charge for two enemies or an elite and seeks resupply when empty; a Medium binds a threatened ally before an unbound enemy; a Photographer frames while Exposure is low. Thresholds and magnitudes are provisional.

All numbers are provisional sandbox tuning, not amendments to GDD Appendix K. Evolution branches are not added. The [Break/Resolve component](break-cc.md) enforces elite control and recovery; scenario mechanics remain deferred. The supplied investigator skins now use retargeted combat animations; spells use particles, range rings, binding markers and brief enemy windup telegraphs. See [combat presentation](combat-presentation.md).

Replication covers cooldowns, channels, held actors, placed markers and per-target/per-spirit resources. The network probe compares their final state on both clients; PIE integration tests exercise the four Q paths and rejected casts. Physical controller feel still needs a hands-on playtest. Infrastructure smoke profiles retain their declared basic-attack tuning and disable bot Q choices; interactive/network runs use Q.

Interactive/network raiders use 350 HP so resource and Q loops have time to develop. The existing smoke profiles keep their explicit HP overrides. This is development tuning, not a balance conclusion.
