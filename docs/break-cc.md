# Break / crowd control

Implements the combat layer in GDD 4.4 (**LOCKED**) and O.6. The GDD is unchanged.
All numeric settings below are **PROVISIONAL** encounter tuning. Mythos boss
encounters, relic/evolution interactions and host migration remain outstanding.

`UDMBreakComponent` owns elite/boss Resolve. Common enemies have no meter. The
component exposes editable settings, with defaults in `Config/DefaultGame.ini`:

| Setting | Sandbox default |
|---|---|
| Max Resolve | 100 |
| Broken duration | 40 logical ticks (4 seconds) |
| Recovery resistance duration | 100 ticks (10 seconds) |
| Pressure multiplier during resistance | 0.5 |
| Protected slow multiplier | 0.5 |
| Fallback pressure for control without authored pressure | 10 |

Encounter setup may override `Resolve->Settings`, then call `Reset()` before
combat. Zero resistance duration disables recovery resistance; a zero pressure
multiplier grants full resistance for the configured interval. Invalid settings
are bounded before use. The Gang Boss uses the protected layer; its faction name
does not make it an implemented Mythos boss.

Common enemies receive full control. Protected targets take damage and the
configured partial slow; stun, stagger and displacement become Break pressure.
Explicit ability pressure takes precedence over the fallback contribution.
Pressure depletes current Resolve. The hit that empties it resolves against the
pre-hit protection, opening Broken for subsequent hits. Additional pressure
cannot extend Broken. At recovery, Resolve refills and temporary resistance
reduces subsequent pressure. Skipping updates never extends either window.

Broken permits full control, but slow/root, stagger, stun and clinch durations
cannot exceed its end. Recovery releases an active clinch. Stagger delays basic
attacks and cancels their telegraph without changing an existing attack cooldown;
stun also stops movement and casts and interrupts channels. An interrupt cancels
pending enemy signatures and orders, basic telegraphs, framing, clinch and charge.
Already released fire persists when its caster is interrupted or restrained.
The Photographer's A-node flash still staggers without acquiring a hard interrupt.
The Smuggler's basic finisher remains common-only, as authored in K.5.

`OpenInterruptWindow` supplies the GDD's **PROVISIONAL** encounter-authored option:
an independent interrupt window admits explicit interrupts while preserving
protection against roots, holds, stun and displacement. It does not manufacture
a Broken state. No existing encounter automatically opens these optional windows.

The whole roster expires control before persistent abilities and enemy signatures
resolve each logical tick. Gameplay accepts control on authority only; bots and
humans use the same ability/control APIs. Current/max Resolve, Broken, resistance,
interrupt-window and control timers are public replicated projections. The HUD
shows remaining Resolve draining toward zero and labels Broken, Resisting and
Interrupt states. There is no hidden boss or RNG state in those projections.

Capture version `0.4.0` declares `combat_rules_version=break-cc-v1`. This is an
intentional simulation change: previous tuning results do not establish the new
rules' outcomes. No RNG stream IDs changed. `break.pressure`, `break.broken`,
`break.recovered`, resistance/window transitions and `control.resolved` observe
accepted state; they never decide outcomes. Pressure includes source/ability IDs
when supplied and the public Resolve state. The broad `break_cc` omission is
removed; missing boss encounters, Madness, evolutions and migration stay declared.

Verification covers pure boundary/data rules, live common and elite control,
clinch recovery and independent interrupt windows. The network probe compares
public Resolve/control projections alongside health and kit state. This does not
establish full gameplay replay, snapshot restoration or host migration.
