# Madness core

Implements the state and ownership foundation in GDD 4.6 and M.4–M.5. The GDD
remains canonical and unchanged. Numerical defaults and grounding are **AI-origin
provisional sandbox tuning**, not approved balance.

`UDMMadnessComponent` owns current Madness, an irreversible floor, unlocked symptom
bands, Crisis timing and explicit recovery. Current is bounded by the floor and
100. `RaiseFloor` takes an absolute new floor and refuses reductions; it raises
current if needed. `Recover` reduces current only. Invalid and non-finite requests
are refused. There is no passive decay. Existing R abilities feed their accepted
30-point pressure into the core through the kit compatibility entry.

| Setting | Provisional default |
|---|---|
| Early / mid / high symptom pool thresholds | 25 / 50 / 75 |
| Crisis trigger | Current reaches 100 |
| Crisis duration | 100 combat ticks (10 seconds) |
| Recovery at Crisis end | Subtract 50, bounded by floor |
| Minimum interval before another Crisis | 100 ticks after Crisis ends |
| Grounding | 30 ticks of quiet, then subtract 10 |

An active Crisis is not extended by further pressure and is not cancelled by
ordinary recovery. A floor of 100 still gets a real interval outside Crisis;
remaining at maximum enters another Crisis after that interval. Crisis never
reverses inputs, stuns, forces movement or blocks player actions.

Press **H / D-pad right** to ground. Accepted grounding stops current movement
and auto-attack orders; subsequent commands remain available. Movement beyond
10 units, incoming damage (including shield absorption), a landed basic, an
accepted cast, downing or hard control interrupts the attempt. It cannot begin
while a cast is channeling or charging, or at the floor. A successful attempt
is one recovery pulse, not an indefinitely toggled state. Companions can use
the same API, but their utility policy does not yet select grounding.

Madness is private. Neither the component state nor the legacy kit-facing meter
replicates on the combatant. Shared resource summaries and other HUD rows omit
it. A reliable RPC delivers a snapshot only through the investigator's owning
PlayerController. The HUD checks that its entity ID matches the currently
possessed pawn. A new owner receives preserved state on possession, with subsequent
updates on accepted changes and combat steps. Dedicated-server AI reads its own authoritative meter. No private state
is added to GameState or another player's verification payload.

`Manifest` is a server-only content integration API: the caller supplies an
authored symptom ID, private text equivalent, required band and duration after
evaluating context. Locked pools reject manifestation, an active cue cannot be
overwritten, and cues expire on the logical clock. Threshold crossing alone
does not choose or manifest a symptom. The owner HUD has a private text slot;
no shared marker or sound is spawned by this core.

Family assignment, Perception entities, Compulsion urges, Dissociation echoes,
Obsession fixations, family-specific Crisis advantages, boss resonance, and
evolution/occult sources of floor growth remain separate work. The core does
not claim a complete Madness family. No random choice was introduced, so named
stream IDs, RNG schema 2 and stream positions are unchanged. Future random
assignment/symptoms must use named streams.

Capture version `0.6.0` and combat rules `madness-core-v1` distinguish this
implementation. Accepted changes emit `madness.changed`, band changes also emit
`madness.threshold`, and Crisis entry/exit emit their respective events. Accepted
manifestations emit `madness.symptom`. These are developer observations, never
player replication. Rules-state copy tests do not establish host migration or
deterministic full gameplay replay.
