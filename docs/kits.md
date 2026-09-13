# Named investigator kits

Every investigator has Basic + Passive + Q/W/E + R (GDD 4.5). Q is described in [abilities](abilities.md); this page
covers the named W/E/R abilities at their **A nodes** only. The evolution lattice (B-F) is not implemented, and
`ability_evolutions` is a declared omission in captures.

W/E/R live in `UDMKitComponent` beside Q's `UDMPrimaryComponent` and follow the same shape: the server validates,
a server-only GAS shim activates, `Resolve` applies the ability, and `Step` advances persistent effects inside the
combat tick. `Validate` never touches the game mode, so a client previews exactly the rejection the server would
give. Rejected casts spend nothing. All numbers below are provisional sandbox tuning, not GDD balance.

## Controls

Q/W/E/R aim; left-click or A confirms; right-click, Escape or B cancels. Self-cast abilities fire on press with
nothing to aim. Pressing the slot that is already aiming cancels it. Keyboard movement is right-click only and
revive moved to V, because Q/W/E/R take the conventional MOBA keys (GDD 3.2).

## Shared control layer

Control effects convert through the Break/Resolve layer (GDD 4.4 LOCKED, O.6):

- **Common enemies** take the full effect: damage, slow, displacement, stagger and interrupt.
- **Unbroken elites** take the damage and half the slow, no displacement, stagger or interrupt, and bank the
  Break pressure instead.
- **Broken elites** take the full effect. `bBreakVulnerable` is the single source of truth for "broken".

Break uses the authoritative Resolve component described in [Break/CC](break-cc.md). Its values are
provisional encounter data. Broken control durations and clinch end at recovery. An independent interrupt
window permits interrupts without granting hard control. Ultimates feed the
[Madness core](madness-core.md), with private current/floor and timed Crisis recovery.
All four [Madness families](madness-families.md) now have sandbox effects; actual Mythos boss encounters remain unimplemented.

Slows stack by strength rather than overwriting, so a root is not cut short by a weaker slow landing on top.

## Trench Raider / Sapper

**W - Suppressing Fire.** A cone fired from where the Sapper stands: 25 degrees either side of the aim direction,
600 units long, lasting 3 seconds. It is fire-and-forget, so the Sapper keeps moving and shooting while it burns.
Every half-second each enemy inside it with line of sight from the cone's origin takes 4 damage and is Suppressed
for one second. Suppression is the carbine's damage tag (+15%) plus a 0.4 slow and a little Break pressure through
the control layer, so an unbroken elite is slowed half as hard and banks the rest. A second cast replaces the
first; there is never more than one live cone.

**E - Tripwire.** Two ends placed independently, each on valid ground within 650 units, between 60 and 500 units
apart with a clear line between them. The first press only arms the placement and costs nothing; the wire is not
laid, and no cooldown starts, until the second end lands. A pending end is dropped by cancelling, by going down,
by a control handover, or after six seconds. The wire arms half a second after placement and triggers on the first
enemy whose movement crosses it, dealing 15 damage, rooting for two seconds, staggering, interrupting and applying
strong Break pressure. It is spent by that first crossing: repeat triggers belong to the Resetting Fuse node.
At most two wires are live; a third evicts the oldest. Tripwire costs no Prepared Charge.

**R - Dead Ground.** For six seconds, traps that would legitimately trigger record the enemy that would have
triggered them instead of firing. After a shared 1.5-second delay every recorded trigger resolves at once. A
satchel is an area trap and tags everyone in its blast; a wire tags only its first crosser, exactly as it would
have triggered. Traps that never gain a trigger stay armed and unspent, and the window never manufactures a
trigger that would not otherwise have happened (GDD K.2). Traps are identified by a stable serial, so detonating
a tagged satchel by hand resolves it once and drops its record. Activation spikes Madness. It cannot be cast
without an armed trap to defer.

## Expedition Photographer

**W - Flashbulb.** A short cone flash, 35 degrees either side, 350 units. Every enemy caught takes 25 Exposure,
a half-second stagger and a brief slow, and banks a little Break pressure. Perfect Moment still applies, so a
subject caught mid-telegraph gives up more. The A node deliberately has **no** interrupt: a hard interrupt is
what Blinding Flash adds. Castable while framing.

**E - Develop.** Consumes the stored Exposure on one enemy within 850 units and deals 0.7 damage per point, so
the payoff is exactly what was invested in that subject. Refused when the subject has no Exposure. Castable
while framing, which is when the Exposure is being built.

**R - Impossible Photograph.** Captures the visible battlefield: every living enemy within 1200 units with line
of sight is brought to 80 Exposure, and for eight seconds stored Exposure stops decaying and Develop no longer
spends it, so the same readings can be developed repeatedly. Develop's cooldown drops while the window runs, and
a Develop already cooling down comes back on the shorter wait rather than the one it started: an altered state
answers now, not once the previous wait runs out. Activation spikes Madness. It cannot be cast with nothing in
view.

## Stage Medium

