# Architecture

`ADMGameMode` retains the tested run/Ritual model. `ADMCombatGameMode` adds a combat
encounter and four persistent investigator slots. Public `ADMGameState` replicates
phase/ritual only. Seed, RNG banks, threat tables and provenance are server-owned.
This follows [Epic's authority model](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine).

`ADMCombatant` is a replicated Character with an AbilitySystemComponent and
`UDMHealthAttributes`. Humans and bots activate the same server-only
`UDMBasicAttackAbility`. Damage uses instant GameplayEffects and validates
team/range/line of sight/cooldown; Shield absorbs damage before Health. Attributes
use [GAS RepNotify handling](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-attributes-and-attribute-sets-for-the-gameplay-ability-system-in-unreal-engine).

Zero Health leaves investigators Downed in the roster. Downing adds an Injury
counter (two ordinary slots, then Grievous). Revive channels require proximity,
a living rescuer and no incoming damage. Revival restores half Health and reduces
existing enemy threat to one quarter. These values and the nonlinear Grievous
duration curve are AI-origin provisional tuning. Named/burst Injury effects,
treatment, Break and CC remain unsupported.

`UDMKitComponent` owns the W/E/R slots beside `UDMPrimaryComponent`'s Q, with the
same shape: `Request` validates and activates a server-only GAS shim, `Resolve`
applies the ability, and `Step` advances persistent effects inside the combat
tick. `Validate` never touches the game mode, so a client can preview the same
rejection the server would give; combat-active is checked only in Request and
Resolve. Cooldowns, ability windows and placed markers are replicated
projections. `Cancel` runs wherever Q's channel and clinch are already cancelled
(downed, combat complete, control handover, EndPlay) so no stance, floor or
half-placed wire outlives its owner. Pure geometry, Break/Resolve conversion,
slow stacking and the deferred-trap ledger live in `DMKitRules` with no UObjects.

Break and Madness exist only as declared stub meters: Madness is a value the
ultimates spike with no symptoms or floor, and Break is an elite-only Resolve
meter that sets the existing vulnerability flag for a window, then recovers with
temporary resistance. Control effects convert through that layer (GDD O.6):
common enemies take the full effect, unbroken elites take the damage and half
the slow and bank the pressure. Neither is the GDD system, and both remain
declared omissions in capture metadata.

`ADMCombatPlayerController` creates Enhanced Input mappings. Q/W/E/R take the
conventional MOBA keys, so keyboard movement is right-click only and revive moved
to V. One aiming state serves all four slots: self-cast abilities fire on press,
targeted ones enter a preview, and the Sapper's two-point Tripwire keeps aiming
between its two ends. Attack/revive RPCs act through the caller's possessed
investigator. CharacterMovement handles prediction/server correction. The open
arena uses direct destinations; general obstacle navigation remains future work.

`ADMSquadController` is a thin shell over the pure utility brain in `DMUtilityAI`
(no world, no UObjects, no RNG). Each logical tick it builds a context from
character-appropriate perception, asks `Decide` for a decision, executes it
through the existing combatant verbs and traces it; policy and commands are
identical in interactive and headless play. Every candidate action is an option
with a rank (Locked > Reflex > Tactical > Routine) and a score in [0,1] from
curved considerations. Options are walked in stable order and each reserves only
the Move, Attack or Cast channels it needs, so a retreat and a basic attack
coexist in one tick. Signatures, base Q and the twelve named W/E/R
abilities must beat a conservation threshold that rises with scarcity and
cooldown, scaled by hit probability, and are vetoed when wasted. An ultimate's
600-tick cooldown would make that threshold unreachable, so per-ability
`ThresholdCooldownCap` bounds the cooldown term without loosening the shared
knob for every ability on the same profile. An ability that supports another
ranks below it: at equal rank a new option can outbid a tuned resource curve for
the one cast channel a tick allows, and a weight chosen to lose that contest only
holds at one resource count. Enter/exit latches (return home, flee, keep distance), runtime caps
and decision cooldowns stop oscillation. The attack channel is a separate hold
gate on the combatant: the focus target stays set for aggro cues and Gunman
positioning even while the bot is told not to fire. Weights are `UDMAIProfile`
data assets per role/hero with C++ defaults, so headless runs need no content.
The asset wins where it exists, so a stale one shadows newly added abilities;
`tools/create_ai_profiles.py` with `DM_PROFILE_REFRESH=1` re-applies the defaults.

`-DMAIWeights=<json>` overrides them for tuning. Decisions use roster order and
stable sorts, never random streams. `ai.decision` traces carry the option and
consideration breakdown whenever a bot's focus or chosen actions change.
`PawnLeavingGame` is overridden because Unreal normally destroys the pawn. A new
human fills the first bot-owned slot: this is backfill, not authenticated
restoration of a returning player's identity.

Companions also publish what they have committed to on a pure `FDMSquadBoard` the
game mode owns and steps before the Think loop. This is communicated intent, not
shared sight (GDD 4.13): a claim says what its author decided, teammates weigh it,
and nothing is ever told what to do. It does not replicate - the player-facing half
of coordination is the ping board, which already has a locked grammar and a HUD.
Claims live two ticks rather than one. Bots Think in roster order and execute before
the next builds its context, so within a tick a later bot hears an earlier one but
not the reverse; a two-tick lifetime makes the exchange symmetric at the cost of one
tick of staleness, instead of splitting the tick into gather and commit phases.
Enemies neither publish nor read: their coordination is the Gang Boss's replicated
orders. The first consumer is rescue: a companion yields a casualty to a better-placed
claimant, so one bot revives and the rest keep fighting rather than all dropping the
fight onto one body.

Companion target choice adds curved terms to the focus score: finish the wounded,
peel for an ally below `ThreatenedAllyHealth`, press a suppressed target, and join
a teammate rather than a crowd. Weights are in the same distance units the formula
already subtracts, so commitment and the ping bonuses keep their tuned meaning.
The company term is a bell rather than a slope because companions already converge
on one enemy unaided, so rewarding company on a slope only buys overkill. Enemies
keep the threat-table formula until their own port.

Companion movement scores where to stand rather than deriving one point from a
formula. `BuildContext` traces twelve whiskers per companion per tick, since the
pure layer cannot trace and the arena has no nav mesh - a bot steers straight at
its goal, so a point behind a crate is a point it grinds into. `ChoosePosition`
then scores sampled stand-points on incoming danger, hazard overlap, the distance
the intent wants, teammate spacing, prepared ground and travel cost. Standing
still and the old formula's point always compete, so it cannot do worse than what
it replaces. Ground claims put a Sapper's armed wire on the board, so a companion
falling back is pulled across prepared ground a teammate laid.

It applies to leaving a fight, not joining one. Scoring the approach was measured
and reverted: with no cover, the shortest path is the least time under fire, and
bots given a say in whether to close declined. `Reposition` (Routine, Move, below
Engage, latched) moves a bot that is already in range to a better spot while it
keeps shooting, which costs no damage output because the Attack channel stays free.

`ADMCombatGameMode` owns a pure `FDMPingBoard` (limits, lifetimes, fulfilment,
acknowledge, cancel, respond) and steps it before the Think loop so bots see a
settled board. `ADMGameState::Pings` replicates the live pings as the public
projection every HUD draws; Perceive pings carry a location only. Human gestures
arrive through `ADMCombatPlayerController::ServerPing`, validated on the server.
Bots consume pings as focus rules, score modifiers and move actions, answer
on_it/busy, and author Enemy and Help pings through the same path. `ping.created`,
`ping.responded` and `ping.ended` are developer traces. See [pings](pings.md).

Decisions, revive channels, abilities and end checks run in stable roster order
at 0.1 logical seconds per step. Smoke profiles accelerate the timer but retain
logical cooldown/channel ticks. Movement/physics remain engine-authoritative and
frame-dependent. This is not whole-game deterministic replay. Named RNG streams
have versioned restore; seeded RunGeneration chooses sandbox spawn offsets.
RNG snapshots are not full host-migration saves.

The native map places `ADMSandboxArena` floor/walls/light composition. Primitive
bodies and diagnostic HUD are presentation placeholders. Editor-only Python
creates the map once, preserving subsequent manual edits.

`PlaytraceCapture` observes accepted events and cannot influence rules. Captures
declare run kind, bot policy, profile, tuning and omissions. Stable actor/ability
IDs and logical ticks accompany combat/control events. The synchronous writer
suits this low-volume slice; measure and add bounded asynchronous capture before
scaling. Developer captures are not player-visible replication or migration saves.

Run/RNG unit tests remain. Runtime profiles test victory, TPK and rescue. A
loopback test checks disconnect takeover and final Health/Shield/GameState on
two clients. Matchmaking, packet-loss recovery, subjective entitlement and
physical controller feel remain separate acceptance work.
