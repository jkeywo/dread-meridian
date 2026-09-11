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

`ADMCombatPlayerController` creates Enhanced Input mappings. Attack/revive RPCs
act through the caller's possessed investigator. CharacterMovement handles
prediction/server correction. The open arena uses direct destinations; general
obstacle navigation remains future work.

`ADMSquadController` is a thin shell over the pure utility brain in `DMUtilityAI`
(no world, no UObjects, no RNG). Each logical tick it builds a context from
character-appropriate perception, asks `Decide` for a decision, executes it
through the existing combatant verbs and traces it; policy and commands are
identical in interactive and headless play. Every candidate action is an option
with a rank (Locked > Reflex > Tactical > Routine) and a score in [0,1] from
curved considerations. Options are walked in stable order and each reserves only
the Move, Attack or Cast channels it needs, so a retreat and a basic attack
coexist in one tick. Signatures and base Q must beat a conservation threshold
that rises with scarcity and cooldown, scaled by hit probability, and are vetoed
when wasted. Enter/exit latches (return home, flee, keep distance), runtime caps
and decision cooldowns stop oscillation. The attack channel is a separate hold
gate on the combatant: the focus target stays set for aggro cues and Gunman
positioning even while the bot is told not to fire. Weights are `UDMAIProfile`
data assets per role/hero with C++ defaults, so headless runs need no content;
`-DMAIWeights=<json>` overrides them for tuning. Decisions use roster order and
stable sorts, never random streams. `ai.decision` traces carry the option and
consideration breakdown whenever a bot's focus or chosen actions change.
`PawnLeavingGame` is overridden because Unreal normally destroys the pawn. A new
human fills the first bot-owned slot: this is backfill, not authenticated
restoration of a returning player's identity.

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