**W - Beckon.** Every bound spirit detaches and travels to a chosen point, spreading around it so they do not
stack. A travelling spirit's passive presence is suspended on the way, except during Open Seance, where the
spirits are fully manifest and stay active as they cross. Each one arriving makes a single pulse: allies within
180 units gain shield scaled by that spirit's Attention, enemies are slowed and take Break pressure. Spirits
arrive as ground presences, so a called spirit leaves whatever it was attached to.

**E - Intercession.** Calls on the highest-Attention spirit, ties broken on the lowest id so the choice is
stable. What it does depends on where that spirit is bound: an enemy binding takes damage scaled by Attention,
a strong slow, displacement away from the Medium and heavy Break pressure; an ally binding gains shield and
damage reduction that is held across the next combat step; a ground binding shields allies and slows enemies
within 180 units. It normally exhausts the spirit, leaving a quarter of its Attention. Refused below 20
Attention, so a spirit is kept for a real intervention rather than spent because it was available.

**R - Open Seance.** For eight seconds the bound spirits fully manifest: passive presence doubles, travelling
spirits stay active, and Intercession no longer exhausts the spirit it calls on. Beckon and Intercession both
come off cooldown faster while it runs. Activation spikes Madness. It needs at least one bound spirit.

## Bare-Knuckle Smuggler

**W - Shoulder Through.** A half-second charge along the aimed direction. Each enemy the run reaches is hit once
for damage, displacement along the charge and a stagger, and each contact builds Momentum. World geometry stops
the charge; bodies do not, so the Smuggler barges through a crowd rather than stalling on the first shoulder.
The charge owns the Smuggler while it runs: no basic attack lands out of a shoulder barge, and the owning client
stops feeding movement so it does not predict against the server.

**E - Dig In.** A two-second brace: incoming damage is reduced, control and displacement resistance rises, and
absorbed pressure converts into extra Momentum. Recasting E while braced ends it with a counter-shove that
damages, displaces and staggers everything within 200 units and carries Break pressure. Letting it run out ends
it quietly. Either way the cooldown starts from the end of the stance, not its beginning.

**R - Drowned Man Walking.** For eight seconds Momentum cannot fall below a high floor, basic attacks gain
unnatural reach, Shoulder Through displaces half again as far, and braced Momentum generation doubles. Clinch
becomes more effective without waiving the Break layer: on a common or already Broken target the hold lasts
longer and the throw goes farther, and a grab on an unbroken elite lands as pure Break pressure instead of a
hold. Activation spikes Madness.

## Bots

Companions choose W/E/R through the same utility brain and conservation model as Q: an option's value is scored
against a threshold that rises with scarcity and cooldown length, and wasted casts are vetoed. An ultimate's
cooldown is capped for threshold purposes, because a 600-tick cooldown would otherwise raise the bar beyond
anything a reachable fight produces, and lowering the shared cooldown weight to compensate would loosen every Q
on the same profile.

A Sapper bot suppresses where enemies cluster, worth half again as much where it has already prepared ground;
lays wire across an approach but never behind a retreating enemy or in front of one already past it; and holds
Dead Ground until two enemies or an elite stand near an armed trap. A Photographer flashes what its cone would
catch, worth more against a subject mid-telegraph, and treats Exposure as an investment: it develops a subject
once the stored reading is worth more than another frame, and lowers that bar inside the photograph. A Medium
calls its spirits to a threatened ally before the focus, never when they are already gathered there, and keeps a
barely attended spirit rather than spending it because it was available. A Smuggler charges a line rather than a
body already in reach, braces against pressure actually aimed at it, and saves the altered state for a pair or
an elite. `DREAD_AI_RESULT` counts casts per slot, so a slot that never fires is visible in the tuning harness
rather than silently absent.

Only one cast is chosen per tick, so a named ability competes with Q for the same channel. Suppressing Fire is
ranked below the traps deliberately: at equal rank it won while Prepared Charges were still worth spending and
quietly undid the satchel conservation the Q tuning had already established. Ranked below them, the satchel and
the wire are used while they clear their own bars, and suppression is what fires once the last charge is held
back. Ranking says that plainly; a weight chosen to beat the satchel's conservation curve would only hold at one
charge count.

## Verification

`DreadMeridian.Foundation.Kits.Rules` covers the pure geometry, control conversion, Break meter, slow stacking and
the deferred-trap ledger. `Kits.Components` covers the resource and combatant state the abilities build on.
`UtilityAI.Kits` asserts each option fires and holds where intended, with the baked tuning values.
`DreadMeridian.Editor.Kits.<Investigator>` drives each kit in a live PIE world: cone damage and suppression, a
wire that ignores an enemy standing clear and triggers on a crossing, Dead Ground deferring a blast and
resolving it after the delay; a flash that exposes and staggers, Develop spending that reading, and the
photograph holding readings while it runs; spirits called across the field and pulsing where they land, an
intervention that exhausts its spirit except inside the seance; a charge that carries and displaces, a brace
that blunts a blow and ends in a shove, and an altered state that lands a grab on an unbroken elite as Break
pressure rather than a hold. See [combat presentation](combat-presentation.md) for the clips and effects.
