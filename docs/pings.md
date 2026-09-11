# Pings

Pings are how humans steer bots and each other (GDD 9.3, locked grammar; VERB-PING-001).
One key: **G** on keyboard, **D-pad up** on a controller. A tap (released under 0.3 s)
infers the kind from what is under the cursor; a hold opens a compact radial and release
picks the sector under the cursor. Bots read the same grammar as humans. A ping influences
priorities; it is not a squad order.

| Kind | Tap context | Radial | What bots do | Ends |
| --- | --- | --- | --- | --- |
| Enemy | living enemy | no | raise that enemy's priority; idle bots go and look | target dies or 15 s |
| Focus | | yes | companions kill this one first | target dies or 30 s |
| Ignore | | yes | that enemy scores far lower, never zero (bots still defend themselves) | 30 s |
| Go Here | ground | yes | companions move to the point, then hold | author cancels or 30 s |
| Defend | | yes | hold near the point; engage only enemies within 500 units of it | author cancels or 30 s |
| Retreat | | yes | companions disengage toward the point; overrides the low-health retreat goal | 20 s |
| Help | downed or hurt ally | yes | rescue if down, else move to the ally and fight their attacker | ally gone, or 30 s |
| Pickup | scrounge pickup | no | Sapper bots prefer that pickup | collected or 30 s |
| Perceive | | yes | subjective "I perceive something here": bots go and look, never target it | 10 s |

The radial runs clockwise from the top: Go Here, Defend, Retreat, Help, Focus, Ignore,
Perceive. Tap your own live ping to cancel it; tap someone else's to acknowledge it.
Downed players may still ping Help and Perceive. Perceive never carries a target and
replicates a location only: it never becomes a shared target marker.

Limits: at most three live pings per author; a new ping of a kind you already have live
replaces your older one of that kind; 0.5 s between your pings. The server validates
every ping (living target for Enemy/Focus/Ignore, an investigator for Help, a location
clamped into the arena for the rest) and reports rejections through the Q feedback line.

## Bot responses

A bot that acts on a ping answers **on it** the first tick the ping shapes something it
does (already standing inside a rally, defend or help ring, or leaving an ignored enemy
alone, counts); one that could have taken the task but has not after 2 s answers
**busy**. Perceive and Ignore pings never draw busy, a Pickup ping only from the Sapper,
and enemies never read the board. Both responses show as pips under
the marker beside human acknowledgements, and as `ping.responded` traces. Human pings
weigh 1.0, bot pings 0.5, both scaled by the hero's `PingCompliance` in its AI profile.

Bots author pings through the same board, so they appear on every HUD: **Enemy** when a
bot acquires a target no teammate has pinged (5 s per-bot cooldown) and **Help** on
itself when it starts retreating at low Health.

Markers carry the kind label, author tint, a shrinking age bar and responder pips; there
are no marker actors, only the replicated `ADMGameState::Pings` projection. Pings shape
scores and targets but never override the brain's safety rules: leash, tether, the attack
hold gate and hazard evasion still apply, so a Focus ping across the map does not send a
companion out of reach of the leader. All numbers are provisional sandbox tuning.
