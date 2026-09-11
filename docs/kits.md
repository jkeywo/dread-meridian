# Named investigator kits

Every investigator has Basic + Passive + Q/W/E + R (GDD 4.5). Q is described in [base Q](base-q.md); this page
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

Break is an **elite-only stub**: pressure accumulates to a threshold, the enemy is Broken for a window, then
recovers with temporary resistance to further pressure. Madness is a **stub meter** that the ultimates spike and
nothing else reads; it has no symptoms, no floor and no decay. Neither is the GDD system, and `madness` and
`break_cc` remain declared omissions.

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

## Photographer, Medium, Smuggler

Not implemented. Their W/E/R slots report themselves unavailable and the HUD draws them as empty.

## Bots

Companions choose W/E/R through the same utility brain and conservation model as Q: an option's value is scored
against a threshold that rises with scarcity and cooldown length, and wasted casts are vetoed. An ultimate's
cooldown is capped for threshold purposes, because a 600-tick cooldown would otherwise raise the bar beyond
anything a reachable fight produces, and lowering the shared cooldown weight to compensate would loosen every Q
on the same profile.

A Sapper bot suppresses where enemies cluster, worth half again as much where it has already prepared ground;
lays wire across an approach but never behind a retreating enemy or in front of one already past it; and holds
Dead Ground until two enemies or an elite stand near an armed trap. `DREAD_AI_RESULT` counts casts per slot, so a
slot that never fires is visible in the tuning harness rather than silently absent.

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
`DreadMeridian.Editor.Kits.Sapper` drives the three abilities in a live PIE world: cone damage and suppression,
a wire that ignores an enemy standing clear and triggers on a crossing, and Dead Ground deferring a blast and
resolving it after the delay. See [combat presentation](combat-presentation.md) for the clips and effects.
