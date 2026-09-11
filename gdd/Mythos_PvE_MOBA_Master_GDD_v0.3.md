# Mythos PvE MOBA — Master Game Design Document

**Working title:** Untitled Mythos PvE MOBA  
**Document ID:** GDD-MASTER  
**Version:** 0.3  
**Status:** Draft - reconciled exhaustive MVP specification  
**Date:** 10 September 2026  
**Target milestone:** Vertical Slice

## Document control

| Field | Value |
|---|---|
| Project | Untitled Mythos PvE MOBA (working title) |
| Document ID | GDD-MASTER |
| Owner | Design |
| Status | Draft - reconciled exhaustive MVP specification |
| Version | 0.3 |
| Last updated | 10 September 2026 |
| Target milestone | Vertical Slice |
| Canonical location | TBD |
| Related repositories / drives | TBD |

### Status legend

| Tag | Meaning |
|---|---|
| **LOCKED** | Explicitly selected during the design grill; change through a recorded design decision. |
| **PROVISIONAL** | A concrete implementation or tuning recommendation needed to make the design testable, but not explicitly locked. |
| **OPEN** | Material decision still unresolved. |
| **CONTENT TBD** | Instance-level content is intentionally not invented in this master document; it belongs in structured catalogues/specs. |

### Document purpose

This is the front-door design document for a cooperative PvE game built around traditional MOBA combat, a four-investigator expedition structure, escalating Cthulhu-Mythos ritual pressure, strong in-run hero evolution, subjective Madness, and replayable authored scenarios. It records the canonical design rules and relationships established in the design grill. High-volume content, exact balance values, implementation tickets, dialogue, and asset inventories should live in linked structured sources rather than being duplicated here.

The document deliberately distinguishes locked design from provisional implementation. Where the grill did not settle a question, this GDD does not manufacture certainty.

### Design summary

**DESIGN DIRECTION.** A 1–4 player cooperative PvE game in which a four-investigator team fights through a compact, handcrafted 1920s/30s scenario using traditional MOBA controls and hero kits. The team races an advancing Mythos ritual rather than another player team. While completing a generated-but-authored objective plan, investigators level from a pulp-action baseline into increasingly impossible, Madness-tainted versions of themselves. Optional objectives can award scarce, random, systemic relics. The hidden Elder One is gradually revealed through ritual manifestations, enemy intrusions, and the special interaction between the boss and exactly one investigator’s resonant Madness. The run ends in an Apocalypse phase and climactic boss confrontation, or earlier in a team wipe. Successful runs target a compact repeatable session; exact duration is tuning data, and failing teams should collapse earlier rather than create long losing tails.

## Contents

1. Game identity
2. Core loops
3. Player model
4. Systems index
5. Progression and economy
6. World and level architecture
7. Narrative architecture
8. Content architecture
9. UI / UX
10. Accessibility
11. Art direction
12. Audio and music
13. Technical and platform constraints
14. Tools and content pipeline
15. Localisation
16. Analytics and telemetry
17. Live operations / post-launch model
18. QA and verification
19. Production and milestone maturity
20. Decisions and change control
21. Appendix A — Vertical slice definition
22. Appendix B — Launch scope
23. Appendix C — Principal risks
24. Appendix D - Grill decision register
25. Appendix E - MVP content specification
26. Appendix F - MVP investigator specification
27. Appendix G - MVP scenario and objective specification
28. Appendix H - MVP Elder Ones and intrusion enemies
29. Appendix I - MVP run-state, relic and investigation specification
30. Appendix J - MVP technical and production acceptance
31. Appendix K - Detailed MVP investigator mechanics
32. Appendix L - Detailed MVP scenario and objective mechanics
33. Appendix M - Detailed MVP Madness, Injury, healing and relic mechanics
34. Appendix N - Detailed MVP Elder One encounters and intrusion enemies
35. Appendix O - Detailed MVP technical architecture
36. Appendix P - Detailed MVP production scope and acceptance
37. Appendix Q - Post-110 MVP decision register
38. Appendix R - Design rationale and comparable-system notes
39. Appendix S - Detailed MVP meta-investigation and Leads

# 1. Game identity

## 1.1 One-sentence proposition

**LOCKED.** A premade-first, four-investigator cooperative PvE game that combines traditional MOBA combat and build progression with replayable 1920s/30s Mythos scenarios, where players race an escalating ritual, adapt to hidden Madness and a hidden Elder One, complete pressured objectives, and enter a climactic Apocalypse boss fight.

## 1.2 Player fantasy

The player is a capable pulp survivor who has already encountered the Mythos and keeps returning to incidents that sensible people would avoid. At the beginning of a run the investigator is exceptional but recognisably human—or only slightly supernatural. During the mission the investigator deliberately uses increasingly dangerous power, descends into Madness, accumulates physical Injuries, and may become the uniquely resonant focal character of the Elder One’s plan.

The repeated fantasy is: **read the crisis, coordinate a four-person expedition, become powerful enough to confront the impossible, and survive the consequences of doing so.**

## 1.3 Audience

- **Primary audience — LOCKED direction:** players who enjoy MOBA-style positional combat and hero mastery but want cooperative PvE rather than PvP competition.
- **Secondary audience:** co-op action/RPG players who enjoy objective pressure, raid-style bosses, roguelite run variation, and pulp/cosmic-horror themes.
- **Experience assumptions:** players can learn a compact ability kit, read telegraphs, prioritise targets, rotate between objectives, and coordinate through pings/text.
- **Accessibility assumption — LOCKED:** subjective Madness may make information unreliable, but accessibility settings must not make that information inaccessible.
- **Session context:** best with a friend group; public matchmaking is supported but is not the social design centre.

## 1.4 Platforms and session model

| Field | Decision |
|---|---|
| Engine | **LOCKED:** Unreal Engine 5.x. Exact project version to be pinned during prototype/pre-production. |
| Primary platform | **LOCKED:** PC-first. |
| Input | **LOCKED:** mouse/keyboard reference implementation plus full controller parity from the start. |
| Online model | **LOCKED:** listen-server authoritative multiplayer; matchmaking selects a host; host migration supported. |
| Party | **LOCKED:** always four investigators. 1–4 humans; empty slots filled immediately by AI companions. |
| Public play | **LOCKED:** matchmaking, pings, text, backfill, immediate AI takeover on disconnect. No built-in voice chat. |
| Private play | **LOCKED:** premade-first; lightweight scenario briefing before launch. |
| Typical successful run | **PROVISIONAL planning target:** about 30 minutes; exact duration remains tuning data. |
| Failed run shape | **LOCKED:** troubled runs should tend to TPK earlier, not stretch beyond a normal successful run. |
| Long-term structure | **LOCKED:** standalone scenarios with strong in-mission builds; meta progression is narrative/investigative rather than permanent combat power. |

## 1.5 Design pillars

Each pillar is a release-level constraint, not a slogan.

| Pillar | Meaning | Encourages | Rules out | Primary test |
|---|---|---|---|---|
| **MOBA mastery without PvP** | Traditional click-to-move combat, cooldown sequencing, threat, Break, target priority and team composition provide mastery without racing another player team. | Readable kits, positional play, threat management, interrupts, rotation decisions. | Twitch-shooter controls as the core; PvP balance as the design centre. | A four-player team should report MOBA-like combat decisions even with no opposing player team. |
| **Race the Ritual** | The opponent above individual encounters is an advancing ritual that changes the map and eventually forces Apocalypse. | Prioritisation, splitting, rotation, letting some problems fail deliberately. | Leisurely full-map clearing as the dominant strategy; generic hard mission timer as the only pressure. | Teams should regularly debate what they can afford to leave unresolved. |
| **Build from what you learn** | No pre-run loadout build. Hero evolution, Madness, boss clues and random relics emerge during play. | Adaptive evolution choices, pivoting, shared discussion. | Solved pre-mission builds or permanent vertical progression. | The same hero should end runs with materially different role emphasis for reasons discovered during those runs. |
| **Madness reveals truth** | Madness is both risk and information. One investigator has a Madness resonant with the current Elder One and becomes the run’s implicit focal character. | Subjective perception, learnable symptoms, boss clues, team trust. | Random control reversal, arbitrary friendly fire, purely cosmetic sanity. | Players should sometimes act on information only one investigator can perceive, without losing mechanical agency. |
| **Failure compounds** | Objectives fail forward, HTN repairs are harder, the Ritual advances, Injuries persist, and the run continues until TPK. | Recovery play, memorable consequences, difficult comeback attempts. | Escort-NPC death = instant mission failure; frequent hard resets. | Most non-TPK failures should worsen state rather than terminate the match. |
| **Four-person expedition** | Every normal mission assumes four active investigators; bots are teammates, not reduced difficulty substitutes. | 2+2 and 3+1 decisions, revives, capability coverage, coordinated boss mechanics. | Separate solo balance rules for every objective/boss mechanic. | 0-human/4-bot runs must exercise the same scenario logic as 4-human runs. |
| **Authored systems, variable episodes** | Handcrafted maps and bespoke critical fiction are combined through HTN planning, objective templates, faction configuration and boss-map contracts. | Replayability with causal coherence. | Pure procedural dungeon assembly; fixed identical critical path every run. | Repeated seeds should produce coherent episodes; varied seeds should produce meaningfully different but authored-feeling episodes. |
| **Pulp reality to cosmic nightmare** | Runs begin as heightened 1920s/30s pulp adventure and become increasingly surreal as the Ritual progresses. | Visual escalation, readable silhouettes, theatrical horror. | Photorealistic murk that compromises combat legibility; comedy-first camp. | Combat readability remains intact at maximum visual corruption. |

## 1.6 Non-goals

- **LOCKED:** not a PvP MOBA and not a simultaneous race against another team.
- **LOCKED:** not a hard tank/healer/DPS trinity; soft role tendencies and capabilities matter, but no specific role is required.
- **LOCKED:** not an economy/shop game. No universal gold currency or in-run item shop.
- **LOCKED:** not a persistent-power RPG. Meta progression does not grant permanent combat strength.
- **LOCKED:** not a voice-chat-dependent co-op game.
- **LOCKED:** not a full squad tactics/RTS game. AI companions are autonomous and influenced primarily through pings.
- **LOCKED:** not a full detective minigame. The meta-investigation automatically organises evidence and turns clues into trackable Leads.
- **LOCKED:** not a simulation where every Injury, faction, or world consequence permanently branches the campaign state.
- **NON-GOAL:** strict historical simulation. The period is 1920s/30s pulp, with authenticity used in service of readability and tone.
- **NON-GOAL:** copying protected expression from Evercore Heroes, Cthulhu: Death May Die, or other comparables. The project may learn from high-level structures but requires original expression, content, art, terminology where protectable, and a rights audit for Mythos material.

# 2. Core loops

## 2.1 Core gameplay loop

```text
Select scenario + investigators + optional mutators
        ↓
Receive spoiler-light briefing and personal Lead shortlist
        ↓
Server chooses hidden Elder One, valid Madness set, factions and HTN mission plan
        ↓
Enter handcrafted open objective map
        ↓
Fight / rotate / split / revive / interact with objectives
        ↓
Gain XP → level → evolve Q/W/E and core hero systems
        ↓
Optional objectives may produce random systemic relic loot
        ↓
Time + failed/expired objectives advance discrete Ritual stages
        ↓
Ritual manifestations + enemy intrusions + resonant Madness narrow boss identity
        ↓
Complete compulsory chain + required disruption; choose whether to spend time on power-ups
        ↓
Force manifestation OR Ritual reaches Apocalypse state
        ↓
Map-wide Apocalypse + asymmetric Elder One confrontation
        ↓
Victory or TPK
        ↓
Case report → Lead completions → new investigation evidence
        ↓
Choose next run
```

### Run phases

| Phase | Target shape | Key decisions | Exit |
|---|---|---|---|
| Briefing | Short; no tactical spoilers | Hero preferences, scenario, mutators, which personal Leads matter. | Host launches. |
| Opening / compulsory chain | **PROVISIONAL:** ~4–5 min of a successful run. | How to establish the incident and gain a valid route to ritual disruption. | Critical chain opens wider objective phase. |
| Objective phase | **PROVISIONAL:** ~15 min. | Which disruption tasks are mandatory now; which power-ups are worth Ritual time; when to split/regroup. | Minimum endgame requirements met or Ritual escalation forces crisis. |
| Manifestation / Apocalypse transition | **PROVISIONAL:** ~3–4 min. | Commit now, recover, or take one last risk; respond to map transformation. | Boss encounter enters climactic structure. |
| Boss | **PROVISIONAL:** ~6–8 min. | Resolve boss-specific mechanics, protect/use resonant investigator, exploit Break/vulnerability cycles. | Elder One defeated or team wiped. |

### Failure and recovery rule

> **LOCKED:** Almost every failure makes the game harder rather than ending it. Team wipe is the only universal scenario failure state.

- Objective runs its course → authored consequence + Ritual progression; mission continues.
- Visible critical task becomes impossible → HTN performs fiction-first constrained repair; replacement is harder.
- Investigator reaches 0 Health → Downed, gains an Injury, can be revived.
- Repeated physical trauma → up to two mechanical Injuries plus Grievous Injuries that block treatment priority.
- Ritual completes → Apocalypse rather than automatic defeat.
- Lead condition completed before a TPK → meta-investigation progress remains completed unless the Lead explicitly requires victory.

## 2.2 Meta loop

```text
Play any scenario
   ↓
Fulfil one or more personal/team Lead conditions (possibly unintentionally)
   ↓
Run ends: Victory or TPK
   ↓
Case report records consequences and fulfilled Leads
   ↓
New journals / letters / photos / artefacts / case notes become available
   ↓
Archive automatically connects evidence into investigation threads
   ↓
Vague observation → hypothesis → explicit trackable Lead
   ↓
Many Leads remain available in parallel
   ↓
Choose a scenario/hero/mutator combination likely to pursue some of them
   ↓
Repeat
```

**LOCKED:** challenges are mechanically possible even before the player has uncovered the evidence that describes them. If a player accidentally completes a future condition, the game records it and can resolve the Lead immediately when that investigation thread later becomes available. Knowledge reveals possibilities; it does not make them start existing.

## 2.3 Resource flow

| Resource / state | Sources | Sinks / recovery | Persistence | Intent |
|---|---|---|---|---|
| Health | Enemy attacks, hazards, some hero costs. | Relatively generous healing, support abilities, scenario resources; revive returns roughly half Health. | Run only. | Immediate tactical survival. |
| Burst-damage window | Recent Health loss over a rolling window. | Naturally decays with time. | Run only. | Converts concentrated damage into lasting Injury risk. |
| Injuries | Cross burst-damage threshold; being Downed. | Scarce Injury treatment. Max two mechanical Injuries. | Run only. | Physical attrition that changes behaviour. |
| Grievous Injuries | Third and subsequent Injury events. | Must be treated before ordinary Injuries can be removed. | Run only. | Further physical attrition without debuff soup. |
| Current Madness | Ritual exposure, enemies, chosen power use, R, hero-specific interactions. | Temporary reduction; cannot fall below floor. | Run only. | Controllable risk/reward and active symptom intensity. |
| Madness floor | Ability evolution and selected irreversible effects. | Cannot be reduced during a normal run. | Run only. | Records irreversible descent during the episode. |
| Hero resource | Bespoke per hero; generated/spent by that hero’s kit. | Hero-specific. | Run only. | Class-like combat rhythm and identity. |
| XP / levels | Small amount from regular enemies; more from elites/encounters/discoveries; large chunks from objectives. | Level thresholds. | Run only. | MOBA-style growth where fighting is worthwhile but farming is not. |
| Relics | Random drops/rewards from optional objectives. | Need/Greed/Pass; deliberately low carrying capacity (exact cap is tuning/scope data). | Run only. | Rare systemic rule-breakers that force adaptation. |
| Ritual stage/progress | Time, failed/expired local objectives, authored events. | Disruption objectives can delay/interfere as specified; not a freely reversible meter. | Run only. | Global pressure and escalation. |
| Break / Resolve | CC and Break-focused attacks on elites/bosses. | Recovers after vulnerability/control window. | Encounter only. | Keeps control builds relevant without stun-locking bosses. |
| Threat | Damage/healing/control/hero abilities under standard threat rules. | Decay, explicit manipulation, substantial reduction after revive. | Encounter/combat context. | Predictable soft-tank play with telegraphed exceptions. |
| Investigation evidence / Leads | Run achievements and narrative unlocks. | Evidence opens further threads; no power purchase. | Persistent per player. | Meta-narrative progression and purposeful replay. |

# 3. Player model

## 3.1 Player verbs

| Verb ID | Name | Input | Preconditions | Core result | Failure / rejection | System |
|---|---|---|---|---|---|---|
| VERB-MOVE-001 | Move | Right-click / controller left stick | Hero mobile; destination reachable | Path toward destination / direct controller movement | Blocked destination resolves to nearest legal navigation solution; immobilised states reject | SYS-MOV-001 |
| VERB-ATK-001 | Basic Attack | Attack-move / target / mapped controller input | Valid target/range/cooldown | Hero-specific basic attack | Invalid target or control state rejects with feedback | SYS-COM-001 |
| VERB-ABL-001 | Cast Q/W/E | Ability keys/buttons + targeting | Ability learned, cooldown/resource/state valid | Execute hero ability through GAS | Clear rejection reason; no silent resource loss | SYS-HER-001 |
| VERB-ULT-001 | Use R | R/button + targeting | Hero-specific Madness and resource requirements | Execute Madness-dependent ultimate | If unavailable, UI states why | SYS-MAD-001 / SYS-HER-001 |
| VERB-INT-001 | Interact | Context interact | Valid objective/world interaction | Channel, carry, manipulate, deliver, activate, etc. | Interrupted/invalid interactions give explicit state | SYS-OBJ-001 |
| VERB-REV-001 | Revive | Interact on Downed ally | Ally revivable; player not hard-disabled | Channel/action restores ally to ~50% Health; ally keeps Injury; threat reduced | Interrupted by configured damage/control; progress policy per revive spec | SYS-INJ-001 |
| VERB-PING-001 | Ping | Tap contextual / hold radial | Valid map/world/subjective target | Broadcast intent or perception to humans and bots | Subjective targets remain marked as perceived, not objective truth | SYS-COMMS-001 |
| VERB-MAP-001 | Inspect map | Map key/button | Not blocked by modal state | View objectives, routes, known terrain, Ritual state | Fog still hides live state | SYS-VIS-001 |
| VERB-EVO-001 | Evolve ability | Level-up prompt | Evolution-choice level reached | Choose next valid node in ability lattice | Only valid successors shown | SYS-HER-001 |
| VERB-LOOT-001 | Need / Greed / Pass | Relic roll UI | Random relic available; slot rules apply | Resolve shared relic allocation | Need outranks Greed; Pass declines; ties random | SYS-REL-001 |

## 3.2 Controls

### Keyboard / mouse — LOCKED reference

- Traditional MOBA click-to-move.
- Cursor-targeted basic attacks and abilities.
- Free MOBA camera: edge-pan and/or drag-pan, snap-to-hero, optional follow, limited zoom.
- Q/W/E/R mapped conventionally; exact keys rebindable.
- Tap ping = contextual default; hold ping = compact radial.
- Map and objective panels are planning layers, not permanent HUD clutter.

### Controller — LOCKED parity

- Left stick direct movement.
- Right stick aiming/camera control according to combat context.
- Ability mappings on face/shoulder inputs with purpose-built target assistance.
- Equivalent access to pings, map, objectives, relic rolls, evolution choices and text/UI navigation.
- Parity means equivalent PvE capability, not identical input semantics.

### Input accessibility — LOCKED principle

- Full remapping where platform permits.
- Hold/toggle alternatives for repeated or sustained interactions where mechanically possible.
- No core mechanic requires voice chat.
- Subjective audio information must have an equivalent optional visual/text channel without becoming public information.

## 3.3 Camera and fog of war

**LOCKED:** free MOBA camera with fog of war. Known objectives show through fog so the team can plan rotations. Fog blocks live information, not strategic intent.

- **Default map model:** once terrain/routes are known, fog hides enemies, patrols, faction movement, temporary hazards, transient Ritual effects and some interactables.
- **Scenario exception:** selected maps may make terrain discovery itself part of the scenario (ruins, caves, dream spaces, shifting estates).
- **Vision sources:** investigators plus persistent world sources such as lamps, searchlights, lookout points, friendly NPCs, wards or machinery.
- **Ritual interaction:** vision sources may be disabled, corrupted or otherwise changed as escalation proceeds.
- **Subjective Madness:** a player can perceive entities through their own private visibility layer. Pinging them communicates “this investigator perceives something here.”

# 4. Systems index

## 4.1 Master systems table

| System ID | System | Purpose | Status | Key dependencies |
|---|---|---|---|---|
| SYS-MOV-001 | Movement / navigation | Traditional MOBA locomotion and rotation | LOCKED design | Nav, input, camera, networking |
| SYS-COM-001 | Combat | Readable PvE MOBA combat | LOCKED design | GAS, threat, Break, enemy AI |
| SYS-THR-001 | Threat | Predictable baseline enemy targeting | LOCKED design | Combat, AI, hero abilities |
| SYS-BRK-001 | Break / Resolve | Keep CC relevant versus elites/bosses | LOCKED design | Combat, abilities, boss |
| SYS-HER-001 | Hero kit / evolution | Bespoke class-like hero identity and adaptive run builds | LOCKED design | XP, GAS, Madness, relics |
| SYS-MAD-001 | Madness / resonance | Risk-reward descent, subjective information, boss relationship | LOCKED design | Hero evolution, boss, UI/audio, run generator |
| SYS-INJ-001 | Health / Injury / Downed | Physical attrition and recoverable combat failure | LOCKED design | Combat, healing, revive, enemy special actions |
| SYS-XP-001 | XP / levels | In-run MOBA growth | LOCKED design | Enemies, objectives, hero evolution |
| SYS-REL-001 | Relics | Rare random systemic build pivots | LOCKED design | Optional objectives, loot UI |
| SYS-RIT-001 | Ritual escalation | Global pressure and map transformation | LOCKED design | Objectives, director, boss, Madness |
| SYS-OBJ-001 | Objectives / HTN mission planner | Authored-but-variable scenario plans and fail-forward repair | LOCKED design | Scenario data, factions, map markup, Ritual |
| SYS-DIR-001 | Encounter director | Ritual-driven reinforcement and map destabilisation | LOCKED design | Enemy AI, spawn markup, factions, Ritual |
| SYS-FAC-001 | Factions | Scenario-native ecology and inter-faction relationships | LOCKED design | AI, scenario generation, director |
| SYS-BOS-001 | Elder Ones / Apocalypse | Hidden boss identity and asymmetric climactic encounters | LOCKED design | Madness, ritual, map contract, Break |
| SYS-VIS-001 | Fog / vision / perception | Strategic information control and subjective reality | LOCKED design | Camera, Madness, map, AI knowledge |
| SYS-AI-001 | Companion / enemy / team AI | Four-investigator viability with 0–4 AI companions | LOCKED design | Utility layer, StateTree/BT, perception, objectives |
| SYS-COMMS-001 | Pings / text | Voice-independent team coordination | LOCKED design | UI, AI, subjective perception |
| SYS-META-001 | Investigation / Leads | Persistent narrative progression through achievements | LOCKED design | Case report, evidence database, challenges |
| SYS-MUT-001 | Mutators / challenge runs | Rule-changing difficulty and investigation conditions | LOCKED design | Scenario generator, meta |
| SYS-MAT-001 | Matchmaking / backfill | Premade/public sessions with hero preference and state-preserving replacement | LOCKED design | Networking, AI, UI |
| SYS-NET-001 | Authoritative simulation / migration | Deterministic server authority on listen host | LOCKED design | RNG service, serialization, replication |
| SYS-QA-001 | Seeded simulation harness | 0-human reproducible run testing | LOCKED design | All authoritative systems |

## 4.2 Combat system — SYS-COM-001

**Purpose.** Deliver traditional MOBA combat against PvE encounters: positioning, target priority, cooldown sequencing, resource management, threat, control, interrupts and team composition should matter more than twitch aiming.

### Player-facing rules

- Basic attack + Passive + Q/W/E + R is the canonical hero kit.
- Cooldowns are universal; every hero also has a bespoke class-like resource system.
- Soft role tendencies exist, but encounters test capabilities rather than requiring a tank/healer/DPS trinity.
- Ordinary enemies take normal hard CC. Elites/bosses use Break/Resolve to prevent permanent control.
- Combat damage contributes to Health loss; sufficiently concentrated recent damage can inflict a random Injury.
- Normal target selection uses threat tables; telegraphed special abilities may override threat.

### Verification

- At least two materially different team compositions can clear the same representative encounter without mandatory role substitution.
- A control-heavy hero remains useful against the MVP bosses through Break/interrupt mechanics.
- Low-tier enemies remain visibly easier after hero growth even as overall encounter difficulty rises through composition.

## 4.3 Threat — SYS-THR-001

**LOCKED.** Standard MMO-like threat tables are the baseline. Damage, healing, control and explicit abilities may generate or manipulate threat. Vanguard-leaning heroes can manage threat strongly, but the encounter does not assume one permanent tank.

- Common enemies rarely break normal threat behaviour.
- Elites use occasional special target-selection rules.
- Bosses use target overrides frequently as authored mechanics.
- Examples of valid overrides: highest Madness, most Injured, objective carrier, furthest target, isolated target, interrupt source.
- Exceptional targeting must be telegraphed; invisible arbitrary aggro changes are invalid.
- Reviving substantially reduces the revived hero’s threat but does not set it to zero.

## 4.4 Break / Resolve — SYS-BRK-001

**LOCKED.** Ordinary enemies can be stunned/rooted/controlled normally. Elites and bosses have a visible Break/Resolve layer. Control effects and dedicated Break attacks deplete it; when broken, the enemy becomes vulnerable to a defined control/interrupt window, then recovers and gains temporary resistance as required by the encounter.

**PROVISIONAL:** bosses may also expose explicit interrupt windows independent of a full Break. Exact Break values belong in data, not this GDD.

## 4.5 Hero kits and evolution — SYS-HER-001

### Starting kit

- **LOCKED:** Basic attack, Passive, Q, W, E, R.
- **LOCKED:** no pre-mission ability/loadout customisation. Hero selection is the only meaningful build choice before deployment.
- **LOCKED:** every hero has a bespoke resource system; some may already use minor supernatural mechanics at mission start.

### Ability evolution lattice

```text
A (base ability)
├─ B
│  ├─ D   deeper B specialisation
│  └─ E   hybrid / recombination
└─ C
   ├─ E   same hybrid / recombination
   └─ F   deeper C specialisation
```

**LOCKED:** Q/W/E each evolve twice. The first choice establishes direction without overcommitting; the second choice either specialises or converges through shared node E. The player chooses which ability to evolve at each major evolution opportunity.

### Level cadence

- **PROVISIONAL planning direction:** frequent levels during a successful run; exact count remains tuning data.
- The run must provide enough major evolution opportunities to complete both Q/W/E evolution choices; exact level cadence remains tuning data.
- Other levels improve passive, basic attack, bespoke resource behaviour and/or modest core stats.
- R is primarily Madness-dependent rather than a normal level-only unlock.
- Regular enemies give small XP; objectives and significant encounters provide the dominant advancement.

### Role drift

**LOCKED:** evolution can substantially shift a hero’s emphasis while keeping the hero recognisable. Systemic relic interactions may occasionally enable a much more dramatic role transformation that evolution alone would not permit.

## 4.6 Madness and resonance - SYS-MAD-001

Madness is simultaneously a risk/reward system, an irreversible in-run floor, a contextual symptom system, a run-variation system, and a hidden information channel.

### Assignment

- Each investigator receives one run-specific Madness.
- Every Madness can be associated with one or more Elder Ones.
- Exactly one investigator in a run has a Madness that resonates with the Elder One actually present.
- The full launch association graph must prevent duplicate association sets from allowing easy negative deduction. The MVP does not need to prove the complete launch graph; it proves one meaningful resonant investigator and subjective information.
- For the current MVP content, Shub-Niggurath resonates with **Obsession** and Nyarlathotep with **Perception**.

### Current Madness and floor

- Current Madness can rise and fall during the run.
- The Madness floor only rises through irreversible run development such as deeper ability evolution and selected occult effects.
- Recovery reduces current Madness but never below the floor.
- Players can deliberately accelerate or restrain Madness. A complete run still trends toward greater instability.

### Manifestation model

- Thresholds unlock symptom pools; contextual conditions decide when symptoms actually manifest.
- The affected player receives the full subjective experience. Other players receive only externally observable consequences unless an effect explicitly becomes shared reality.
- Maximum Madness causes a temporary **Crisis** specific to the Madness. The player retains control throughout. After Crisis, current Madness falls substantially but not below the floor.

### MVP Madness families

**Perception:** the investigator sees an additional subjective layer containing entities, hazards, objects, or opportunities. At higher intensity the subjective layer can affect real combat state. During Crisis it becomes dominant but required shared-world information remains visible. With Nyarlathotep resonance it reveals the true avatar among false manifestations.

**Compulsion:** contextual enemies, corpses, places, ritual objects, or actions become urges. Indulging can ease Madness or grant a short benefit; resisting increases pressure. Crisis presents several simultaneous urges. Control is never removed from the player.

**Dissociation:** some abilities produce predictable delayed echoes. At higher Madness the echoes become more spatially demanding. During Crisis most Q/W/E actions echo, creating a powerful but self-generated future-action puzzle.

**Obsession:** one enemy, corpse, growth, objective element, or location becomes a fixation. Acting on it eases pressure or grants benefit; ignoring it increases Madness. Crisis creates one overwhelming fixation. With Shub-Niggurath resonance, genuine reproductive nodes can become privileged fixation targets.

### Resonant investigator

The Elder One interacts much more strongly with exactly one investigator's Madness, creating the feeling that one party member is the run's implicit focal character without making the other three spectators.

The pattern is:

**resonant investigator perceives/experiences privileged information -> team acts on it -> Elder One pressures that investigator in return.**

Examples in the MVP:

- Shub-Niggurath: Obsession helps identify true reproductive growths.
- Nyarlathotep: Perception identifies the true avatar; Crossing Paths can temporarily suppress that true sight.

### Ultimate relationship

R is strongly Madness-dependent. Each hero ultimate should use only a small number of major Madness consequences rather than stacking every possible effect. Common patterns include a large temporary Madness spike, stronger behaviour at high Madness, or a temporary altered state. Ultimates remain understandable combat tools rather than collections of unrelated occult effects.

## 4.7 Health, Injury, Downed and revival - SYS-INJ-001

### Health to Injury

- Health is tactical and comparatively recoverable.
- The server tracks Health loss across a rolling damage window. Sufficient damage within that window generates an Injury event.
- Specific Injuries are immediate, named, mechanically explicit, and behaviour-changing rather than primarily statistical penalties.
- The MVP catalogue covers distinct behavioural pressures such as pacing ability chains, avoiding repeated heavy impacts, changing basic-attack rhythm, respecting displacement, managing healing, and leaving persistent hazards quickly.
- Only a small number of specific mechanical Injuries can be active at once. Additional Injury events become **Grievous Injuries** instead of adding more debuffs.
- Grievous Injuries must be treated before specific Injuries can be removed.

### Downed

- At 0 Health the investigator becomes Downed rather than dying.
- Becoming Downed causes an Injury event.
- Allies revive through a vulnerable interaction/channel.
- A revived investigator returns with meaningful but incomplete Health and substantially reduced, not reset, threat.
- Specific enemies or scenario states can exploit Downed heroes: drawing power, using them as anchors, creating apparitions, attempting infestation, or dragging them.

### No permanent individual incapacitation

There is no normal individual Incapacitation state that forces a player to spectate the remainder of the run. A Downed investigator remains recoverable as long as the party can complete the revive.

Repeated physical trauma instead accumulates through Grievous Injury stacks. Those stacks increase the time required to revive the investigator on a deliberately nonlinear curve. Exact seconds are tuning data rather than locked balance values.

This preserves the universal failure rule: **only a total party wipe ends the run.**

### Treatment

- Health recovery does not remove Injuries.
- Medical Power-up objectives can open limited-use treatment sources.
- Each treatment clears one Grievous Injury first; if none remain, it can remove one specific Injury.
- Ordinary world sustain comes from authored food pickups that auto-collect for injured investigators and heal over time. Enemies do not drop generic Health pickups.

## 4.8 Relics - SYS-REL-001

Relics are scarce, random, run-specific **systemic rule-breakers**. They are never hero-specific loot.

- Investigators begin a run without relics and can carry only a small number.
- The low capacity deliberately incentivises **Need / Greed / Pass** decisions and passing on merely adequate items in case a better fit appears later.
- Need outranks Greed; ties are random.
- Relics should change decisions or rules, not provide filler percentage-stat bonuses.
- Their value emerges from interaction with the hero's kit, evolution path, Madness, Injury, scenario state, and party needs.

### Current MVP relic set

**Officer's Swagger - Threat.** The first meaningful hard-control or substantial Break interaction against an enemy pushes the holder sharply upward on that enemy's threat table. While the enemy focuses the holder, allies become more effective at applying Break.

**Cracked Saint's Medal - Break.** Contributing meaningfully to Breaking an elite or boss primes the holder's next damaging/control ability against that target; using it during the Break window strengthens the payoff and can slightly extend the vulnerability opportunity.

**Hangman's Knot - Crowd Control.** Applying hard CC to a target causes nearby enemies to receive a weaker secondary control effect such as Slow/Suppression. Secondary targets do not receive equivalent hard CC.

**Overheal Shield relic - Healing (final name open).** Healing above maximum Health becomes Shield. The Shield persists briefly and then decays over time.

**Soldier's Morphine Tin - Injury.** On acquisition, remove one current specific Injury where possible. Future Injury events become Grievous rather than additional specific Injuries. The holder is faster to revive and also revives others faster.

**Black Glass Rosary - Madness.** Crossing upward into a new Madness tier briefly strengthens the investigator's bespoke resource generation/effectiveness. It does not reduce Madness or suppress symptoms.

**Ferryman's Coin - Movement.** Significant movement leaves a short-lived wake. Allies crossing it move more effectively; enemies are slowed and become easier to affect with CC/Break.

**Saint's Work Gloves - Objective Interaction.** While directly carrying/channeling/operating an objective, the holder gains strong control/displacement resistance and ordinary damage does not interrupt the interaction. Explicit interrupt mechanics still work. Completing or releasing the interaction provides a brief nearby defensive benefit.

## 4.9 Ritual escalation - SYS-RIT-001

Every scenario contains an overarching Ritual that advances over time. Local objectives/crises can also advance it. The Ritual replaces Evercore-style opponent pressure: the team races an escalating world state rather than another group of players.

### Five-stage structure

The Ritual uses five discrete mechanical stages:

1. **Incipient** - scenario-native factions dominate; local crises are manageable; Mythos influence is mostly ambiguous.
2. **Stirring** - competing priorities emerge; several objectives can deteriorate; reinforcements and mechanical complications increase.
3. **Intrusion** - Mythos mechanics enter the ordinary objective game; elites and boss-linked intrusion enemies appear; multiple candidate Elder-One ecologies may be present to narrow possibilities without proving identity.
4. **Convergence** - severe objective pressure; Disruption objectives use harder authored variants; actual-boss intrusion increasingly dominates and false candidate intrusions stop being introduced.
5. **Apocalypse** - the Elder One manifests and boss rules supersede ordinary Ritual progression.

Exact timing inside/between stages is tuning data.

### Deliberate manifestation

Once the compulsory chain and required Disruptions are complete, the team can deliberately summon the Elder One. Doing so **immediately jumps the Ritual to Apocalypse**, regardless of the current pre-Apocalypse stage.

This creates a strategic choice: continue preparing and risk further escalation, or summon now.

### Premature Apocalypse

If the Ritual reaches Apocalypse naturally before required disruption is complete, the Elder One manifests anyway. Outstanding mandatory tasks convert into harder **Apocalypse versions** that must be completed while the boss and full Apocalypse pressure are active.

The run remains winnable. There is no hidden unwinnable terminal state and no automatic failure simply because manifestation occurred early.

## 4.10 Objectives and HTN planner - SYS-OBJ-001

### Default mission architecture

The compulsory objective is a chain, not a single task. Main objectives are built from reusable mechanical templates but receive bespoke authored layout, complications, and light narrative wrappers. Optional objectives are more reusable and contextualised around the generated run.

For the current swamp MVP, the compulsory chain has four functional stages:

1. understand the disturbance;
2. trace the ritual infrastructure;
3. expose the submerged ritual structure by draining the basin;
4. gain the means to force manifestation.

Each stage has multiple authored decompositions selected by the HTN planner. The current variants include Missing Fisherman / Impossible Catch; Bell Network / Waterworks; Main Pump / Emergency Sluices; Counter-Sigil / Ritual Components.

### Gameplay-first narrative rule

Objectives differ mechanically first. Narrative is a light layer delivered through flavour text, speech/thought bubbles, barks, and later investigation material. A player can ignore narrative prose and still understand every required action and win.

### Disruption objectives

The swamp MVP uses three mechanically distinct Disruptions, all required before deliberate summoning becomes available:

- **Break the Bell Sequence** - memory/sequence interaction under combat pressure. Difficulty escalates through longer execution, tighter safe pauses, heavier interruption, and later multi-site coordination. It does not use fake tones.
- **Protect the Counter-Ritualist** - escort and moving defence. Later stages require work across multiple sites and short defensive warding stops.
- **Shatter the Marsh Idols** - distributed static targets. At later Ritual stages surviving idols buff nearby enemies.

If Apocalypse arrives first, each retains its identity as a harder Apocalypse objective rather than being replaced by a generic fallback task.

### Power-up objectives

Optional Power-ups include three reward families:

- **Relic:** Smuggler Cache (combat clearance/territory capture) and Lost Curio (recover/carry/deliver).
- **Medical:** Open the Surgery (clear/defend) and Rescue the Doctor (short escort/moving defence), producing limited shared Injury-treatment capacity.
- **Vision:** restore the isolated lighthouse for persistent vision over most of the map.

Relic and medical opportunities can recur. The scenario should present enough optional opportunities that clearing all of them is normally strategically unattractive under Ritual pressure.

### HTN planning and repair

- The scenario defines authored compound tasks, primitive tasks, methods, constraints, locations, prerequisites, and repair branches.
- The HTN chooses a coherent initial critical plan at run generation.
- Unrevealed future tasks may be silently repaired if invalidated.
- If a visible objective becomes impossible, repair is presented fiction-first and mechanically explicit.
- Repair is normally worse: more travel, harder combat, added Ritual progress, lost reward, worse position, or additional Injury/Madness exposure.
- The planner may revise the future, never established past facts or the player's current understanding.

### Objective urgency

Time-sensitive objectives use broad deterioration states leading to a final explicit **Imminent** countdown. The final warning is tuned around rotation feasibility: an immediate response from a reasonably positioned team should usually be possible, while poor positioning or hesitation can make failure unavoidable.

## 4.11 Enemy ecology, factions and director - SYS-FAC-001 / SYS-DIR-001

Maps begin with authored/persistent occupation, patrols, objective defenders, and faction territories. The encounter director then reinforces and destabilises that ecology using valid tagged entrances and emergence points.

Difficulty escalation is primarily **composition and mechanics**, not blanket enemy stat scaling. Low-tier enemies remain low-tier so in-run power growth remains visible.

### Swamp MVP factions

The current scenario uses three native factions. Primary/secondary status changes prevalence, territory, patrol density, and encounter weighting rather than unlocking special units.

**Local Cult - coordination, protection, channels, interrupts.**
- Zealot: basic melee pressure; becomes recklessly aggressive at low Health.
- Acolyte: ranged support with an interruptible buff/heal channel.
- Ritualist: weak direct fighter whose dangerous channels affect areas/objectives/Ritual pressure.
- Enforcer: durable protector with interception/bodyguard behaviour and knockback.
- Cult Leader (elite): coordinates nearby cultists; interrupting/Breaking the Leader cancels or weakens the command.

**Smugglers / Bootleggers - ranged formation, focus fire, area denial.**
- Gunman: becomes more effective if allowed to hold a firing position.
- Bruiser: close-range bodyguard who pushes divers away from the gunline.
- Lookout: marks an investigator so nearby smugglers preferentially focus and hit them.
- Bomber: telegraphed area-denial explosives.
- Gang Boss (elite): mobile ranged coordinator who calls Focus Fire, temporarily overriding normal threat for nearby smugglers.

**Swamp Things - ambush, movement disruption, isolation pressure.**
- Crawler: fast melee pressure, especially against isolated investigators.
- Lurker: reed/swamp ambusher; targetable when committing; opening attack slows rather than delivering huge unavoidable burst.
- Spitter: persistent terrain denial.
- Grasper: telegraphed pull that can be interrupted/countered through Break/CC.
- Old Thing (elite): territorial displacement pressure against separated investigators.

### Faction relationships

- Cult <-> Smugglers: neutral/transactional.
- Cult <-> Swamp Things: hostile but exploitative.
- Smugglers <-> Swamp Things: hostile.

Hostile factions genuinely fight one another and players can exploit that tactically. Ritual effects may temporarily alter relationships.

### Intrusion clue curve

- Early stages: no boss-linked intrusion enemies.
- Intrusion stage: enemies associated with more than one candidate Elder One can appear, so they narrow possibilities without proving identity.
- Convergence: new false-candidate intrusion stops; the actual Elder One's ecology becomes substantially more common.
- Apocalypse: actual-boss intrusion can dominate relevant encounters.

## 4.12 Elder Ones and Apocalypse - SYS-BOS-001

### Hidden identity

The Elder One is selected secretly at run generation. Any launch Elder One must be compatible with any launch scenario through the Boss-Map semantic contract. Identification emerges passively from Ritual manifestations, boss-linked intrusion enemies, and resonant Madness behaviour; there is no separate deduction minigame.

### Shared encounter rule

A common phased/vulnerability structure is a useful default, but Elder Ones are explicitly allowed to break it. At least one MVP boss does so to prove that the architecture is a capability contract rather than one boss script with different data.

### Shub-Niggurath

Shub is the more conventional MVP Elder One. Her identity is **permanent space corruption + multiplicative add pressure + meaningful reproductive nodes**.

- Resonant Madness: **Obsession**. The resonant investigator can identify genuine reproductive nodes among irrelevant/false growths.
- Corruption begins in the boss basin, at unfinished Disruption locations, and along Shub's movement. Existing corruption does not recede during Apocalypse; destroying a source only stops further spread.
- Shub-linked enemies gain damage resistance while standing in corruption, giving displacement/control direct tactical value.
- Core attacks include Trampling Advance (movement/charge leaving corruption), Black Milk (corruption placement around investigators), Call the Brood (corpse/Broodling pressure), and Horned Sweep (broad displacement/Break pressure).
- The fight progresses from rooted reproductive pressure through mobile manifestation into a final birthing frenzy. The arena becomes worse because of accumulated history rather than only numerical escalation.

**Broodling:** feeds on persistent corpse gameplay state. Corpse value fills a replication meter. Broodling corpses are worth only a small fraction of a standard corpse. Feeding and splitting are interruptible. A successful split replaces the parent with two partially injured Broodlings that begin healing to full; any damage interrupts that healing.

**Spawn of the Black Goat:** a charge bruiser that breaks formations and punishes narrow routes/clumping. Break provides counterplay to committed charges.

### Nyarlathotep

Nyarlathotep is the deliberate mould-breaker. His identity is **distributed deception + subjective identification + dangerous false manifestations**.

- Resonant Madness: **Perception**. Only the resonant investigator receives reliable true-sight information identifying the genuine avatar.
- The main cycle presents several simultaneous manifestations, one true and the rest false. False avatars deal full real damage and vanish after taking a meaningful but modest amount of damage, so manual testing is possible but inefficient.
- Destroying the true avatar advances the encounter. After enough successful cycles, Nyarlathotep enters a short direct execution phase with no new puzzle.

Core avatar abilities:

**Borrowed Face:** one manifestation becomes an exact presentation copy of an investigator, reusing existing model/animation presentation rather than requiring bespoke corrupted hero kits.

**Unwelcome Attention:** all avatars temporarily ignore normal threat and focus one marked investigator; the Perception-resonant investigator is more likely, but not guaranteed, to be selected.

**Crossing Paths:** avatars dash along telegraphed damaging lines and can cross/exchange relative positions. After the move, the resonant investigator's true-sight distinction is temporarily suppressed, forcing visual tracking or manual testing until it returns.

**Black Tongue:** applies a Madness spike and immediately triggers one valid symptom from the target's existing Madness rather than applying a generic separate sanity debuff.

Final direct form retains Black Tongue and adds a direct line/dash attack and a broad close-range tendril sweep. Normal threat largely resumes. The payoff is simply: the party has stripped away the deception and can finally kill the true presence.

### False Man intrusion enemy

False Man secretly replaces an ordinary faction unit. Before reveal it retains that unit's apparent identity/name. The only intentional pre-reveal tell is a subtle unnatural glow beneath it.

It has medium-ranged tendril melee. When sufficiently wounded, it becomes temporarily invulnerable, transforms and renames, heals, and telegraphs a radius attack. The attack resolves as transformation finishes and invulnerability then ends.

### Winged Hunter intrusion enemy

A small dragon-like flying hit-and-run attacker. It prefers exposed/isolated targets, telegraphs a dive/strafing pass, attacks and disengages. A melee hit during the correct committed window causes it to crash-land, creating a clear ground-control/burst opportunity.

## 4.13 AI — SYS-AI-001

### Companion philosophy

**LOCKED:** AI companions are autonomous teammates, not player-controlled squad units. The game always fills to four investigators, including 0-human/4-bot testing. Human players steer priorities through the same pings used with other humans.

### Knowledge rules

- Bots operate on character-appropriate perception, not omniscient game state.
- A bot with a subjective Madness can act on entities only it perceives.
- Other bots do not magically see those entities; they can react to pings, communicated intent or shared consequences.
- AI may know authored rules that a competent player would know, but must not read hidden boss identity or planner future state that its character cannot know.

### Strategic coordination

- A team-level utility/planning layer evaluates objective urgency, travel time, required strength, current locations, success probability, opportunity cost, revive risk and Ritual consequence.
- It allocates investigators dynamically into temporary 4, 3+1 or 2+2 groupings as the situation demands.
- Splitting is caused by strategic pressure, not by universal solo bonuses.

### Build decisions

**LOCKED:** bots score evolution and relic decisions adaptively from current state. The evaluation inputs are shared conceptually, but weights differ per hero so each class values capabilities according to its kit rather than using one global build script.

### Testing instrumentation

- Four-bot matches are a first-class test mode.
- Runs use deterministic seeds on the authoritative server.
- Meaningful AI decisions emit compact traces: team assignments, build choices, revive priorities, retreat/regroup, relic Need/Greed, major target switches.
- Outlier seed → replay exact authoritative scenario → inspect decision traces/overlays.

## 4.14 Matchmaking / backfill — SYS-MAT-001

- Unique heroes per team.
- Players rank hero preferences; matchmaking/lobby resolves conflicts rather than first-come instant-locking.
- Disconnect → immediate AI takeover with all hero/run state preserved.
- Backfill → joining player takes over the existing investigator exactly as-is, including level, evolutions, resource state, Madness, Injuries, relics and resonant status.
- Handover grants temporary invulnerability for a few seconds or until first gameplay command, whichever occurs first.
- During the handover window the joining player sees a compact takeover summary: objectives, evolutions, resource state, Madness/revealed symptoms, Injuries, relics, Ritual stage and known resonant behaviour.

## 4.15 Networking / authority — SYS-NET-001

- **LOCKED:** authoritative listen server. Matchmaking chooses host. Host migration is required.
- **LOCKED:** server simulation should be deterministic from the run seed; clients do not need deterministic presentation simulation.
- Server owns RNG, combat outcomes, AI decisions, threat, objectives/HTN, Ritual, Madness/Injury state, director actions, boss logic and relic rolls.
- Clients may use interpolation, animation, cosmetic physics and prediction as needed; they do not own authoritative outcomes.
- Host migration should favour correctness over seamlessness: a short pause/reconnect is acceptable.
- Migration requires resumable authoritative state, including RNG stream positions and planner/director state.

# 5. Progression and economy

## 5.1 Progression structure

| Layer | Rule |
|---|---|
| Short-term progression | XP/levels, Q/W/E evolution lattice, passive/basic/resource improvements, Madness floor/current state, R development, 0–2 relics, Injury accumulation. |
| Long-term power | **LOCKED:** none. No permanent numeric combat growth. |
| Long-term narrative | Per-player diegetic investigation archive; evidence reveals Leads; achievement conditions unlock further meta-narrative. |
| Difficulty progression | Mutators and authored challenge runs, including investigation-specified combinations. |
| Hero mastery | Player knowledge, not locked power. Full core hero mechanics are available without mastery stat upgrades. |
| Reset | Run combat state resets between scenarios. Narrative investigation persists. |
| Seasons / prestige | Not part of launch model. |

## 5.2 Economy

**LOCKED:** there is no universal persistent or in-run currency economy and no shop loop. Scenarios may contain bespoke resources (ritual components, ammunition crates, dynamite, keys, favours, machinery state, etc.) where needed, but these are scenario mechanics rather than a universal economy.

## 5.3 Investigation unlocks and gates

Meta progression is a diegetic investigation puzzle rather than a power tree. Evidence such as journals, letters, photographs, artefacts, clippings, reports, and field notes reveals narrative connections and actionable challenges.

### Evidence model

The intended flow is:

**Evidence -> Observation -> Hypothesis -> Lead -> Revelation**

- Evidence is automatically organised; players do not manually connect obvious corkboard nodes.
- A hypothesis begins as interpretation, then becomes an explicit **Lead** once the gameplay condition is sufficiently understood.
- Leads reveal possibilities that already exist. If the player satisfied a condition before learning the Lead, the game can recognise it retroactively.
- Lead completion unlocks further diegetic narrative material, not permanent combat power.

### Parallel progression

Many Leads should be available in parallel so random boss/objective generation rarely creates a run with no possible meta progress.

Lead anchors include:

- Elder One;
- scenario;
- investigator.

Leads can cross-reference mutators, objective states, Madness, Injury, or unusual run conditions.

### Difficulty bands

- **Discovery:** likely to occur through attentive normal play and introduces a thread/mechanic.
- **Directed:** requires deliberate setup or a changed approach, such as a mutator or late objective state.
- **Mastery:** tests strong understanding/execution and belongs mainly to boss/scenario threads rather than forcing extreme hero-specific feats.

### Current MVP examples

**Shub Discovery - Find the True Growth:** destroy a genuine reproductive node identified through the Obsession-resonant investigator.

Further proposed Shub leads test preventing/controlling Broodling reproduction and succeeding despite severe corruption.

Nyarlathotep leads test identifying the genuine avatar, tracking it through Crossing Paths when true sight is suppressed, and solving the deception loop efficiently.

Scenario leads test exposing the pre-human structure, completing late Bell Sequence variants, and winning after a premature Apocalypse forces outstanding Disruptions into their Apocalypse forms.

Investigator leads focus on each hero's defining systems (prepared battlefield, Exposure/type knowledge, Spirit Attention/Open Seance, threat/melee engagement) at Discovery/Directed difficulty.

### Credit and failure

- Team conditions give credit to every eligible player who has the Lead; no last-hit or final-interaction ownership.
- Personal conditions explicitly require that player's investigator/state/action.
- Completed Lead knowledge survives a later TPK unless victory is itself part of the condition.
- During a run, completion receives a small notification only. Narrative evidence is presented after the run.

### Pre-run briefing

Each player sees a personal shortlist of relevant Leads based on their investigation state, chosen scenario, investigator, and selected mutators. The shortlist is advisory; other active Leads can still progress.

## 5.4 Mutators and challenge runs

- Unlocked mutators can be freely selected for custom/challenge runs.
- Investigation Leads may require specific mutators or combinations.
- Mutators should change rules and pressures rather than primarily multiplying HP/damage.
- Examples of valid categories: faster Ritual, earlier elites, no Injury treatment, altered relic availability, higher Madness floor pressure, slower revive, early boss intrusions, starting Injury, altered Apocalypse timing.
- Normal hidden-seed runs count for progression. Manually replayed seeds do not normally count unless a Lead explicitly permits it.

# 6. World and level architecture

## 6.1 World structure

The world is presented through standalone 1920s/30s Mythos incidents. The investigators are a loose network of survivors rather than a formal uniformed agency. Any investigator can plausibly appear in any scenario.

### Current MVP scenario: rural swamp fishing village

The map is primarily **swamp containing a small village**, not a village map with marsh around its edges. The entire gameplay space remains on one top-down 2D navigation plane.

Major landmarks/regions include:

- a compact village centre;
- scattered fishing huts and jetties;
- an isolated church and graveyard;
- an isolated lighthouse;
- pumping/drainage infrastructure;
- a smuggler hideout/boathouse;
- several distinct swamp zones (deep marsh, reeds/flooded channels, mudflats/drained basin);
- distributed Marsh Idol/ritual clearings;
- the late-revealed pre-human ritual structure.

Traversal uses a strong hierarchy:

- **causeways/boardwalks:** fastest and safest but predictable and easy for factions to control;
- **shallow swamp/reeds:** slower, poorer visibility, greater ambush risk, but useful shortcuts;
- **deep water/mud:** normally impassable or prohibitively slow.

The major dynamic topology change happens once: pumping/drainage exposes the sunken pre-human structure **on the same gameplay plane**, unlocking the structured ritual basin used for the boss confrontation.

The isolated lighthouse is a pure optional Power-up landmark. Restoring it provides persistent vision over most of the map; it is not part of the compulsory chain or boss contract.

## 6.2 Scenario mission schema

| Field | Requirement |
|---|---|
| Stable ID | Permanent `SCN_*` key. |
| Target duration | Tunable successful-run target; troubled runs should tend to collapse earlier rather than create long losing tails. |
| Handcrafted map | Required. |
| Critical HTN domain | Abstract tasks, methods, constraints, repair branches, location requirements. |
| Compulsory chain | Linked critical tasks with authored alternative decompositions. |
| Disruption objectives | Scenario-authored required disruptions; the current swamp MVP uses Bell Sequence, Counter-Ritualist, and Marsh Idols. |
| Power-up objectives | Contextual optional Relic, Medical, Vision, and future scenario-specific opportunities. |
| Manifestation task | Enabled when the compulsory chain and required Disruptions are complete; deliberate use jumps directly to Apocalypse. |
| Factions | Scenario-native factions with primary/secondary prevalence and pair relationships. |
| Enemy ecology | Persistent occupation, patrols, defenders, spawn routes, and boss-linked intrusion. |
| Objective templates | Reusable mechanical verbs with bespoke critical-path dressing. |
| Ritual integration | Five-stage state machine, local crisis consequences, director tables, and Elder One overlay. |
| Boss-Map markup | Required semantic locations/affordances for every supported Elder One. |
| Fog model | Known-state default or scenario-specific exploration exception. |
| Case report outcomes | Flavour consequence categories plus any Lead hooks. |
| Acceptance | All supported Elder Ones can complete a valid encounter; HTN generation/repair never produces an impossible critical path. |

## 6.3 Default objective architecture

The shared architecture is functional rather than a fixed count template:

Compulsory chain  
↓  
Open objective phase  
├─ Required Disruptions (scenario-defined)  
├─ Optional Power-up opportunities  
├─ Local crises / deteriorating objective states  
└─ Continued compulsory HTN steps where required  
↓  
Compulsory chain + required Disruptions complete  
↓  
Choose whether to spend more time on Power-ups or **force manifestation now**  
↓  
Apocalypse / boss

If the Ritual reaches Apocalypse before the required Disruptions are complete, outstanding mandatory objectives convert into harder Apocalypse versions and remain completable under active boss pressure.

For the swamp MVP, the current authored Disruption set must all be resolved before deliberate manifestation. Other scenarios define their own required disruption structure and exact content counts; all must preserve the same fail-forward principle.

## 6.4 Objective template families

| Family | Use | Critical-path guidance |
|---|---|---|
| Manipulate / sequence | Operate machinery, align wards, decode/activate devices under pressure. | Good for bespoke critical objectives when the interaction changes combat positioning. |
| Carry / deliver | Carrier gives up some mobility/ability access or attracts threat. | Use to create escort-like pressure without fragile NPC dependency. |
| Multi-position / split | Simultaneous or alternating interactions at separated points. | Supports controlled splitting; ensure four-bot AI can allocate correctly. |
| Escort / moving defence | Move an NPC/object through space. | Failure should repair/branch, not usually hard-fail the run. |
| Defend / channel | Protect an interaction or object for a duration. | Valid only when positioning, interruption or changing threat makes the channel tactically interesting. |
| Hunt / eliminate | Find and kill mobile elite/leader. | Enemy should interact with map/faction state rather than be a static health bar. |
| Interrupt / contain | Prevent repeated enemy/ritual actions. | Good for Break/control validation. |
| Combat-only | Survive assault, break formation, eliminate required set. | Allowed; should not dominate catalogue. |

## 6.5 Ritual stages

The Ritual has five universal mechanical stages for the current design. Exact timing is tunable.

| Stage | Shared mechanical function | Elder One overlay |
|---|---|---|
| Incipient | Native factions dominate; simpler objectives; manageable local crises. | Mostly ambiguous cosmetic/perceptual signs. |
| Stirring | Competing priorities; simultaneous deterioration; stronger reinforcements and objective complications. | First minor mechanical manifestations, still ambiguous. |
| Intrusion | Mythos enters objective/encounter rules; elites and intrusion enemies appear. | Mix actual and false candidate boss-linked enemies. |
| Convergence | Hard late objective variants, heavy director pressure, severe map changes. | New false candidates stop; actual-boss ecology and resonance become increasingly obvious. |
| Apocalypse | Boss active; outstanding mandatory tasks convert to Apocalypse versions. | Full Elder One encounter rules. |

Deliberate summoning immediately jumps from the current pre-Apocalypse stage to Apocalypse. If Apocalypse arrives naturally too early, required tasks remain completable in their harder Apocalypse forms.

## 6.6 Boss–Map semantic markup

**PROVISIONAL technical contract.** Each map should expose semantic actors/volumes/tags rather than hard-coded boss references. Bosses query these affordances to generate encounter behaviour. The exact vocabulary must be proven against both vertical-slice bosses before content scale-up.

| Markup class | Example use |
|---|---|
| Ritual anchor / vulnerability site | Boss shield source, banishment location, objective interaction. |
| Boss arena / confrontation zone | Final phase location or fallback arena. |
| Spawn / emergence gate | Cult reinforcements, Deep One emergence, spectral intrusion, etc. |
| Hazard zone | Flood, corruption, fire, darkness, impossible geometry. |
| Traversal route / shortcut | Boss chase, blocked path, alternate rotation. |
| High/low ground | Line-of-sight or environmental phase mechanics. |
| Destructible / interactable | Boss can corrupt, consume or weaponise it. |
| Civilian / protected zone | Boss pressure, flavour consequence or optional task. |
| Vision source | Disable/corrupt/invert sight during Ritual/Apocalypse. |

# 7. Narrative architecture

## 7.1 Premise, themes and tone

### Premise

**LOCKED:** in the 1920s/30s, a loose fraternity/network of people who have survived contact with the Mythos recognise the signs of new incidents and repeatedly involve themselves. They are not a uniformed government agency or giant secret bureaucracy. They are scholars, criminals, soldiers, mystics, labourers, doctors, socialites, explorers and other survivors who know enough to be dangerous.

### Tone

- Pulpy action-horror: investigators are larger-than-life and mechanically capable from minute one.
- Some investigators may already be mildly supernatural, but the run still has somewhere much stranger to go.
- The emotional/visual trajectory is pulp adventure → escalating cosmic wrongness → Apocalypse.
- Narrative is primarily emergent/systemic during replay, with brief authored beats for critical-path plot and important meta revelations.
- Avoid long mid-run exposition that stops four players repeatedly.

### Themes

- Knowledge is useful and dangerous.
- Power has a cost but is often necessary.
- The body and mind fail differently: Injury and Madness are parallel pressures.
- Trust matters when perception is subjective.
- Disaster is usually a state to play through, not a fail screen.
- Human competence can matter against cosmic horror without making the Mythos mundane.

### Canon / rights rule

**REQUIRED LEGAL WORK:** maintain a rights register for every named Mythos entity, text-derived element and derivative-game influence. Public-domain status varies by work, element and jurisdiction. Do not copy protected characters, visual designs, scenario text, terminology unique to derivative products, or other expressive content from comparables. The final setting, investigators, ritual fiction, encounters and art direction must be original even when using lawful public-domain Mythos material.

## 7.2 Story structure

- No fixed linear campaign is required to play scenarios.
- Each run generates an episode inside a stable scenario framework.
- Critical path plot receives brief authored beats; most run story is the combination of boss, Madness, objectives, faction state, Injuries, relics, failures and Apocalypse.
- Post-run consequence list is explicit but normally flavour-only. It becomes mechanically persistent only when a meta-investigation Lead specifically references that consequence.
- The meta-investigation is the long-form narrative spine and can unlock new evidence and narrative elements through achievement conditions.

## 7.3 Investigator schema

| Field | Rule |
|---|---|
| Archetype | Strong immediately readable pulp archetype. |
| Personal weirdness | One specific unusual history/supernatural complication that makes the character belong to this setting. |
| Starting power level | Pulp action hero; a minority can be overtly supernatural at baseline. |
| Kit | Basic + Passive + Q/W/E + R. |
| Resource | Unique class-like resource system. |
| Internal role profile | Primary + secondary soft-role tendencies. |
| Player-facing tags | Concrete capabilities: Frontline, Sustain, Burst, Control, Mobility, Break, Threat Control, Objective Utility, etc. |
| Evolution | Q/W/E lattice; meaningful role drift but identity remains clear. |
| R | Strong Madness dependency; one/two significant Madness consequences. |
| Meta arc | Personal investigation Leads, correspondence, memories, relationships and scenario-specific dialogue. No permanent combat power. |
| Relationships | Mostly narrative; occasional contextual in-run interactions/reactions, never persistent pair-stat bonuses. |
| AI | Hero-specific utility weights for builds and tactical priorities. |

## 7.4 Resonant “main character” rule

**LOCKED:** every run guarantees exactly one investigator whose Madness resonates with the current Elder One. This investigator receives the strongest boss/Madness interaction and a feeling of being the episode’s focal protagonist, but the game remains a four-person co-op. The encounter should make that investigator especially important while requiring the other three to protect, enable, interpret or capitalise on the interaction.

## 7.5 Dialogue

- Dialogue should be brief during combat and repeatable content.
- Critical plot beats may use authored dialogue, but replay friction must be low.
- Relationships can alter revive lines, Crisis reactions, objective comments and recognition of another investigator’s symptoms.
- Subjective voices may be heard only by one player. Subtitles must preserve that subjectivity rather than publishing it as objective truth.
- No voice chat requirement affects character VO design: critical teamwork information must also exist in HUD/pings.

## 7.6 Post-run case report

**LOCKED:** present a clear Victory / Failure result, then list consequences rather than a letter-grade score.

- Elder One faced.
- Main/power objectives completed, failed or expired.
- Major Ritual consequences.
- Notable Injuries, Crises and resonant events.
- Scenario flavour consequences: survivors lost, sites destroyed, artefacts recovered, districts changed, etc.
- Lead completions and new investigation material.
- Consequences are flavour-only unless a specific investigation rule references them.

# 8. Content architecture

## 8.1 Stable ID convention

Use permanent IDs and never silently reuse deprecated IDs. Cross-references use IDs, not display names.

| Content | Pattern | Example |
|---|---|---|
| Investigator | `INV_HERO_NNN` | `INV_HERO_001` |
| Ability | `ABL_<HERO>_NNN` | `ABL_H001_Q_001` |
| Evolution node | `EVO_<ABILITY>_<NODE>` | `EVO_H001_Q_E` |
| Madness | `MAD_NNN` | `MAD_007` |
| Elder One | `ELD_NNN` | `ELD_003` |
| Scenario | `SCN_NNN` | `SCN_002` |
| Faction | `FAC_NNN` | `FAC_005` |
| Enemy archetype | `ENY_<FAC>_NNN` | `ENY_F005_003` |
| Objective template | `OBJTPL_NNN` | `OBJTPL_014` |
| Scenario objective | `OBJ_<SCN>_NNN` | `OBJ_S002_006` |
| Relic | `REL_NNN` | `REL_021` |
| Mutator | `MUT_NNN` | `MUT_009` |
| Investigation Lead | `LEAD_NNN` | `LEAD_042` |

## 8.2 Current MVP and launch content direction

Quantities below are current planning scope rather than frozen balance/design numbers. Content may move as production evidence accumulates.

| Content type | Current MVP content | Launch direction |
|---|---|---|
| Investigators | Trench Raider/Sapper; Expedition Photographer; Stage Medium; Bare-Knuckle Smuggler | Broader soft-role roster using the same hero schema |
| Scenario | Rural fishing village in a swamp | Several full handcrafted scenario maps |
| Elder Ones | Shub-Niggurath; Nyarlathotep | Broader boss catalogue, every boss compatible with every map |
| Madness | Perception; Compulsion; Dissociation; Obsession | Expanded association graph sufficient for non-trivial hidden-boss deduction |
| Native factions | Local Cult; Smugglers/Bootleggers; Swamp Things | Three supported native factions per scenario; reuse allowed |
| Relics | Representative systemic set covering Threat, Break, CC, Healing, Injury, Madness, Movement, Objective Interaction | Expanded systemic catalogue; still no hero-specific relics |
| Mutators | Representative rules for Ritual speed, treatment restriction, Madness pressure, and director/map pressure | Expanded challenge library tied into investigation |
| Investigation Leads | Boss, scenario and investigator threads across Discovery/Directed/Mastery patterns | Broad parallel web with many actionable threads |

## 8.3 Investigator content schema

| Field | Required |
|---|---|
| ID / display name / status | Yes |
| Pulp archetype / personal weirdness | Yes |
| Internal role profile | Yes |
| Player-facing capability tags | Yes |
| Basic / Passive / Q/W/E/R refs | Yes |
| Bespoke resource spec | Yes |
| Q/W/E evolution lattices | Yes |
| R Madness consequence set | Yes |
| Threat / Break / sustain capability notes | Yes |
| AI utility weights | Yes |
| VO/relationship hooks | Yes |
| Personal Lead hooks | Yes |
| Animation/VFX/audio dependencies | Yes |
| Acceptance tests | Yes |

## 8.4 Madness content schema

| Field | Requirement |
|---|---|
| ID / display name | Stable. |
| Associated Elder Ones | One or more; used by run generator. |
| Association conflict key | Allows pairwise-disjoint validation. |
| Threshold bands | Unlock symptom pools. |
| Context triggers | Conditions that manifest symptoms. |
| Subjective world rules | What only affected player sees/hears. |
| Externally observable rules | What teammates can see. |
| Crisis state | Maximum-Madness behaviour; player keeps control. |
| Recovery behaviour | Current reduction only; floor unchanged. |
| Resonant overrides | Per associated Elder One interaction hooks. |
| Ping behaviour | How private perception is communicated. |
| Accessibility equivalents | Visual/audio/text alternatives preserving information ownership. |
| AI perception behaviour | How bot with this Madness reasons. |

## 8.5 Elder One content schema

| Field | Requirement |
|---|---|
| ID / legal status | Stable ID plus rights review. |
| Resonant Madness support | One or more associated Madnesses; generation must still select exactly one matching investigator. |
| Shared early manifestations | Ambiguous effects shared with other bosses. |
| Revealing manifestations | Later effects that narrow identity. |
| Enemy intrusion rules | Boss-linked enemies/mutations by Ritual stage. |
| Resonant-investigator rules | Subjective effects, targeting, vulnerability, and Downed/revival interactions. |
| Boss–Map requirements | Semantic tags/affordances consumed. |
| Apocalypse transformation | Map-wide rules. |
| Phase structure | Default 2–3 phases + vulnerability cycles, or explicit mould-breaking structure. |
| Break/interrupt rules | How control builds contribute. |
| AI requirements | Companion strategy and enemy behaviour. |
| Mutator hooks | Valid challenge changes. |
| Acceptance suite | Must operate on every supported scenario. |

## 8.6 Scenario content schema

| Field | Requirement |
|---|---|
| ID / location / period context | Stable. |
| Map topology | Open objective map; bespoke exception allowed. |
| Critical HTN domain | Required. |
| Repair branches | Required for invalidatable visible tasks. |
| Disruption objectives | Scenario-defined required set sufficient to create meaningful prioritisation and fail-forward pressure. |
| Power-up opportunities | Scenario-defined optional set sufficient to create meaningful opportunity cost; exact quantity is tuning/content dependent. |
| Three faction support slots | Required. |
| Faction pair relationships | Required. |
| Persistent enemy ecology | Required. |
| Director spawn/route markup | Required. |
| Ritual stage effects | Required. |
| Vision sources / fog exception | Required. |
| Boss–Map semantic markup | Required for all Elder Ones. |
| Case report consequence templates | Required. |
| Investigation hooks | Optional/high-value. |
| Performance budget | Required before content complete. |

## 8.7 Relic content schema

- ID, display/localisation, rarity/availability if used, shared systemic hooks, eligibility conditions, Need/Greed semantics, conflict rules, two-slot interaction, VFX/UI cue, AI valuation inputs, validation tests.
- No hero-specific relic eligibility or bespoke hero-only text.
- Avoid passive percentage filler unless it materially changes a shared system rule.

## 8.8 Investigation Lead schema

| Field | Requirement |
|---|---|
| ID / thread | Stable. |
| Evidence prerequisites | What allows the clue/hypothesis to be shown. |
| Fictional clue | In-world observation. |
| Explicit condition | Machine-verifiable challenge. |
| Scope | Team or personal. |
| Qualifying run types | Normal / mutator / seed replay allowance. |
| Victory required? | Explicit boolean. |
| Retroactive completion | Supported where condition already occurred before revelation. |
| Narrative unlock | Evidence or meta-narrative item. |
| Briefing relevance rules | When it can appear in a player’s shortlist. |
| Telemetry/QA hook | How completion is verified. |

# 9. UI / UX

## 9.1 Information architecture

| Screen / overlay | Primary goal |
|---|---|
| Main / scenario select | Choose scenario or enter investigation/custom challenge. |
| Premade / matchmaking lobby | Resolve unique hero preferences, party, host and readiness. |
| Scenario briefing | Show stable scenario information, factions, mutators and personal relevant Lead shortlist without spoilers. |
| HUD | Immediate combat state only. |
| Map / objective panel | Strategic planning: known routes/objectives, urgency, Ritual stage, fog state. |
| Evolution choice | Fast field choice among valid successor nodes. |
| Relic roll | Need / Greed / Pass and current 0–2 relic slots. |
| Downed / revive | Show revive state and special threats exploiting the Downed hero. |
| Backfill takeover | Compact inherited-build and run-state summary during invulnerability. |
| Case report | Victory/Failure + consequences + Lead progress. |
| Investigation archive | Evidence graph, hypotheses, explicit Leads, newly unlocked narrative. |
| Mutator / custom seed | Build challenge run; replay/share completed seed. |

## 9.2 HUD

**LOCKED:** layered HUD. Always-on display is limited to combat-critical information; strategic detail is one interaction away.

### Always visible

- Health.
- Hero bespoke resource.
- Basic/Passive status as required and Q/W/E/R cooldown/availability.
- Current Madness and key active symptom/Crisis state.
- Active mechanical Injuries and Grievous count/priority as needed.
- Teammate Health/Downed state and critical role-independent state.
- Current Ritual stage.
- Tracked/urgent objective state.
- Boss Break/Resolve when relevant.

### One interaction away

- Full map and objective list.
- Detailed faction state.
- Detailed Madness explanation.
- Relic detail.
- Investigation Leads.
- Extended threat explanation beyond immediate aggro cues.

## 9.3 Pings and text

- **LOCKED:** no built-in voice chat.
- Tap ping uses context to infer enemy/objective/ally/ground/relic intent.
- Hold ping opens a compact radial for Go Here, Defend, Retreat, Help, Focus, Ignore/Leave and subjective “I perceive something here” semantics.
- AI consumes the same ping grammar as humans.
- Subjective ping never converts private information into a confirmed shared target marker.
- Text chat supports public/private party coordination and moderation requirements appropriate to platform; exact moderation stack **OPEN**.

## 9.4 Briefing

- Show scenario/location, known local factions, broad mission premise, selected mutators, hero choices.
- Each player sees a personal relevant Lead shortlist; party members can discuss it manually.
- Do not reveal Elder One, Madness assignments, generated critical graph, exact faction dominance, relics or hidden Ritual manifestations.

# 10. Accessibility

## 10.1 Core rule

> **LOCKED:** Madness may make information unreliable; accessibility must not make information inaccessible.

## 10.2 Requirements

- Full control remapping where platform permits.
- Mouse/keyboard and controller parity.
- Hold/toggle/repeated-input alternatives where equivalent timing can be preserved.
- Text scaling and controller-focus navigation on every gameplay-critical screen.
- No colour-only distinction for enemy telegraphs, Madness entities, objective states or faction identifiers.
- Subjective audio cue can have optional directional visual/text equivalent visible only to the affected player.
- Subjective visual distortion must support reduced-distortion/static alternatives that preserve gameplay objects and ownership of information.
- Flashes, warps and camera effects require reduction/disable options where they are not the sole carrier of a mechanic.
- Subtitles identify speaker and preserve subjective provenance (e.g., perceived voice versus shared speech) without confirming truth.
- Motion reduction and camera comfort options must not remove telegraph information.
- Critical timer urgency must use multiple channels: UI state, sound, world cue where possible.
- No requirement to use voice chat or accurately hear spatial audio to coordinate.

## 10.3 Open accessibility work

- **OPEN:** exact timing-assist policy for objective Imminent windows and whether assist settings alter progression eligibility.
- **OPEN:** screen narration scope for investigation/archive and gameplay menus.
- **OPEN:** cognitive-load options for layered HUD and Madness symptom explanations.

# 11. Art direction

## 11.1 Visual thesis

**LOCKED:** start each run in a heightened but readable 1920s/30s pulp reality and let the Ritual progressively attack visual stability. The world becomes stranger; the combat language must remain clear.

- Strong hero and enemy silhouettes at normal combat zoom.
- Period architecture and props are theatrical/readable rather than committed to photorealistic clutter.
- Early Ritual: fog, rain, shadows, subtle growths, wrong details.
- Mid Ritual: intrusive colour/value shifts, impossible perspective cues, architecture deforming, celestial anomalies, stronger boss-linked motifs.
- Apocalypse: spectacular map corruption driven by Elder One rules.
- Subjective Madness can distort one player’s presentation more aggressively because it carries private gameplay information.
- Enemy telegraphs, objective affordances, ally readability and dangerous zones remain legible regardless of corruption stage.

## 11.2 Asset production constraints

- **PROVISIONAL:** use modular scenario kits with semantic gameplay markup separated from boss-specific visuals.
- Boss-specific corruption should layer onto shared map geometry where practical to support six-boss × four-map interoperability.
- Every boss effect requires a readability fallback for reduced distortion/flash settings.
- Exact triangle, material, texture and VFX budgets are **OPEN** until a representative slice performance capture exists.

# 12. Audio and music

## 12.1 Sonic pillars

- Combat-system audio is trustworthy and readable.
- World/perception audio may become subjective under Madness.
- Ritual stages should be audible as an escalating state even when the player is focused on combat.
- The resonant investigator can receive private boss-linked audio that becomes another clue to identity.

## 12.2 Music states

**PROVISIONAL:** scenario music should support at least opening, objective pressure, late Ritual, Apocalypse and boss climax states, with boss identity capable of injecting motifs late without giving itself away in the opening minutes.

## 12.3 Subjective audio

- **LOCKED:** Madness may create footsteps, voices, spatial cues and entity sounds that only one investigator hears.
- False/perceptual cues must follow learnable rules rather than random noise.
- Core combat cues such as ability availability, confirmed damage telegraphs and critical shared warnings remain reliable.
- Accessibility equivalents remain private to the affected player.

# 13. Technical and platform constraints

## 13.1 Engine architecture

| Area | Recommendation / status |
|---|---|
| Engine | **LOCKED:** Unreal Engine 5.x. Pin exact version after prototype risk spikes. |
| Core gameplay code | **PROVISIONAL:** C++ for deterministic authoritative rules, run generation, RNG, HTN, AI utility scoring, state serialization and performance-critical systems. |
| Content scripting | **PROVISIONAL:** Blueprints for scenario/boss composition, cosmetic sequencing and designer-authored content where deterministic authoritative state remains in C++/validated data. |
| Abilities | **PROVISIONAL:** Gameplay Ability System (GAS) for abilities, attributes/effects, tags, cooldowns and replicated state. |
| Input | **PROVISIONAL:** Enhanced Input for keyboard/mouse + controller mapping contexts. |
| UI | **PROVISIONAL:** CommonUI/UMG with explicit controller focus/navigation and scalable HUD layers. |
| Tags | **PROVISIONAL:** Gameplay Tags as the shared semantic vocabulary for abilities, objective affordances, factions, map markup and status interactions. |
| AI execution | **PROVISIONAL:** StateTree and/or Behavior Trees for local behaviour; EQS where spatial queries materially help. |
| Team AI | **LOCKED design / PROVISIONAL implementation:** separate team-level utility/planner above individual behaviour trees/state machines. |
| Mission planner | **PROVISIONAL:** custom deterministic HTN domain/planner in C++ with data-authored methods/operators and explicit repair costs. |
| Navigation | **PROVISIONAL:** Recast NavMesh + authored nav links/areas; semantic routes for objective travel-time estimation. |
| World streaming | **PROVISIONAL:** compact scenario maps may use level streaming/data layers; use World Partition only where map scale/content workflow justifies it. |

## 13.2 Deterministic authoritative server

**LOCKED:** the server must reproduce authoritative simulation from seed; clients need not reproduce presentation deterministically.

### Deterministic scope

- All designed randomness goes through named seeded RNG streams.
- Mission generation, Madness selection, factions, optional objectives, relic rolls, AI tie-breaks, director choices, combat RNG and boss variation are server-owned.
- Server fixed-step/update ordering must be defined for systems whose ordering changes outcomes.
- Visual particles, animation interpolation, camera, cosmetic physics and other non-authoritative client effects are outside deterministic requirements.

### Risk

Perfect deterministic replay in UE can be undermined by floating-point, physics, navigation/path variation and timing/order differences. The design requires deterministic **authoritative gameplay outcomes**, not necessarily bit-identical engine presentation. Prototype must prove the chosen deterministic boundary before content scale-up.

## 13.3 Listen server and host migration

- Matchmaking scores/selects host using connection quality and hardware/network suitability criteria (**algorithm OPEN**).
- Gameplay architecture must not depend on the host also being a local player.
- Host migration may pause the match and reconnect; seamless combat continuation is not required.
- Migration snapshot must include run seed, RNG stream positions, GameState/PlayerState equivalents, hero evolution/resource/Madness/Injury/relic state, threat tables, HTN state, objective states, Ritual state, director state, faction state, boss state and AI strategic assignments needed to resume correctly.
- **HIGH RISK:** host migration with deterministic AI-heavy listen-server simulation is a vertical-slice technical spike, not a late production feature.

## 13.4 Replication

- Server authoritative for damage, healing, threat, Break, CC, objectives, XP/evolution validity, Madness/Injury, loot, AI, boss and Ritual.
- Clients predict only where useful for responsiveness and reconcile to server truth.
- Fog/subjective perception replication must filter information so clients do not receive hidden entities/state they are not entitled to know if that creates cheat/debug risk.
- **PROVISIONAL:** use relevancy/Replication Graph strategies if entity counts justify them.

## 13.5 Save/progression architecture

- Persistent save: investigation evidence, unlocked Leads/narrative, mutators, scenario/content access, cosmetics if any, settings.
- No persistent hero power/equipment state.
- Run snapshot: authoritative resumable state for host migration/reconnect; not necessarily a general suspend-anywhere feature.
- Versioned schema and migration tests required before Alpha.

## 13.6 Performance targets

- **PROVISIONAL client target:** 60 FPS on target recommended PC at representative combat density. Exact minimum hardware and quality tiers are OPEN.
- **OPEN:** authoritative server tick/fixed simulation frequency. Must be chosen from combat responsiveness and deterministic cost measurements.
- 0-human headless runs must execute faster than real time where possible for balance simulation; exact throughput target is OPEN until representative slice exists.
- Enemy/VFX density should be capped by encounter readability before raw engine maximums.

# 14. Tools and content pipeline

## 14.1 Authoring principle

> source tool → schema validation → deterministic generation/compile step → Unreal import/runtime data → automated build verification → QA seed suite

## 14.2 Required pipelines

| Pipeline | Canonical source | Validation |
|---|---|---|
| Heroes / abilities / evolution | Structured data + UE assets/specs | All lattice successors valid; GAS tags/effects resolve; no missing AI weights. |
| Madness association graph | Structured table/data asset | MVP: exactly one current-boss resonant investigator. Later content integration: every boss can generate a four-investigator pairwise-disjoint association set with exactly one current-boss match. |
| Scenarios / HTN | Scenario domain data | Planner produces legal plan; repair branches terminate; no contradiction with revealed facts. |
| Boss–Map contract | Semantic map markup + Elder One requirements | Every supported boss finds required affordances on every supported map. |
| Factions / encounters | Faction and encounter catalogues | Relationship matrix valid; director only uses legal spawn routes/compositions. |
| Relics | Systemic relic catalogue | No hero-specific dependencies; two-slot conflicts validated; AI valuations defined. |
| Investigation Leads | Narrative/challenge database | Condition is machine-verifiable; team/personal scope explicit; retroactive rules defined. |
| Mutators | Config/data assets | Combination conflict rules; deterministic seed impact defined. |

## 14.3 Debug tooling

- Seed override and copy/share UI.
- Force Elder One / Madness set / faction configuration / HTN plan for developer builds.
- Ritual stage advance/rewind for non-shipping debug.
- Objective state and HTN repair inspector.
- Threat table overlay.
- Break/Resolve overlay.
- AI team-plan and utility score trace overlay.
- Per-investigator perception view to verify subjective state filtering.
- Host migration trigger button and snapshot inspector.
- Headless four-bot batch runner with outcome export.

## 14.4 Source control / build pipeline

- **OPEN:** final source-control choice and binary-asset workflow.
- CI must build server/headless and client configurations, run deterministic unit/integration tests, validate data schemas and execute selected seeded bot scenarios.
- Content validation failures should block integration where they can create impossible missions, invalid Madness assignments or broken boss-map contracts.

# 15. Localisation

- Source locale: **OPEN** (English assumed for initial authoring until confirmed).
- All player-facing strings use stable IDs; display names may change without changing content IDs.
- Investigation prose requires context metadata because clue wording can distinguish observation from confirmed fact.
- Subjective speech/subtitles must carry provenance metadata so localisation does not accidentally convert uncertain perception into objective narration.
- UI must tolerate text expansion at configured scaling; evolution/relic tooltips need concise rule language.
- Period terminology, accents and names require localisation sensitivity review; do not encode essential mechanics in untranslatable wordplay unless supported by alternate clues.
- Target locale list, VO localisation policy, RTL support and font coverage are **OPEN**.

# 16. Analytics and telemetry

Telemetry should exist to answer design/quality questions, not to create a live-service engagement economy. Consent/privacy requirements depend on platform and jurisdiction.

| Event | Design question | Trigger / parameters |
|---|---|---|
| `run_start` | Which scenario/hero/mutator combinations are attempted? | scenario, seed hash, party human/bot composition, heroes, mutators, host type. |
| `run_end` | Why do runs end and at what point? | victory/TPK, duration, Ritual stage, boss, objective states, Injury/Madness summary. |
| `objective_state` | Which objectives are ignored, fail, or trigger repairs? | objective ID/template, state transition, travel distance, repair branch. |
| `ritual_stage` | Where does pressure accelerate/collapse runs? | stage, elapsed time, causes of advancement. |
| `injury_gain` | Are burst thresholds generating fair attrition? | injury ID, damage source, recent damage, downed flag. |
| `madness_threshold` | Are players controlling Madness or being forced unpredictably? | Madness ID, floor/current, trigger class, resonant flag. |
| `evolution_choice` | Are evolution branches viable/adaptive? | hero, ability, node, current party capability summary, likely-boss confidence if tracked by AI only. |
| `relic_roll` | Do relics create meaningful competition/variation? | relic ID, Need/Greed/Pass results, slot occupancy. |
| `boss_phase` | Where do boss attempts fail? | boss, map, phase, vulnerability cycle, resonant state. |
| `ai_major_decision` | Why did a bot/team planner choose a surprising action? | seed, decision class, scored alternatives in debug/test environments. |
| `lead_complete` | Are investigation Leads achievable without excessive dud runs? | Lead ID, team/personal, scenario, victory required, revealed-before-complete flag. |
| `host_migration` | Is listen-server migration reliable? | old/new host, snapshot age, duration, outcome/errors. |

# 17. Live operations / post-launch model

## 17.1 Commercial model

**LOCKED:** premium boxed game with substantial paid expansions. No gameplay microtransactions, battle pass, hero grinding or pay-to-unlock power.

## 17.2 Expansion philosophy

- An expansion should add a coherent bundle of investigators, scenarios, Elder Ones, Madnesses, factions, relics and investigation threads.
- New Elder Ones must work on existing scenarios through the Boss–Map Contract.
- New Madnesses must integrate with the association graph without creating assignment dead-ends.
- New relics remain systemic and should create interactions with the whole hero roster.
- New investigation content can reuse existing scenarios by adding Leads/narrative relationships rather than requiring every unlock to be a new map.

## 17.3 Live-service non-goals

- No seasons required for progression.
- No daily retention economy required.
- No battle pass.
- No rotating power store.
- Daily/weekly shared seeds may exist later as optional community challenge content, not as the core monetisation loop.

# 18. QA and verification

## 18.1 Release-blocking quality classes

- Crash, hang or unrecoverable desync.
- Host migration corrupts authoritative state or changes hidden run identity incorrectly.
- Run seed cannot reproduce authoritative designed randomness within the documented deterministic boundary.
- HTN produces impossible critical path or contradictory visible repair.
- MVP blocker: resonance generation produces zero or more than one investigator associated with the current Elder One.
- Later-content blocker: the expanded Madness catalogue cannot produce a legal pairwise-disjoint four-investigator association set for every supported Elder One.
- Boss–Map Contract fails to provide a complete encounter on a supported scenario.
- Fog/subjective replication leaks hidden information to a player who should not perceive it.
- Backfill loses inherited Madness, resonant status, evolution, Injury, relic or resource state.
- Input loss or inaccessible required action on keyboard/mouse or controller.
- Critical combat information becomes unreadable under Ritual corruption or accessibility settings.
- Progression save corruption or incorrect Lead completion scope.

## 18.2 Representative acceptance criteria

### Madness assignment

```text
Given: a selected Elder One and a four-investigator run
When: the run generator selects Madness manifestations
Then: exactly one selected Madness association set contains the current Elder One
And: no two selected Madness association sets intersect
And: all four selected Madnesses are valid for their investigators
```

### Objective repair

```text
Given: a visible critical objective that becomes impossible
When: the HTN planner repairs the mission
Then: the replacement is fictionally explained to players
And: no previously established fact is contradicted
And: the replacement has a measurable added cost/difficulty versus the invalidated route
And: the mission remains completable
```

### Imminent objective warning

```text
Given: a team in a representative moderately distant location
When: a time-sensitive objective enters Imminent state
Then: an immediate optimal rotation can normally reach and contest it before resolution
But: a team on the far side of the map or committed to another major task is not guaranteed recovery
```

### Backfill

```text
Given: a bot currently controls a hero after a disconnect
When: a new player backfills that slot
Then: the joining client receives the exact current level, evolution, resource, Madness, Injury, relic and resonant state
And: the hero is temporarily invulnerable until the configured handover time expires or the player issues a gameplay command
And: the takeover summary is visible before normal control
```

### Deterministic test run

```text
Given: the same build, authoritative seed and deterministic configuration
When: a 0-human four-bot run is executed repeatedly
Then: all designed random selections and authoritative game-rule outcomes follow the deterministic contract
And: any allowed non-deterministic engine/presentation differences are documented and do not alter canonical result state
```

## 18.3 Playtest questions

- Do teams feel they are racing the Ritual rather than simply clearing a PvE map?
- Do players willingly abandon/allow some objectives to fail because opportunity cost is real?
- Can players infer an Elder One gradually without the identity becoming trivial too early?
- Does the resonant investigator feel like the episode’s focal character without making the other three secondary spectators?
- Do Madness symptoms feel surprising but learnable rather than arbitrary?
- Do Injuries change behaviour without creating debuff overload?
- Does the two-relic cap create meaningful Pass decisions?
- Do different hero evolution paths emerge in response to run information rather than fixed guides?
- Do failed HTN routes feel like authored consequences instead of procedural substitution?
- Do struggling runs collapse decisively rather than drag?
- Can four bots complete, fail and recover for understandable reasons without hidden-information cheating?

# 19. Production and milestone maturity

## 19.1 Milestone plan

| Stage | Required proof for this project |
|---|---|
| Prototype | Traditional MOBA movement/combat in UE5; GAS feasibility; basic threat/Break; deterministic RNG service; one simple objective; one basic AI hero. |
| Pre-production | Custom HTN domain/repair spike; Madness/private perception prototype; four-investigator bot team utility; listen-server state model; Boss–Map semantic markup prototype. |
| Vertical slice / MVP | Current production-intent swamp scenario, current four-investigator team, Shub-Niggurath and Nyarlathotep (one standard-ish, one mould-breaking), legal MVP resonance assignment, representative factions/objectives/relics, full Ritual→Apocalypse loop, 0-human simulation, backfill, host-migration proof, case report + investigation Lead loop, target art/audio/UI/accessibility. Exact content quantities beyond this representative set remain provisional. |
| Alpha | Expanded investigator, Elder One and scenario content; major systems end-to-end; content tools and validation stable. Exact catalogue targets are production planning values, not canonical design constants. |
| Content complete / Beta | Intended launch-scale Elder One/scenario/faction/investigation/mutator content; full accessibility, localisation and performance passes. Exact catalogue targets remain a production-plan decision. |
| Release candidate | Deterministic/host-migration regression, seed suites, save migration, platform compliance, networking soak, performance and content validation clean. |
| Launch | Premium boxed launch baseline traceable by content IDs/build/version. |

## 19.2 Vertical-slice scope guardrails

- Do not scale the launch hero roster before proving the current MVP investigator set and its distinct soft-role/capability profiles.
- Do not scale scenario production before both MVP Elder Ones work on the first map through the intended semantic contract.
- Do not scale the Madness catalogue until MVP resonance/subjective replication is proven and the later pairwise-disjoint assignment solver is validated.
- Do not defer four-bot play; it is a core design and QA dependency.
- Do not defer host migration to late production; prove resumable authoritative state early.
- Do not overproduce art before Ritual-stage readability and boss-map transformation workflow are validated.

# 20. Decisions and change control

## 20.1 Decision policy

- Locked decisions in this document are changed only through a decision record with rationale and affected systems/content.
- Provisional numeric values move to structured balance data once the relevant system is implemented.
- Content instances belong in catalogues; this GDD should retain schema and global rules rather than duplicate every hero/boss/objective.
- When a system rule changes, update its acceptance criteria, analytics interpretation and affected content validation in the same change.

## 20.2 Remaining open decisions

The earlier open gameplay questions around individual Incapacitation, premature Apocalypse, and Ritual-stage structure are now resolved. Remaining opens are deliberately production/tuning/content-scale questions rather than holes in the core loop.

| Open item | Why it matters | Resolve by |
|---|---|---|
| Exact Injury rolling window and thresholds | Determines how frequently behaviour-changing Injuries occur. | Combat tuning after representative encounters exist |
| Exact Madness thresholds/rates and floor progression | Determines risk/reward cadence without changing the four-Madness rules. | Hero/Madness playtests |
| Final launch Madness association graph | Must support non-trivial hidden-boss inference without assignment dead-ends. | Before broad Elder One content production |
| Exact objective/Ritual timing | Controls optional-objective opportunity cost and run pacing. | Full swamp scenario playtests |
| Final relic values and carrying/treatment tuning | Must keep relics meaningful without dominating hero identity. | Slice balance pass |
| Host selection algorithm and snapshot cadence | Networking reliability and migration loss window. | Technical spike/soak testing |
| Authoritative server fixed simulation frequency | Responsiveness, determinism, CPU cost. | Combat/network prototype |
| Minimum/recommended PC hardware and quality tiers | Performance and accessibility of target market. | Representative performance capture |
| Text moderation/service requirements | Required for public text matchmaking. | Platform/backend selection |
| Screen narration/timing-assist/cognitive-load policy | Accessibility completeness. | Accessibility user testing |
| Final source-control/binary-asset workflow | Team scaling and Unreal asset conflicts. | Production planning |
| Localisation/VO target locales and RTL/font coverage | Budget and content pipeline. | Production/publishing plan |
| Exact price and expansion cadence | Commercial planning; core model remains premium game + substantial expansions. | Publishing plan |

# Appendix A - MVP / Vertical Slice definition

The MVP is a representative vertical slice of the intended game architecture, not a miniature content dump. It must prove the risky relationships between systems at sufficient fidelity for external playtesting.

## A.1 Required playable content

Current slice composition:

- one complete handcrafted swamp scenario with the drainage transformation and structured boss basin;
- four mechanically complete investigators (Sapper, Photographer, Medium, Smuggler) with bespoke resources, full Q/W/E evolution lattices, Madness-dependent R, AI support, controller support, replication, and snapshot state;
- Shub-Niggurath and Nyarlathotep as two intentionally different Elder One architectures;
- Local Cult, Smugglers/Bootleggers, and Swamp Things with relationship rules and full representative encounter roles;
- Perception, Compulsion, Dissociation, and Obsession Madnesses with complete Crisis behaviour and boss resonance;
- representative systemic relics, mutators, Injuries, objective templates, and investigation Leads;
- all three swamp Disruption objectives plus the current HTN compulsory-chain variants and Power-up templates.

Numbers, timings, thresholds, Health values, exact spawn budgets, and final catalogue size remain tunable.

## A.2 Required end-to-end systems

- briefing -> seeded generation -> HTN critical path -> Disruption/Power-up play -> Ritual escalation -> boss inference -> deliberate/natural Apocalypse -> Elder One fight -> Victory/TPK -> consequence report -> investigation progress;
- four active investigators with humans automatically filled by AI;
- zero-human four-bot headless/test execution using production rules;
- deterministic authoritative server boundary;
- listen server, immediate AI takeover on disconnect, state-preserving backfill, and host-migration proof;
- subjective Madness/perception filtering plus equivalent accessibility channels;
- fog/vision, including lighthouse persistent vision;
- Need/Greed/Pass relic flow;
- gameplay-first objective communication independent of flavour prose.

## A.3 Slice exit criteria

The slice is ready for the next production phase when external testers can repeatedly complete or fail full runs and demonstrate that:

- traditional MOBA combat is engaging against PvE encounter compositions;
- the same swamp map supports both Elder Ones without duplicate boss-specific levels;
- Shub's spatial corruption/reproduction and Nyarlathotep's avatar deception feel structurally distinct;
- HTN mission variation and visible repair remain coherent;
- optional objectives create real opportunity cost under Ritual pressure;
- subjective Madness remains understandable and private without being inaccessible;
- AI can split/regroup, build, revive, handle objectives, and complete whole runs without hidden-information cheating;
- premature Apocalypse remains winnable but clearly worse;
- troubled runs tend to collapse through compounding state rather than simply becoming longer;
- host migration restores a representative active run correctly;
- players finish with concrete reasons to approach another run differently.

# Appendix B - Launch direction

The launch game expands the same architecture rather than changing it.

- A broader investigator roster preserving bespoke resources, classic MOBA kits, and full in-run evolution.
- Several full handcrafted scenarios, each supporting three native factions and the shared Boss-Map semantic contract.
- A larger Elder One catalogue in which every boss can appear on every scenario map.
- A sufficiently broad Madness association graph to make boss inference meaningful without duplicate-association giveaways.
- A larger systemic relic and mutator catalogue.
- A broad per-player investigation web with many parallel Leads.
- Always four active investigators, with human vacancies filled by AI.
- Premium boxed commercial model with substantial paid expansions; no persistent power grind, battle pass, or gameplay microtransaction economy.

# Appendix C — Principal risks

| Risk | Why severe | Mitigation / proof |
|---|---|---|
| Boss × map combinatorics | A full boss × map matrix can silently become a bespoke encounter matrix and explode production. | Boss–Map semantic contract; prove two structurally different bosses on one slice map before expansion. |
| Madness assignment constraints | Pairwise-disjoint association sets can become impossible as content grows. | Offline validator and generation solver; content schema rejects dead-end association graphs. |
| Subjective multiplayer information | Networking/UI/AI can accidentally leak private state or become confusing. | Perception ownership model, filtered replication, subjective ping semantics, accessibility equivalence tests. |
| Host migration | Listen-server migration of AI/planner/RNG-heavy state is technically difficult. | Early resumable-state spike; explicit pause/reconnect acceptable; automated migration tests. |
| Deterministic server in UE5 | Engine subsystems may not replay identically by default. | Define deterministic gameplay boundary; own RNG/order; avoid authoritative dependence on unstable physics; seed tests. |
| HTN coherence | Dynamic repair can feel procedural or contradict story. | Initial plan + constrained repair; only silent for unrevealed future; visible repairs fiction-first and harder. |
| Companion AI scope | Bots must fight, build, plan, split, interpret pings and respect private perception. | Team utility layer separated from local combat AI; 4-bot testing is core development workflow. |
| Information overload | Health, resource, Madness, Injuries, threat, objectives, Ritual, Break, fog and relics compete for attention. | Layered HUD; always-on combat information only; field usability tests. |
| Madness frustration | Random/subjective effects can feel arbitrary or accessibility-hostile. | Learnable contextual trigger rules; player agency preserved; equivalent accessibility channels; no random control theft. |
| Content scope | Deep bespoke heroes, a broad Elder One catalogue and multiple semantic maps are expensive even without PvP. | Federated schemas, MVP gates, systemic relics, reusable objective templates, and staged content expansion only after risky contracts are proven. |
| Legal Mythos scope | Not every derivative Mythos element is automatically safe to use. | Rights register and legal review; original expression; avoid derivative-game copying. |

# Appendix D — Grill decision register

This register preserves the historical outcome of the 110-question design grill. It is intentionally terse; the canonical implications are expanded in the relevant GDD sections. Numerical quantities recorded here are retained for audit history and are not automatically canonical where v0.3 now marks them as planning/tuning values. One question number was reused during the live grill, so it is recorded as 44a/44b below.

| Question | Topic | Locked outcome |
|---|---|---|
| 1 | Product structure | Standalone scenarios with strong in-mission builds; meta progression horizontal/narrative. |
| 2 | Scenario variability | Handcrafted map with variable objective graph. |
| 3 | Combat | Traditional MOBA: click-to-move, isometric/top-down, ability bar. |
| 4 | Roles | Soft roles; capabilities rather than mandatory trinity. |
| 5 | Party | Four-investigator canonical target; 1–4 humans with AI fill. |
| 6 | Map structure | Compact open objective maps. |
| 7 | In-run build | Hero-specific evolutions; optional objectives provide random relics; power progression descends into Madness. |
| 8 | Madness core | Power-for-instability plus irreversible floor; hidden manifestation random per run and learned through play. |
| 9 | Splitting | Controlled splitting under pressure; regroup for major encounters. |
| 10 | Pressure | Overarching Ritual + local objectives/crises that can advance it. |
| 11 | Ritual completion | Apocalypse phase rather than immediate defeat. |
| 12 | Boss compatibility | Any Elder One can appear on any map; reuse shared ritual escalation and map markup. |
| 13 | Boss location | Hybrid map-wide/bespoke use driven by boss type, ending in clear climax. |
| 14 | Hero fiction | Loose fraternity/network of Mythos survivors; 1920s/30s. |
| 15 | Baseline tone | Pulp action heroes; some slightly supernatural at start. |
| 16 | Hero kit | Basic + Passive + Q/W/E + R; R heavily Madness-dependent. |
| 17 | Ultimate/Madness | Using R pushes deeper; each R uses only 1–2 consequences; transformation can be one option. |
| 18 | Lethality | Health recoverable; Injury and Madness operate side by side; encounters can pressure either. |
| 19 | Injury trigger | Enough Health loss within a rolling time window creates a random Injury. |
| 20 | 0 Health | Downed + escalating consequences; certain enemies/scenario states exploit downed heroes. |
| 21 | Healing | Generous Health recovery; scarce Injury treatment; no mandatory healer. |
| 22 | XP | Mixed XP; regular enemies small amount, objectives/significant encounters much more. |
| 23 | Objective authorship | Bespoke main objectives built from templates; generic/reusable optionals. |
| 24 | Objective graph | Compulsory chain + 2–3 disruption + 2–3 power-up + manifestation. |
| 25 | Boss clues from objectives | Mostly passive revelation; keep deduction simple. |
| 26 | Enemy relation to boss | Scenario-native enemies fixed first; boss-linked intrusion later. |
| 27 | Enemy population | Persistent authored ecology + ritual-driven reinforcement director. |
| 28 | Split reward | Splitting is strategic only; no solo bonuses. |
| 29 | Threat | Traditional threat tables; special abilities override targeting, increasingly on elites/bosses. |
| 30 | CC | Break/Resolve system for elites/bosses; ordinary enemies take full CC. |
| 31 | Ability resources | Mixed initially; later amended so every hero has a bespoke class-like resource. |
| 32 | Pre-run customisation | None; all meaningful build customisation happens in game. All heroes have bespoke resources. |
| 33 | Evolution topology | A→B/C; B→D/E; C→E/F; E recombines branches. |
| 34 | Level pacing | Frequent levels; six major Q/W/E evolution choices among ~8–10 total. |
| 35 | Relics | Only systemic rule-breakers; nothing hero-specific. |
| 36 | Relic capacity | Start 0; finish 1–2 depending on optional objectives; hard cap 2. |
| 37 | Relic allocation | Shared random loot with Need/Greed/Pass; cap incentivises waiting for fit. |
| 38 | Universal failure | Team wipe only. |
| 39 | Meta progression | Diegetic investigation puzzle; journals/evidence guide faster narrative progression. |
| 40 | Meta unlock condition | Achievements/challenges unlock new meta narrative; many available in parallel. |
| 41 | Lead presentation | Clue first, then explicit condition; investigation informed by comparable research. |
| 42 | Resonant Madness guarantee | Exactly one investigator guaranteed to have Madness resonant with current Elder One. |
| 43 | Madness associations | All Madnesses resonate with one or more Elder Ones, not necessarily the one present. |
| 44a | Resonance uniqueness | Only one investigator has Madness associated with current boss; selected Madness association sets are pairwise disjoint. |
| 44b | Visibility of Madness | Affected player gets full subjective experience; others see observable consequences. |
| 45 | Madness communication | Contextual pings can indicate subjective targets without making them shared truth. |
| 46 | Manifest timing | Thresholds unlock symptoms; context triggers manifestations. |
| 47 | Madness agency | Strong risk/reward control; descent still trends upward. |
| 48 | Madness recovery | Reduce current Madness only, never below floor. |
| 49 | Maximum Madness | Temporary Crisis state; player retains agency; current Madness later drops toward floor. |
| 50 | Injury effects | Behaviour-changing conditions, not mostly numeric penalties. |
| 51 | Injury information | Immediate and explicit name/effect. |
| 52 | Injury cap | Two active mechanical Injuries; third+ become Grievous, no extra mechanic, treated first. |
| 53 | Revive health | Moderate-health revive (~50% target), Injury persists. |
| 54 | Revive threat | Threat drops substantially but not to zero. |
| 55 | Mission length | ~30 min successful run; correction: troubled runs should TPK earlier, not run longer. |
| 56 | Traversal | Light tactical traversal with patrols/hazards/shortcuts/route pressure. |
| 57 | Enemy scaling | Composition/mechanics first; modest numeric scaling. |
| 58 | Difficulty | Mutators and challenge runs rather than conventional stat tiers; can interact with investigation. |
| 59 | Mutator selection | Hybrid: freely chosen custom runs plus investigation-specified challenges. |
| 60 | Launch roster | 8 investigators; 4 in vertical slice. |
| 61 | Role distribution | Internal primary/secondary role tendencies; player-facing capability tags. |
| 62 | Role drift | Evolution allows meaningful drift; relics can occasionally produce more radical transformation. |
| 63 | Hero fiction | Strong pulp archetype + specific personal weirdness. |
| 64 | Hero continuity | Narrative threads persist; mechanics reset every run. |
| 65 | Hero relationships | Mostly narrative with occasional contextual in-run interactions; no pair bonuses. |
| 66 | Run narrative | Mostly emergent/systemic, with brief authored critical-path plot beats. |
| 67 | Outcome | Clear Victory/Failure screen + consequence list; usually flavour-only unless a Lead needs it. |
| 68 | Critical-path variance | B/D hybrid: authored narrative functions generated with an HTN planner. |
| 69 | Replanning | Initial plan + constrained repair; repairs escalate difficulty/cost. |
| 70 | Repair visibility | Fiction-first explicit repair only if the invalidated objective was already visible. |
| 71 | Optional objectives | Contextual layer around HTN critical plan rather than fully inside planner. |
| 72 | Boss count | 2 vertical slice, 4 alpha/content integration, 6 launch. |
| 73 | Scenario count | 4 launch; 1 deep vertical-slice scenario. |
| 74 | Factions per scenario | 3 factions that can become primary/secondary. |
| 75 | Faction relations | Authored allied/neutral/hostile relationships; hostile factions can fight. |
| 76 | Camera | Free MOBA camera + fog; objectives visible through fog. |
| 77 | Vision | Heroes + persistent world-based vision sources; no generic ward system. |
| 78 | Fog model | Hybrid: known terrain/live-state fog by default; scenario exceptions can hide terrain. |
| 79 | Objective interaction | Combat-integrated verbs default; some channel/hold and combat-only objectives allowed. |
| 80 | Objective timers | Staged escalation; final warning gives just enough time for immediate nearby/moderate response. |
| 81 | Low-human scaling | Always field four investigators; AI fills empty slots. |
| 82 | AI control | Autonomous bots influenced through pings, not squad micromanagement. |
| 83 | AI builds | Utility-based adaptive choices with hero-specific scoring weights. |
| 84 | AI strategy | Shared team-level utility/planning; support 4-bot matches for testing. |
| 85 | AI instrumentation | Compact decision traces + deterministic seeds. |
| 86 | Determinism | Authoritative server deterministic; client presentation need not be. |
| 87 | Network authority | Listen server; matchmaking selects host; host migration required. |
| 88 | Hero selection | Unique heroes; simultaneous preference ranking resolves conflicts. |
| 89 | Backfill | Take over existing hero; short invulnerability until command or timeout. |
| 90 | Takeover UX | Compact summary of inherited build/run state. |
| 91 | Disconnect | Immediate AI takeover. |
| 92 | Communication | Pings + text; no built-in voice. |
| 93 | Ping UX | Contextual tap + compact hold radial. |
| 94 | Social model | Premade-first; public matchmaking supported but not design centre. |
| 95 | Private briefing | Scenario briefing with stable info, mutators, hero choices; no hidden-run spoilers. |
| 96 | Lead briefing | Short personal relevant Lead list per player. |
| 97 | Shared run progression | Different players progress their own Leads independently in same run. |
| 98 | Lead credit | Team conditions grant party-wide credit to eligible players; personal conditions stay personal. |
| 99 | Failed-run progression | Completed Lead knowledge survives TPK unless victory is part of condition. |
| 100 | Lead reveal timing | Completion notice in run; narrative evidence revealed after run. |
| 101 | Visual arc | Pulp reality → escalating surrealism; maintain combat readability. |
| 102 | Boss structure | Default phases + objective-driven vulnerability cycles; bosses may wildly break mould; at least one slice boss should. |
| 103 | Ritual UI structure | Discrete Ritual stages. |
| 104 | HUD | Layered: combat-critical always visible; planning information one interaction away. |
| 105 | Economy | No universal currency or shop. |
| 106 | Control/platform | PC first with full controller parity. |
| 107 | Madness audio | Subjective world/perception audio; core combat-system audio remains reliable. |
| 108 | Accessibility | Equivalent information channels preserve subjectivity without withholding mechanics. |
| 109 | Run seeds | Hidden by default; completed-run seeds can be replayed/shared in custom mode. |
| 110 | Commercial model | Premium boxed game + substantial paid expansions. |


## D.1 Post-v0.1 resolved decisions

The following supersede older open/provisional entries in v0.1:

- No permanent individual Incapacitation; Grievous Injury stacks instead increase revive difficulty.
- Five Ritual stages: Incipient, Stirring, Intrusion, Convergence, Apocalypse.
- Deliberate summoning jumps immediately to Apocalypse.
- Premature natural Apocalypse converts unfinished required objectives into harder boss-active versions rather than creating defeat or an unwinnable state.
- Current MVP location/content is the rural swamp village, current four investigator concepts, Shub-Niggurath/Nyarlathotep, and the four Madness families detailed in Appendices E-J.
- Gameplay-first narrative: objective mechanics must remain fully understandable if flavour text, speech/thought bubbles, and archive prose are ignored.

# Appendix E - MVP content specification

This appendix records the reviewed MVP content decisions made after v0.1. It is authoritative for current content direction; exact tuning values remain provisional.

## E.1 Gameplay-first narrative rule

The game is gameplay first. Objectives are differentiated by mechanics. Narrative is a light interpretive layer delivered through flavour text, speech/thought bubbles, barks, environmental detail, and the investigation archive. Ignoring narrative must never prevent a player from understanding what to do or how to win.

## E.2 Swamp scenario content

The MVP location is a rural fishing village embedded in a large swamp. The village centre is small; fishing huts are scattered; the church and lighthouse are isolated; several marsh zones provide traversal texture. Boardwalks/causeways are fast/predictable, shallow swamp/reeds are slower/ambush-prone shortcuts, and deep water/mud structures the routes.

The ritual centre is a pre-human structure submerged in a basin. Pumping/drainage exposes it onto the same 2D gameplay plane. The exposed basin contains a central structure, outer ritual platforms/nodes, shallow channels/stone ribs, several entrances, and enough open/structured space for both MVP bosses.

The lighthouse is a pure optional Power-up: restoring it grants persistent vision over most of the map.

## E.3 Compulsory HTN chain

Functional stages and current authored alternatives:

| Function | Variant A | Variant B |
|---|---|---|
| Understand disturbance | Find Missing Fisherman | Examine Impossible Catch |
| Trace ritual infrastructure | Inspect Bell Network | Trace Waterworks |
| Expose ritual structure | Restore Main Pump | Open Emergency Sluices |
| Gain manifestation method | Charge Counter-Sigil | Assemble Ritual Components |

The HTN plans the initial route and can repair unrevealed future tasks silently. Visible failures receive explicit fiction-first repair and a harder mechanical route.

## E.4 Disruptions

**Break the Bell Sequence:** Simon-Says-like memory/sequence interaction under combat pressure. Later stages increase execution pressure and distribute bells across sites; no false tones.

**Protect the Counter-Ritualist:** escort/moving defence. Escalates from one site to multi-site routes and warding stops.

**Shatter the Marsh Idols:** static distributed targets. At later Ritual stages surviving idols buff nearby enemies.

All required Disruptions must be completed before deliberate summoning. If Apocalypse occurs first, unfinished Disruptions convert into harder Apocalypse versions.

## E.5 Power-up templates

- Smuggler Cache -> combat clearance / territory capture -> relic.
- Lost Curio -> recover / carry / deliver -> relic.
- Open the Surgery -> clear / defend -> limited treatment source.
- Rescue the Doctor -> short escort / moving defence -> limited treatment source.
- Lighthouse -> persistent vision over most of the map.

Food pickups provide ordinary Health sustain as Heal-over-Time. They appear at plausible authored locations with run-to-run activation variation and auto-collect when an injured investigator approaches.

# Appendix F - MVP investigator specification

## F.1 Trench Raider / Sapper

**Role:** Vanguard / Controller. **Core verb:** shape the battlefield. **Resource:** Prepared Charges replenished from components dropped by human enemies. **Basic:** semi-automatic carbine. **Passive:** Scrounger.

**Q Satchel Charge:** precision demolition branch (Shaped Blast -> Breaching Charge), area/displacement branch (Wide Charge -> Daisy Chain), convergence through Concussive Charge.

**W Suppressing Fire:** defensive coverage branch (Covering Fire -> Entrenched Position), stronger pin/Break branch (Pin Them Down -> Walking Fire), convergence through Kill Zone.

**E Tripwire:** two independently placed endpoints. Reinforced interception -> Clothesline; Detonator Wire -> Resetting Fuse; convergence through Prepared Ground.

**R Dead Ground:** traps that would legitimately trigger tag the triggering enemy instead of firing. After a short shared delay, tagged enemies simultaneously receive the effects of the traps that tagged them. Dormant traps with no valid trigger remain unused. R creates a strong Madness spike.

## F.2 Expedition Photographer

**Role:** Striker / Support. **Core verb:** identify and exploit priority targets. **Resource:** per-target Exposure. **Basic:** precision rifle whose effectiveness rises with Exposure. **Passive:** Perfect Moment, rewarding framing committed/telegraphed enemy actions.

**Q Frame the Subject:** single-target analysis (Close Study -> Tell-Tale Detail), area study (Wide Angle -> Group Portrait), convergence through Composition.

**W Flashbulb:** stronger disruption/Break (Blinding Flash -> Afterimage), stronger Exposure (Overexpose -> Magnesium Burst), convergence through Caught in the Light.

**E Develop:** single-target burst/vulnerability (Weak Point -> Fatal Detail), type-wide knowledge/debuff (Clear Evidence -> Case File), convergence through Published Findings. Clear Evidence/Case File apply their debuff to all enemies of the studied type.

**R Impossible Photograph:** temporarily exposes the visible battlefield, prevents Exposure decay, and allows repeated Develop use against stored Exposure; higher Madness can include normally hidden/subjective phenomena.

## F.3 Stage Medium

**Role:** Controller / Support. **Core verb:** manipulate states/positioning indirectly. **Resource:** per-spirit Attention. **Basic:** Spirit Lash, which builds Attention on a spirit bound to the struck enemy. **Passive:** Thin Places; deaths, Downed investigators, ritual sites, and strong Madness phenomena create better Attention opportunities.

**Q Bind Spirit:** protection (Guardian Spirit -> Vigil), hostile haunting (Haunting -> Possession), convergence through Restless Watcher.

**W Beckon:** movement/procession (Procession -> Funeral March), stronger arrival activation (Calling -> Seance), convergence through Crossroads.

**E Intercession:** protection (Guardian's Hand -> Not Yet), control/Break (Unquiet Dead -> Drag Below), convergence through Between Worlds.

**R Open Seance:** bound spirits fully manifest, intensify, and can Intercede without normal exhaustion for a short window; higher Madness can invite additional unbound spirits around deaths/Downed investigators.

## F.4 Bare-Knuckle Smuggler

**Role:** Striker / Vanguard. **Core verb:** stay engaged and snowball pressure. **Resource:** Momentum, a build-and-decay combat-state meter. **Basic:** bare-knuckle multi-hit combo. **Passive:** Keep Your Feet, improving stickiness and resistance to slows/displacement at higher Momentum.

**Q Clinch:** stronger hold/duelling branch (Lock Up -> Make It Personal, which also gives a damage buff against that enemy), stronger throwing branch (Heave -> Thrown Weight), convergence through Rough Handling.

**W Shoulder Through:** longer engage branch (Head Down -> Run Them Down when exactly one enemy is hit), wider crowd displacement (Clear the Way -> Bowling Through), convergence through Bar Room Entrance.

**E Dig In:** defensive pressure absorption (Take It on the Chin -> Still Standing), retaliation (Come On Then -> Your Turn), convergence through Give It Back.

**R Drowned Man Walking:** altered melee state with a high Momentum floor, improved Clinch/Shoulder Through/Dig In and unnatural attack reach, followed by a temporary post-R Momentum-generation penalty at higher Madness.

# Appendix G - MVP Madness, Injury and relic specification

## G.1 Madnesses

- **Perception:** additional subjective reality layer; Crisis makes it dominant. Nyarlathotep resonance identifies the true avatar.
- **Compulsion:** contextual urges the player may indulge or resist; Crisis presents several simultaneously.
- **Dissociation:** predictable delayed ability echoes; Crisis produces an echo storm.
- **Obsession:** a contextual fixation target; Crisis produces one overwhelming fixation. Shub resonance privileges true reproductive nodes.

## G.2 Injury and revival

Injury events come from severe Health loss inside a rolling window. Specific Injuries change behaviour and are immediately explained. Once the specific-Injury capacity is filled, additional events become Grievous Injuries instead of additional debuffs.

Grievous Injury stacks make the investigator progressively slower to revive on a nonlinear curve. They do **not** create permanent individual Incapacitation. Treatment clears Grievous stacks before specific Injuries. TPK remains the sole universal failure state.

## G.3 Representative Injury catalogue

Broken Ribs, Concussion, Wounded Arm, Twisted Knee, Deep Cut, and Burns cover different behaviour-change patterns; exact mechanical magnitudes are tuning data.

## G.4 Relics

Current MVP relic mechanics are detailed in SYS-REL-001. The catalogue intentionally covers Threat, Break, CC, Healing, Injury, Madness, Movement, and Objective Interaction with one clearly noticeable systemic rule change in each area.

# Appendix H - MVP Elder Ones and intrusion enemies

## H.1 Shub-Niggurath

Shub is the conventional phase/vulnerability boss. Apocalypse begins with corruption in the basin and any unfinished Disruption sites. Her movement and growth nodes spread additional permanent corruption. Shub-linked enemies receive damage resistance on corrupted ground.

The resonant Obsession investigator identifies genuine reproductive nodes. The party still has to reach/control/destroy them.

Core boss actions: Trampling Advance, Black Milk, Call the Brood, Horned Sweep.

**Broodling:** corpse-consumption meter -> interruptible split -> two partially injured offspring -> offspring heal unless damaged. Broodling corpses contribute only a small fraction of standard corpse value.

**Spawn of the Black Goat:** charge/displacement bruiser.

## H.2 Nyarlathotep

Nyarlathotep uses a repeated avatar-identification structure. Several manifestations are active, one true and the rest false. All deal full damage. False avatars vanish after modest damage; only the true avatar advances the encounter. The Perception-resonant investigator sees which one is true.

**Crossing Paths:** avatars dash along damaging telegraphed lines and cross positions. True sight is suppressed briefly afterward.

**Unwelcome Attention:** all avatars focus one marked investigator, with increased likelihood of choosing the resonant player.

**Black Tongue:** Madness spike + immediate valid symptom from the target's own Madness.

**Borrowed Face:** exact presentation copy of an investigator using existing content rather than bespoke corrupted kits.

After sufficient correct avatar cycles, Nyarlathotep becomes a short direct execution fight using Black Tongue, a direct Crossing Blow, and Many Hands.

**False Man:** replaces an ordinary faction unit; subtle under-light is the only intentional pre-reveal tell. At low Health it becomes invulnerable, transforms/renames while healing and telegraphing a radius attack, fires the attack, then loses invulnerability.

**Winged Hunter:** small flying hit-and-run attacker. A melee hit during its committed dive window causes it to crash-land and become vulnerable on the ground.

# Appendix I - MVP investigation and challenge specification

The investigation is per-player, diegetic, and knowledge-based. It uses evidence to reveal observations, hypotheses, explicit Leads, and new narrative material. Leads can be completed retroactively when the condition existed before the player learned it. Team conditions give team credit; personal conditions remain personal. Knowledge earned before a later TPK persists unless victory is explicitly part of the condition.

Current content structure includes boss, scenario, and investigator threads across Discovery, Directed, and Mastery patterns. Character threads focus on Discovery/Directed challenges rather than extreme mastery feats.

Representative Lead examples:

- Shub Discovery: **Find the True Growth**.
- Nyarlathotep: identify true avatars, track correctly after Crossing Paths, and master the deception loop.
- Swamp scenario: expose the pre-human structure, complete a late Bell Sequence, and win after premature Apocalypse forces outstanding Disruptions into boss-active forms.
- Sapper: prepared-ground/Dead Ground interactions.
- Photographer: type-wide Clear Evidence/Impossible Photograph.
- Medium: Thin Places/Open Seance.
- Smuggler: Make It Personal/Drowned Man Walking.

# Appendix J - MVP technical and production acceptance

## J.1 Technical architecture

- Unreal Engine 5 with authoritative gameplay primarily in C++ and designer-facing presentation/content wrappers in Blueprint.
- Gameplay Ability System for abilities/effects; Gameplay Tags as the shared systems vocabulary; Enhanced Input and CommonUI for input/UI.
- Data-driven investigators, evolutions, Madnesses, Injuries, relics, factions, enemies, objectives, HTN methods, mutators, bosses, and Leads with stable IDs and validation.
- Server-owned named RNG streams for designed randomness; explicit authoritative update ordering.
- StateTree/Behaviour Trees/EQS/utility scoring chosen by problem rather than mandated uniformly.
- Team-level strategic AI above individual tactical combat AI.

## J.2 Multiplayer and migration

Normal multiplayer is listen-server authoritative. Matchmaking selects a host. Disconnects immediately convert the existing hero to AI. Backfill takes over that same hero and receives a short protected handover plus compact state summary. Host migration may pause/reconnect, but must resume valid authoritative state.

Migration snapshot state includes seed/RNG, HTN/objectives, Ritual, factions/director, corpses, hero build/resources, Health/Shield, Injury/Grievous, Madness, relics, threat/Break, AI assignments, and boss state.

## J.3 Subjective information

Private Madness and true-avatar information is filtered per client. Clients should not receive privileged hidden state merely to hide it visually when avoidable. AI obeys the same knowledge boundaries as humans.

## J.4 Headless test requirement

A complete zero-human four-bot run uses the same production gameplay systems as human play and can run headlessly. Deterministic seeds and compact AI decision traces allow outlier runs to be reproduced and diagnosed.

## J.5 Production gates

Do not scale content before proving:

- deterministic authoritative combat/run simulation;
- host migration with meaningful active state;
- private perception filtering;
- HTN repair;
- four-bot strategic coordination;
- two structurally different bosses on one semantic map contract;
- reliable navigation/collision transition when the basin is drained.

## J.6 Definition of MVP complete

External testers who did not participate in design can form a four-investigator party, understand the briefing, traverse the swamp, fight distinct factions, complete generated objectives, evolve builds, experience Madness/Injury/relic decisions, infer the Elder One, reach Apocalypse, defeat either boss or TPK, understand the consequence report, and receive meaningful investigation progress. They can then state concrete reasons they would approach another run differently.


# Appendix K - Detailed MVP investigator mechanics

This appendix is normative for the current MVP investigator implementations. The shorter investigator summaries in the main body and Appendix F are indexes; where they omit detail, this appendix governs. Exact damage, cooldown, range, duration, proc rate, resource capacity and similar numeric tuning remain data rather than locked GDD values.

## K.1 Shared investigator rules

Every investigator uses Basic Attack + Passive + Q/W/E + R. Q/W/E each use the converging lattice `A -> B/C; B -> D/E; C -> E/F`. The first branch creates a direction; the second either specialises or returns toward a hybrid E node. All meaningful build customisation occurs during the run. Every investigator has a bespoke resource system. R is strongly Madness-dependent and should use only a small number of major Madness consequences.

### AI requirement

Every ability and evolution node must expose AI valuation hooks. Bots choose builds from current run state rather than a fixed loadout. Character-specific weights change how the same situation is valued.

## K.2 Trench Raider / Sapper

**Role:** Vanguard / Controller. **Core verb:** shape the battlefield. **Personal weirdness:** still hears tactical instructions from soldiers who died in the Great War. **Resource:** Prepared Charges. **Basic:** semi-automatic carbine. **Passive:** Scrounger.

### Prepared Charges

Prepared Charges are a discrete carried stock. Satchel Charge consumes one. Human enemies can drop explosive components/ammunition; the Sapper auto-collects them and sufficient components restore one Prepared Charge. Non-human enemies do not normally replenish this resource. The exact carrying capacity, component requirement and drop chance remain tunable.

### Basic - Semi-Automatic Carbine

Reliable medium-range MOBA auto-attack with no separate ammunition subsystem. It provides moderate sustained damage while the Sapper prepares terrain. It is slightly more effective against Suppressed enemies, creating a baseline link to W without becoming another resource loop.

### Passive - Scrounger

Human enemies can replenish Prepared Charges through dropped components. Relevant drops are easy for the Sapper to recognise and collect. Scrounger exists to support the finite-charge loop rather than provide unrelated combat bonuses.

### Q - Satchel Charge

**A - Satchel Charge.** Place a persistent charge at a valid target location. It remains armed until detonated or removed and consumes one Prepared Charge.

**B - Shaped Blast.** Smaller effective blast footprint, substantially stronger central damage and Break. Favors elite, boss and priority-target demolition.

**C - Wide Charge.** Larger blast footprint and stronger displacement, with less concentrated central damage. Favors area control.

**D - Breaching Charge.** Deepens Shaped Blast. Further improves damage/Break against elites, bosses, objective objects and destructible structures.

**E - Concussive Charge.** Hybrid. Retains useful area and damage while adding strong stagger/displacement near the centre.

**F - Daisy Chain.** Deepens Wide Charge. Nearby prepared charges can participate in a deliberate linked detonation pattern where each blast has a valid tactical purpose. The implementation must not blindly expend charges into empty ground.

### W - Suppressing Fire

**A - Suppressing Fire.** Sustained cone/area fire dealing modest damage and applying strong Suppression/slow. Primary purpose is to hold enemies inside prepared zones.

**B - Covering Fire.** Broader/longer suppression. Allies operating through the affected space gain protection against ranged pressure.

**C - Pin Them Down.** Narrower area, stronger suppression and Break pressure.

**D - Entrenched Position.** Deepens Covering Fire. The Sapper gains a stronger defensive benefit while maintaining/fighting around the suppression zone.

**E - Kill Zone.** Hybrid. Suppressed enemies become easier to exploit with Satchel and Tripwire control/Break interactions.

**F - Walking Fire.** Deepens Pin Them Down. The suppression area can be swept/re-aimed while active to herd enemy movement.

### E - Tripwire

**A - Tripwire.** Place two endpoints independently within placement constraints, creating a persistent line. The first valid crossing triggers control. Common enemies can be stopped/knocked down; elites require appropriate Break vulnerability for full hard control.

**B - Reinforced Wire.** Longer/more tactically useful segment and stronger stopping/Break pressure.

**C - Detonator Wire.** A valid Tripwire trigger can also trigger an eligible nearby Satchel Charge.

**D - Clothesline.** Deepens Reinforced Wire. Crossing enemies take a substantial impact and are thrown back against their approach, especially punishing charging enemies.

**E - Prepared Ground.** Hybrid. Stronger interception plus local suppression/control around the crossing point, creating a reliable setup for a nearby charge.

**F - Resetting Fuse.** Deepens Detonator Wire. The wire rearms after a short reset rather than being consumed and can repeatedly trigger eligible nearby charges as new legitimate opportunities arise.

### R - Dead Ground

For a short altered-state window, traps that would normally and legitimately trigger do not immediately resolve. Instead, each triggering Satchel or Tripwire tags the enemy that would have caused it. After a short shared delay, every tagged enemy simultaneously receives the effects of all traps that tagged it. A target can accumulate multiple legitimate effects. Traps that never gain a valid trigger remain armed and unused.

The fantasy is not remote auto-detonation; it is that battlefield causality is deferred and then catches up at once. Activation creates a strong temporary Madness spike. Higher Madness may broaden the scale of the valid delayed interaction, but it never manufactures a trigger that would not otherwise have happened.

### Sapper AI identity

Prioritise objective approaches, choke points, predictable enemy movement, Break and valuable defensive preparation. Use Suppression to keep enemies inside prepared terrain. Avoid wasting Prepared Charges on low-value isolated targets. Seek human-enemy resupply when doing so does not compromise urgent team objectives.

## K.3 Expedition Photographer

**Role:** Striker / Support. **Core verb:** identify and exploit priority targets. **Personal weirdness:** photographs increasingly contain details that were not visible when the shutter was pressed. **Resource:** per-target Exposure. **Basic:** precision rifle. **Passive:** Perfect Moment.

### Exposure

Each enemy stores an independent Exposure state for that Photographer. Frame and Flashbulb add Exposure. Exposure decays gradually when the Photographer disengages from that subject. Develop consumes the target's stored Exposure. Higher Exposure creates stronger Develop outcomes. There is no finite film/plate ammunition economy.

### Basic - Precision Rifle

Long-range, slow, accurate single-target auto-attack. Damage improves with the target's current Exposure. Strong against priority targets and intentionally weaker at swarm clearing. No ammunition subsystem.

### Passive - Perfect Moment

Exposure builds faster against enemies visibly committed to telegraphed attacks, channels, casts or other vulnerable actions. The Photographer is rewarded for observing meaningful behaviour rather than simply holding Q on any target.

### Q - Frame the Subject

**A - Frame the Subject.** Maintain the target/area in frame to build Exposure and reveal useful combat information.

**B - Close Study.** Faster single-target Exposure and deeper combat information.

**C - Wide Angle.** Slower Exposure generation across several enemies in a small area.

**D - Tell-Tale Detail.** Deepens Close Study. Framing an enemy during an important telegraph/commitment produces a major Exposure gain and a brief vulnerability opportunity.

**E - Composition.** Hybrid. Strong Exposure on the primary subject plus lesser Exposure on nearby enemies.

**F - Group Portrait.** Deepens Wide Angle. Enemies framed together become partially linked for later Develop effects.

### W - Flashbulb

**A - Flashbulb.** Short cone flash that disrupts nearby enemies and rapidly adds Exposure.

**B - Blinding Flash.** Stronger disruption, interrupt and Break pressure.

**C - Overexpose.** Weaker control but substantially greater Exposure gain.

**D - Afterimage.** Deepens Blinding Flash. Affected enemies remain impaired briefly after the initial disruption.

**E - Caught in the Light.** Hybrid. Moderate disruption/Exposure and a brief window in which affected enemies are easier to Frame.

**F - Magnesium Burst.** Deepens Overexpose. Very large Exposure spike against enemies currently committed to telegraphed actions.

### E - Develop

**A - Develop.** Consume accumulated Exposure on the target to create an immediate payoff.

**B - Weak Point.** Single-target offensive branch: significant damage plus a brief vulnerability window.

**C - Clear Evidence.** Information branch: the resulting debuff applies to all enemies of the studied enemy type, not only the photographed specimen.

**D - Fatal Detail.** Deepens Weak Point. High-Exposure Develop becomes a major burst/Break event against elites and bosses.

**E - Published Findings.** Hybrid. Moderate burst on the studied target plus a shorter type-wide vulnerability/debuff.

**F - Case File.** Deepens Clear Evidence. The type-wide debuff is stronger and/or lasts longer.

### R - Impossible Photograph

The Photographer captures the visible battlefield as a whole. For a short window, visible enemies become heavily/fully Exposed, stored Exposure is prevented from decaying, and Develop can be used repeatedly without immediately exhausting that stored Exposure. At higher Madness, the photograph can include normally concealed or subjective phenomena. Activation creates a strong Madness spike.

### Photographer AI identity

Prioritise high-value targets, repeated enemy types, telegraphed actions and safe sightlines. Treat Exposure as an investment in a subject, not generic combo points. Choose between single-target burst and type-wide utility based on encounter composition and current team need.

## K.4 Stage Medium

**Role:** Controller / Support. **Core verb:** manipulate states and positioning indirectly. **Personal weirdness:** a stage medium whose performances ceased to be wholly fraudulent when the dead began answering. **Resource:** per-spirit Attention. **Basic:** Spirit Lash. **Passive:** Thin Places.

### Spirit Attention

Each bound spirit has its own Attention state. Deaths, Downed investigators, ritual sites and strong Madness phenomena can increase nearby spirits' Attention. Higher Attention strengthens passive spirit presence. Intercession normally selects the highest-Attention eligible spirit and substantially reduces its Attention after use. Beckon can reposition spirits toward useful Attention sources.

### Basic - Spirit Lash

Medium-range occult projectile with modest reliable damage. Striking an enemy carrying a bound spirit generates additional Attention for that spirit.

### Passive - Thin Places

Deaths, Downed investigators, ritual sites and strong Madness phenomena create especially strong Attention opportunities. The Medium becomes more capable as the boundary between ordinary and supernatural reality weakens.

### Q - Bind Spirit

**A - Bind Spirit.** Attach/place a spirit on an ally, enemy or valid location; passive behaviour depends on binding target.

**B - Guardian Spirit.** Stronger protective binding on allies/defensive locations.

**C - Haunting.** Stronger hostile binding, hindrance and Attention generation from an enemy.

**D - Vigil.** Deepens Guardian Spirit. The spirit can automatically intervene when its protected target is threatened.

**E - Restless Watcher.** Hybrid. Moderate protection/hindrance, with added value when repositioned by Beckon.

**F - Possession.** Deepens Haunting. At high Attention, the spirit can briefly disrupt the bound enemy or force a positional error.

### W - Beckon

**A - Beckon.** Redirect bound spirits toward a selected point; movement/arrival can trigger effects.

**B - Procession.** Longer/faster spirit movement; enemies crossed are briefly slowed/displaced.

**C - Calling.** Shorter movement, stronger arrival trigger.

**D - Funeral March.** Deepens Procession. Spirits leave a temporary route that hinders enemies and lightly protects allies crossing it.

**E - Crossroads.** Hybrid. Moderate route effect plus moderate arrival trigger.

**F - Seance.** Deepens Calling. Several spirits arriving close together trigger an additional combined effect.

### E - Intercession

**A - Intercession.** Command the highest-Attention spirit to make a strong intervention: protect, hinder, block or displace. Normally spends substantial Attention.

**B - Guardian's Hand.** Stronger protective intervention.

**C - Unquiet Dead.** Stronger hostile intervention and Break pressure.

**D - Not Yet.** Deepens Guardian's Hand. Especially valuable around a Downed investigator or active revive, improving the safety/speed of rescue without becoming a general instant revive.

**E - Between Worlds.** Hybrid. Moderate protection/control and a temporary spirit presence at the intervention point.

**F - Drag Below.** Deepens Unquiet Dead. Against sufficiently Broken enemies, violently repositions and heavily impairs the target.

### R - Open Seance

For a short window all bound spirits fully manifest, their passive effects intensify, Beckon moves them as active battlefield presences and Intercession no longer exhausts the selected spirit in the normal way. At higher Madness, additional unbound spirits can appear around recent deaths and Downed investigators. Activation produces a strong Madness spike.

### Medium AI identity

Value threatened/Downed allies, high-Attention spirits, important hostile bindings, control opportunities and positioning spirits before spending them. Do not expend Intercession simply because it is available; preserve useful high-Attention spirits for meaningful intervention.

## K.5 Bare-Knuckle Smuggler

**Role:** Striker / Vanguard. **Core verb:** stay engaged and snowball pressure. **Personal weirdness:** survived an implausibly long drowning and sometimes coughs up water nowhere near the sea. **Resource:** Momentum. **Basic:** bare-knuckle combo. **Passive:** Keep Your Feet.

### Momentum

Momentum is a build-and-decay combat-state meter. Basic attacks, Clinch, Shoulder Through, displacement and receiving meaningful pressure generate Momentum. Higher threshold bands improve stickiness and resistance to slows/displacement. Momentum begins decaying only after a meaningful period with neither outgoing nor incoming combat pressure. Momentum is not spent to cast abilities.

### Basic - Bare-Knuckle Combo

Short-range repeating melee sequence. Successful hits generate Momentum. The final hit in the cadence produces a small shove/stagger against common enemies. Repeated attacks against the same target improve stickiness so the Smuggler can remain engaged.

### Passive - Keep Your Feet

Higher Momentum increases resistance to slow/displacement and improves melee stickiness.

### Q - Clinch

**A - Clinch.** Grab nearby enemy; recast to shove/throw in a chosen direction. Common enemies are fully controllable; elites require appropriate Break vulnerability.

**B - Lock Up.** Stronger hold/control and Break pressure.

**C - Heave.** Shorter emphasis on hold, substantially stronger throw distance and collision force.

**D - Make It Personal.** Deepens Lock Up. After release the Smuggler jumps sharply up that enemy's threat table and gains a temporary damage buff specifically against that enemy.

**E - Rough Handling.** Hybrid. Solid hold/throw; colliding the thrown enemy with another target briefly staggers both.

**F - Thrown Weight.** Deepens Heave. Thrown enemies of any kind deal substantial damage and Break to enemies they collide with; heavier targets can create stronger impacts.

### W - Shoulder Through

**A - Shoulder Through.** Short charge that damages/displaces enemies and generates Momentum from meaningful contacts.

**B - Head Down.** Longer charge, greater control resistance while moving and stronger first-target Momentum generation.

**C - Clear the Way.** Shorter but wider charge with stronger group displacement.

**D - Run Them Down.** Deepens Head Down. Triggers when the charge hits exactly one enemy; carries farther through/with that target and grants a temporary damage bonus against it.

**E - Bar Room Entrance.** Hybrid. Moderate range/width with a brief area stagger at the primary impact.

**F - Bowling Through.** Deepens Clear the Way. Successive enemies struck in the same charge suffer increasing displacement and/or Break pressure.

### E - Dig In

**A - Dig In.** Brace against incoming pressure, gaining control/displacement resistance and converting incoming pressure into Momentum; can finish with a counter-shove.

**B - Take It on the Chin.** Stronger mitigation/control resistance and improved Momentum generation from incoming pressure.

**C - Come On Then.** Less pure mitigation, but enemies striking the Smuggler during Dig In are primed for stronger retaliation.

**D - Still Standing.** Deepens Take It on the Chin. Absorbing enough pressure grants a short sustain/Health-recovery window afterward.

**E - Give It Back.** Hybrid. Moderate protection plus a counter-shove whose strength scales with absorbed pressure.

**F - Your Turn.** Deepens Come On Then. Ending Dig In produces a heavy retaliation with bonus Break against the most threatening enemy that attacked during the stance.

### R - Drowned Man Walking

For a short altered-state window Momentum cannot fall below a high floor; Clinch works more effectively against heavy/Broken enemies; Shoulder Through gains stronger collision/displacement; Dig In converts incoming pressure into Momentum more efficiently; and basic attacks gain unnatural additional reach. Activation causes a major Madness spike. Higher Madness can prolong/strengthen the state at the cost of a temporary post-R penalty to Momentum generation.

### Smuggler AI identity

Value sustained engagement, appropriate threat, isolated targets, displacement, protecting ranged allies and opportunities to move Shub-linked enemies out of corruption or crash-land Winged Hunters. Do not remain in lethal terrain solely to preserve Momentum.

# Appendix L - Detailed MVP scenario and objective mechanics

This appendix is normative for the rural swamp fishing-village scenario. Exact timings, map dimensions, encounter budgets and objective quantities remain tunable.

## L.1 Scenario identity and map topology

The map is a swamp containing a small rural fishing village, not a village bordered by decorative marsh. Gameplay remains on one top-down 2D navigation plane. The built environment consists of a compact village centre, scattered fishing huts/jetties, an isolated church and graveyard, an isolated lighthouse, pumping/drainage infrastructure and a smuggler hideout/boathouse. Several swamp sub-zones provide distinct traversal texture: open deep marsh, reeds/flooded channels and mudflat/basin areas.

The ritual centre is a pre-human structure submerged beneath the swamp surface, not underground. Late in the compulsory chain, the water-control objective drains the basin and reveals the structure on the same gameplay plane. This is the major topology transformation of the run.

## L.2 Route hierarchy

**Causeways/boardwalks:** fastest and safest movement, but predictable and easy for factions to hold.

**Shallow swamp/reeds:** slower, poorer vision and ambush-prone, but offers shortcuts and alternate approaches.

**Deep water/mud:** normally impassable or prohibitively slow and used to shape route boundaries.

The game should create rotation decisions, not hiking. Traversal pressure comes from routes, patrols, hazards, objectives and ambushes.

## L.3 Fog and lighthouse

Fog hides live state such as enemies, patrols, hazards and transient events. Known objectives remain visible through fog. The lighthouse is a pure optional Power-up. Restoring it grants persistent vision over most of the map. It does not damage enemies, weaken the boss, accelerate objectives or become part of the boss contract.

## L.4 Compulsory HTN chain

The critical chain has four functional stages. Each current stage has two authored mechanical decompositions. The HTN selects a coherent route at run generation.

### Understand the disturbance

**Find the Missing Fisherman:** tracking/movement plus rescue or evidence recovery in huts/reeds.

**Examine the Impossible Catch:** environmental investigation/interaction around an anomalous catch, abandoned boat or fish-handling site.

### Trace the ritual infrastructure

**Inspect the Bell Network:** investigate bell/signal sites under local Cult pressure and establish that the bell system participates in the ritual.

**Trace the Waterworks:** inspect pumps, drainage and altered flow to establish water-control infrastructure as ritual geometry.

### Expose the ritual structure

**Restore the Main Pump:** concentrated machinery repair/activation and defence.

**Open the Emergency Sluices:** distributed route-control objective across remote points, creating movement and split pressure.

Both produce the same world-state result: the basin drains and the pre-human structure becomes traversable.

### Gain the means to force manifestation

**Charge the Counter-Sigil:** multi-point activation/hold/movement around the newly exposed structure.

**Assemble Ritual Components:** collect/carry/deliver components into marked sockets/points while defending carriers.

Completing this stage plus the required Disruption set enables deliberate summoning.

## L.5 Gameplay-first narrative rule

Mechanical objective information is always sufficient to win. Flavor text, speech/thought bubbles, barks and archive material explain why the action matters but are not instructions. The HTN plans mechanical tasks first and binds authored narrative wrappers afterward.

## L.6 HTN repair

The initial mission plan is generated at run start. Unrevealed future tasks can be silently repaired. Once an objective or fact is visible, it cannot be silently rewritten. Visible repair is fiction-first and mechanically explicit. The replacement is normally worse through more travel, harder encounter pressure, added Ritual progress, lost reward, worse position or additional Injury/Madness exposure.

## L.7 Disruption objective - Break the Bell Sequence

Core mechanic is a Simon-Says-like memory/sequence task performed while combat continues. Difficulty escalates through execution pressure, not false audio information.

**Early:** observe and perform the counter-sequence while defending the bell site.

**Intrusion:** longer sequence, tighter safe pauses and heavier interruption pressure.

**Convergence:** sequence work is distributed across multiple bell sites, creating movement/split coordination.

**Apocalypse version - Bells of Manifestation:** the boss is already active while the team performs the full late-stage counter-sequence. Completion resolves the same unfinished ritual requirement rather than a generic replacement task.

## L.8 Disruption objective - Protect the Counter-Ritualist

Core mechanic is moving defence and interruption prevention.

**Early:** escort the Counter-Ritualist to one ritual point and defend while they work.

**Intrusion:** the ritualist must work at multiple nearby sites and enemies increasingly prioritise interruption.

**Convergence:** a longer multi-site route with warding stops that create short defensive holds.

**Apocalypse version - Last Counter-Rite:** the full route continues under active boss pressure. Interruption pauses progress rather than arbitrarily resetting completed work.

## L.9 Disruption objective - Shatter the Marsh Idols

Static idols occupy authored ritual sites across the swamp. Players rotate to destroy them. They do not move or become mobile enemies.

At later Ritual stages, surviving idols buff enemies fighting nearby, making postponed clearance progressively more dangerous.

**Apocalypse version - Anchors of the Apocalypse:** surviving idols remain active while the boss is present, continue strengthening nearby enemies and must be destroyed under boss pressure.

## L.10 Power-up objective templates

**Smuggler Cache:** combat clearance/territory capture followed by opening the cache; rewards a random shared relic.

**Lost Curio:** recover/carry/deliver an object from a hazardous swamp location to a safe inspection point; rewards a random shared relic.

**Open the Surgery:** clear/defend a treatment location; opens a persistent limited-use treatment source.

**Rescue the Doctor:** reach an NPC under pressure and escort them a short distance to a treatment location; opens a persistent limited-use treatment source.

**Lighthouse:** restore the lighthouse to gain persistent vision over most of the map.

Relic and medical opportunities can recur. The scenario should present enough optional opportunities that full-map clearing is normally unattractive under Ritual pressure. Exact opportunity count remains tuning/content data.

## L.11 Food sustain

Ordinary enemies do not drop generic Health pickups. Food appears at plausible authored spawn points such as homes, huts, kitchens, village stores and smuggler supplies. Only a subset of valid points are active in a run. Food auto-collects when an injured investigator approaches and provides Heal-over-Time rather than instant burst healing.

## L.12 Objective urgency

Time-sensitive objectives move through broad deterioration states and end in an explicit Imminent countdown. The countdown is tuned against rotation feasibility: a reasonably positioned team that responds immediately should usually be able to contest the objective, while poor positioning or commitment elsewhere can make failure unavoidable.

## L.13 Ritual-stage mechanical escalation

Ritual stages must escalate mechanics/objectives, not only art.

**Incipient:** native factions dominate; objective states are simpler; local crises manageable; Mythos overlay mostly ambiguous.

**Stirring:** multiple priorities can deteriorate; reinforcement/director pressure rises; objective complications begin; splitting becomes attractive.

**Intrusion:** Mythos enters ordinary objective rules; elites become more common; boss-linked intrusion appears; objective variants become harder; Madness triggers become more frequent.

**Convergence:** several crises can coexist; Disruptions use their hardest authored pre-Apocalypse forms; optional tasks become expensive; director pressure is high; actual-boss ecology dominates new intrusion.

**Apocalypse:** boss active; remaining mandatory tasks become their boss-active Apocalypse versions.

Deliberate summoning jumps immediately from the current pre-Apocalypse stage to Apocalypse. Natural Apocalypse before readiness remains winnable through transformed outstanding tasks.

## L.14 Boss basin

The drained basin is the canonical final confrontation site. It contains a central pre-human structure, outer ritual platforms/nodes, shallow channels/raised stone ribs, multiple entrances and enough open/structured space for adds, line-of-sight play, Shub movement/corruption and Nyarlathotep manifestations. The same arena must support both bosses without duplicate boss-specific levels.

# Appendix M - Detailed MVP Madness, Injury, healing and relic mechanics

## M.1 Health and Injury

Health is tactical and comparatively recoverable. The server tracks recent Health loss across a rolling window; sufficiently concentrated damage generates an Injury event. This should make chip damage manageable while focus fire, failed boss mechanics and overlapping hazards create lasting cost.

Specific Injuries are immediately named/explained and change behaviour rather than primarily applying passive stat penalties. The current representative Injury themes are Broken Ribs, Concussion, Wounded Arm, Twisted Knee, Deep Cut and Burns. Exact effects are tuning data but should respectively pressure repeated heavy impacts, rapid ability chaining, uninterrupted basic attacks, displacement/aggressive movement, recovery after burst damage, and persistent-hazard exposure.

Once the specific-Injury capacity is filled, subsequent Injury events create Grievous Injury stacks instead of more mechanical debuffs. Grievous stacks must be treated before specific Injuries. There is no permanent individual Incapacitation state.

## M.2 Downed and revival

At zero Health the investigator becomes Downed and suffers an Injury event. Allies revive through a vulnerable interaction. Revived investigators return with meaningful but incomplete Health and substantially reduced, not reset, threat. Specific enemies/scenario states may exploit Downed heroes through power drain, anchoring, apparitions, infestation or dragging.

Grievous Injury stacks increase revive time on a deliberately nonlinear escalating curve. The exact curve is balance data. Repeated trauma therefore makes rescue increasingly dangerous without removing a player into spectator mode. TPK remains the sole universal failure condition.

## M.3 Healing and treatment

Health recovery does not clear Injury. Ordinary world sustain comes from authored food pickups that auto-collect for injured investigators and heal over time. Medical Power-up objectives open limited shared treatment sources. Each treatment removes one Grievous stack first; if none remain, it removes one specific Injury.

## M.4 Madness core

Each investigator receives one run-specific Madness. Current Madness can rise/fall; the Madness floor rises through irreversible run development such as deeper evolution and selected occult effects. Recovery cannot reduce current Madness below the floor. Players can deliberately accelerate/restrain Madness but a full run trends toward greater instability.

Thresholds unlock symptom pools; contextual triggers decide when symptoms manifest. Maximum Madness enters a temporary Crisis specific to the Madness; player control is retained throughout. After Crisis, current Madness falls toward but not below the floor.

## M.5 Subjective information and accessibility

The affected player receives the full subjective manifestation. Teammates see only externally observable consequences unless an effect explicitly becomes shared reality. Subjective targets can be pinged as 'this investigator perceives something here'. Accessibility equivalents remain private and must preserve uncertainty: audio can gain directional text/visual equivalents; colour must not be the sole signal; distortion can be reduced without deleting gameplay information.

## M.6 Perception

Perception means seeing an additional subjective layer rather than merely being lied to.

**Early:** harmless/informative anomalies and impossible details.

**Mid:** private hazards, enemies, objects or interactables.

**High:** interaction with subjective entities can affect real combat state.

**Crisis:** the subjective layer becomes dominant; private entities/hazards become numerous and shared-world information is visually de-emphasised but never hidden. The investigator gains a strong advantage against subjective-layer entities.

**Nyarlathotep resonance:** the same perceptual channel reveals which avatar is genuine. Crossing Paths can temporarily suppress this true-sight distinction; during a Perception Crisis the true avatar becomes especially obvious.

## M.7 Compulsion

Contextual enemies, corpses, locations, ritual objects or interactions become urges. Indulging an urge can reduce current Madness or grant a brief benefit; resisting increases Madness pressure. Control is never removed.

**Crisis:** several urges become active simultaneously, creating a short priority-management problem rather than a forced-action state.

## M.8 Dissociation

Certain abilities create weaker delayed echoes. Echo timing is predictable. At higher Madness the echo increasingly resolves from the original cast position/state rather than the investigator's current position, creating planned future effects.

**Crisis:** most Q/W/E casts echo for a short period, allowing deliberate layered damage/control while forcing the player to manage the consequences of recent actions.

## M.9 Obsession

At intervals one enemy, corpse, growth, objective element or location becomes a fixation. Acting on it eases Madness or grants a benefit; ignoring it increases pressure.

**Crisis:** one target becomes an overwhelming fixation; acting on it produces escalating benefit and resolving it ends the Crisis.

**Shub resonance:** genuine reproductive nodes can become privileged fixation targets, allowing the resonant investigator to distinguish meaningful growths from irrelevant/false ones.

## M.10 Resonance assignment

For the current MVP, exactly one investigator receives a Madness resonant with the actual Elder One: Obsession for Shub-Niggurath or Perception for Nyarlathotep. The full later-content association system must ensure selected Madness association sets do not create trivial negative deduction; that exhaustive pairwise-disjoint validation is not an MVP requirement beyond proving exactly one current-boss resonance and private information handling.

## M.11 Relic rules

Relics are random, run-specific systemic rule-breakers distributed through Need/Greed/Pass. They are never hero-specific loot. Carrying capacity is intentionally low so passing on merely adequate drops can be rational. Exact cap is a balance/scope value rather than a universal design number. Relics should change decisions rather than provide filler percentage bonuses.

### Officer's Swagger - Threat

The holder's first meaningful hard-control or substantial Break interaction against an enemy pushes them sharply upward on that enemy's threat table. While the enemy focuses the holder, allies become more effective at applying Break.

### Cracked Saint's Medal - Break

Meaningfully contributing to Breaking an elite/boss primes the holder's next damaging/control ability against that target. Using the primed ability during the Break window strengthens its payoff and slightly extends the vulnerability opportunity.

### Hangman's Knot - Crowd Control

Applying hard CC to a target causes nearby enemies to receive a weaker secondary control effect such as Slow/Suppression. Secondary targets do not receive equivalent hard CC.

### Overheal Shield relic - Healing

Healing above maximum Health becomes Shield. The Shield remains briefly at full value and then decays steadily. Further overhealing can refresh/add to it up to an appropriate tunable cap. Final flavour name remains open.

### Soldier's Morphine Tin - Injury

On acquisition, remove one current specific Injury where possible. Future Injury events become Grievous Injuries rather than new specific Injuries. The holder is significantly faster to revive and also revives others significantly faster. The relic trades behavioural wound accumulation for Grievous trauma; it does not prevent physical attrition.

### Black Glass Rosary - Madness

Crossing upward into a new Madness tier grants a short strong improvement to the investigator's bespoke resource generation/effectiveness. It does not reduce Madness, suppress symptoms or prevent Crisis.

### Ferryman's Coin - Movement

Significant movement in a short period leaves a temporary wake. Allies crossing it gain a short movement advantage. Enemies crossing/standing in it are slowed and become easier to affect with CC/Break. Wakes are short-lived and should not stack heavily.

### Saint's Work Gloves - Objective Interaction

While carrying, channeling or operating an objective, the holder gains substantial control/displacement resistance and ordinary damage does not interrupt the interaction. Explicit interrupt mechanics still work. Completing or deliberately releasing the interaction grants nearby allies a brief defensive benefit.

# Appendix N - Detailed MVP Elder One encounters and intrusion enemies

## N.1 Shared Elder One rules

The Elder One is secret at run generation. Early Ritual signs are shared/ambiguous. Intrusion first narrows possibilities by mixing enemies associated with more than one candidate boss; later Convergence stops introducing false candidates and increasingly uses the actual boss ecology. Resonant Madness provides a second clue. Boss lore is optional; mechanics are communicated through telegraphs, target markers and state changes.

## N.2 Shub-Niggurath

**Encounter identity:** permanent space corruption + multiplicative add pressure + meaningful reproductive nodes. **Resonant Madness:** Obsession.

### Corruption

Apocalypse begins with corruption in the boss basin and at any unfinished Disruption objective locations. Shub leaves corruption wherever she moves. Growth nodes can create further spread. Existing corruption remains for the rest of Apocalypse; destroying a source only prevents new spread. Investigators remaining in corruption gain Madness pressure. Shub-linked enemies gain damage resistance while on corruption, making displacement/control directly valuable.

### Broodling

Broodlings feed on persistent gameplay corpse markers. Corpse consumption fills a replication meter. A Broodling corpse contributes only a small fraction of a standard corpse's feeding value so the population cannot sustain exponential multiplication purely on its own dead. Feeding is interruptible.

At threshold, the Broodling performs an interruptible split. On completion, the parent becomes two partially injured Broodlings. Both begin healing toward full, and any incoming damage stops that healing. The team therefore has three counterplay windows: interrupt feeding, interrupt splitting, or tag fresh offspring before they stabilise.

### Spawn of the Black Goat

Large charge/displacement bruiser. Its job is to break formations and punish narrow causeways/clumped teams, not to summon more adds. Break provides a clear answer to committed charges.

### Boss phases

The fight broadly progresses from rooted/reproductive pressure, through a mobile manifestation, into a final birthing frenzy. Exact phase count/thresholds are tunable. The arena gets worse because corruption and unresolved state accumulate, not merely because boss stats rise.

### Trampling Advance

Telegraphed charge/movement through the basin. Causes strong displacement and leaves permanent corruption along Shub's path.

### Black Milk

Places corruption pools around targeted investigators, forcing temporary spread/repositioning. Primary purpose is terrain pressure rather than raw burst.

### Call the Brood

Interacts with the corpse/Broodling ecosystem: can draw Broodlings to corpses, create feeding pressure, or introduce Broodlings when needed to maintain the reproductive loop.

### Horned Sweep

Broad close positional attack producing heavy disruption/Break pressure and displacement rather than relying on one-shot damage.

### Vulnerability

Successful control/destruction of genuine reproductive nodes creates meaningful vulnerability opportunities. The Obsession-resonant investigator helps identify the true nodes; the whole team must reach/control/destroy them.

## N.3 Nyarlathotep

**Encounter identity:** distributed deception + subjective identification + dangerous false manifestations. **Resonant Madness:** Perception.

### Avatar cycle

Several manifestations appear simultaneously in the drained basin. One is genuine and the rest false. False avatars deal full real damage. They can be manually tested: enough damage causes a false avatar to disperse, removing that pressure source but not advancing the encounter. The resonant investigator sees the true avatar through private true sight.

Destroying the true avatar completes an identification cycle. After several successful cycles, Nyarlathotep is forced into a short conventional direct manifestation. Exact avatar count/cycle count are tuning/content values.

### Borrowed Face

One manifestation becomes an exact presentation copy of an investigator using the existing hero model/animation presentation. No distorted bespoke duplicate kit is authored. Mechanically the avatar remains Nyarlathotep.

### Unwelcome Attention

All avatars temporarily ignore ordinary threat and focus one marked investigator. Selection is biased toward the Perception-resonant investigator but not guaranteed.

### Crossing Paths

Avatars dash along clearly telegraphed damaging lines and can cross/exchange relative positions. The team must both avoid the lines and visually track the target. After the move resolves, the resonant investigator's true-sight distinction is temporarily disabled; the party must rely on tracking or manual testing until it returns.

### Black Tongue

Applies a Madness spike and immediately triggers one valid contextual symptom from the target's existing Madness. It weaponises what is already wrong with the investigator rather than adding a separate generic sanity debuff.

### Final direct manifestation

After the avatar cycle is resolved, false manifestations cease. Nyarlathotep enters a short aggressive physical burn phase in the basin. Normal threat mostly resumes. The final set retains Black Tongue and adds Crossing Blow (direct damaging dash/line attack without avatar swapping) and Many Hands (broad close-range tendril sweep). There is no new puzzle after the deception loop; the payoff is finally getting to kill the true presence.

## N.4 False Man intrusion enemy

From the Intrusion stage onward the director can secretly replace an ordinary faction unit with a False Man. It keeps the replaced unit's apparent name/form. The only intentional pre-reveal tell is a subtle unnatural glow/under-light.

It attacks at medium melee range with tendrils. When sufficiently wounded, it becomes temporarily invulnerable, transforms and renames while healing, and clearly telegraphs a radius attack. The radius attack resolves as transformation completes, then invulnerability ends. The sequence is a readable repositioning window, not a surprise unavoidable blast.

## N.5 Winged Hunter intrusion enemy

Small dragon-like flying hit-and-run attacker. It selects exposed/isolated targets more readily than ordinary threat targeting, telegraphs a dive/strafing pass, deals damage along the pass, disengages and re-enters from another angle. A melee attack during the correct committed approach window causes it to crash-land, after which it becomes vulnerable to ordinary ground control and focused damage.

# Appendix O - Detailed MVP technical architecture

This appendix is normative technical direction for the MVP. Engine-version pinning, tick frequency, hardware targets and exact performance budgets remain open until representative profiling.

## O.1 C++ / Blueprint boundary

Authoritative deterministic rules should primarily live in C++: combat resolution, Health/Shield/Injury/Madness, threat, Break, XP/evolution validity, hero resources, HTN, objectives, Ritual, director, deterministic RNG, AI utility/team planning, snapshot serialization and filtered gameplay replication. Blueprint is appropriate for designer-facing composition, presentation, VFX/animation hooks, map scripting wrappers and cosmetic sequencing where authoritative state remains in validated systems/data.

## O.2 Gameplay Ability System and Gameplay Tags

Use GAS for investigator/enemy abilities, attributes/effects, cooldowns, buffs/debuffs, CC, Break interactions, Shield and replicated ability state where suitable. Use Gameplay Tags as the shared semantic vocabulary for state, CC, enemy role/faction, objective type, Madness, map affordances and system interactions.

Representative tag families: `State.*`, `CC.*`, `Enemy.Faction.*`, `Enemy.Intrusion.*`, `Objective.*`, `Madness.*`, `Map.*`. The exact taxonomy evolves with implementation but stable semantics matter more than display names.

## O.3 Data-driven ability evolution

Avoid a separate duplicated ability class for every evolution node where modular behaviour suffices. Each node should carry stable ID, predecessor rules, display text, granted tags/effects, behaviour modifiers, AI valuation hooks and Madness-floor impact. Validation must ensure all nodes are reachable and convergence E is reachable from both branches.

## O.4 Common hero-resource interface

All bespoke resources should expose common operations for authoritative state, HUD reporting, AI valuation, replication and snapshot serialization while retaining different internal rules.

**Prepared Charges:** current stock, capacity, component progress, placed Satchels/Tripwires and Dead Ground tag state.

**Exposure:** lightweight per-target/per-Photographer state supporting add, decay, Develop consumption, type-wide effects and cleanup when targets die.

**Spirit Attention:** explicit lightweight spirit entities/components with owner, binding, Attention, position, effects and Intercession eligibility.

**Momentum:** hero-local meter/bands with gains, delayed decay, R minimum floor and post-R penalty.

## O.5 Threat API

Threat should be server authoritative and modified only through a shared API: add/multiply/reduce threat, set minimum position, query highest threat, apply/clear temporary targeting overrides and remove invalid targets. This allows hero/relic/enemy interactions without direct table manipulation scattered through content code.

## O.6 Break / CC resolution

Common enemies receive full CC. Elites/bosses expose a Break/Resolve component tracking current/max Resolve, Break damage, Broken state and recovery. CC on protected targets converts to partial effect/interrupt/Break contribution until Broken; then stronger control can apply for a defined window. Bosses may also expose explicit interrupt windows independent of full Break.

## O.7 Corpse state

Authoritative corpse gameplay markers are separate from client ragdolls. Store location, source enemy type, corpse value, consumption state and cleanup eligibility. Broodling feeding and future corpse mechanics query this state; cosmetic corpses may despawn independently.

## O.8 AI stack

Use StateTree and/or Behaviour Trees for local execution, EQS for useful spatial queries and utility scoring for decisions. A separate team-level strategic planner runs above individual tactical AI.

Typical local states include travel, patrol, engage, reposition, ability execution, objective defence, revive, retreat, regroup, ping investigation and Madness-specific behaviour.

Team assignments use objective urgency, travel cost, active encounters, party condition, Downed state, Power-up value and Ritual pressure. Use hysteresis/commitment so bots do not oscillate between similar assignments.

## O.9 HTN domain and revealed-state tracking

Each scenario's deterministic HTN domain contains compound tasks, primitive tasks, methods, preconditions, costs, exclusions, repair methods, narrative bindings and required map tags. Same seed + initial state + content version should generate the same initial plan. Track whether tasks/facts are player-visible so hidden future state can repair silently while visible state requires explicit fiction-first repair.

## O.10 Objective state machine

Objectives should support explicit authoritative states such as available, active, deteriorating, critical, imminent, completed, failed, repaired and Apocalypse-converted. Each objective instance binds a mechanical template, location, faction pressure, escalation data and optional flavor wrapper.

## O.11 Ritual service

One authoritative Ritual service owns stage/progress and all advancement. Time, objective failures, HTN repair costs and authored events call a shared advancement API rather than mutating stage state independently. Deliberate summon performs a deterministic transition directly to Apocalypse. Premature Apocalypse converts outstanding mandatory objective instances rather than spawning a generic fallback.

## O.12 Encounter director

The director sits above the authored initial ecology. It activates reinforcements/patrols, repopulates routes, introduces elites and boss-linked intrusion according to Ritual stage, faction legality, active objective, location, current pressure and encounter budget. It should use enemy cost/budget data rather than unbounded spawning. It must not secretly rubber-band by altering player/enemy stats according to success.

## O.13 Boss-Map contract

Maps expose semantic affordances such as BossSite, ManifestationPoint, GrowthSite, SpawnRoute, ObjectiveAnchor, HazardRegion, MovementLane, CorruptionValid and ArenaEntrance. Elder Ones query capabilities rather than hard-coded level object names. Automated validation must report a content error if a supported boss lacks required affordances on a supported map.

## O.14 Subjective replication

The server owns true state but clients should receive only the information they are entitled to render where practical. Use owner-only data, per-connection relevancy or Replication Graph rules as needed. This applies to Perception entities, Nyarlathotep true-avatar state, fog-hidden enemies and unrevealed information. AI knowledge must follow the same entitlement boundary.

## O.15 Named deterministic RNG streams

Use separate seeded streams for materially different systems, for example RunGeneration, ElderOne, Madness, Relics, HTN, Director, Combat, AIChoice, Faction and BossVariation. Adding an unrelated random roll to one subsystem should not silently perturb all later random outcomes in another.

## O.16 Authoritative update ordering

Define a stable fixed authoritative ordering for systems whose sequence changes outcomes. The exact order is an implementation decision, but must explicitly cover input acceptance, movement, ability resolution, damage/healing, threat/Break, Injury/Madness, objectives/Ritual, AI/team decisions, director decisions and replication. Client animation/particles/cosmetic physics are outside deterministic requirements.

## O.17 Deterministic boundary and physics/navigation

Avoid authoritative dependence on unstable cosmetic physics. Knockback and projectile outcomes should derive from controlled server gameplay logic. Navigation/path selection should use stable candidate ordering/tie-breaks where reproducibility matters. The goal is deterministic authoritative outcomes, not bit-identical presentation.

## O.18 Headless four-bot simulation

Provide a server/headless build capable of running the real scenario, four production AI investigators, enemy AI, objectives, Madness, Ritual and bosses with no rendering. It should run faster than real time where feasible by disabling presentation waits/cosmetic systems. The exact throughput target is set only after representative profiling.

## O.19 Gameplay event model

Major systems emit structured events such as EnemyKilled, InjuryGained, HeroDowned, HeroRevived, MadnessTierCrossed, CrisisStarted, ObjectiveCompleted/Failed, RitualStageChanged, BossManifested, RelicAcquired, BreakTriggered and CorpseConsumed. Investigation Leads, telemetry, UI and tests consume these events rather than independently re-inferring what happened.

## O.20 Host migration snapshot

Snapshot must include run seed and RNG states, Game/Player state, hero Health/Shield/evolutions/resources/Madness/Injuries/relics, threat/Break, HTN and revealed state, objectives, Ritual, director/faction state, corpse markers, AI strategic assignments and boss-specific state such as growths/avatars. Snapshot schema is versioned. Host migration may pause/reconnect; correctness is more important than seamlessness.

## O.21 Navigation and basin transition

The swamp uses authored navigation areas/links for boardwalk, shallow swamp and blocked deep water. The drainage transition must predictably update/switch collision/navigation so AI and players can enter the newly exposed basin. Prefer pre-authored state switching/data layers/sublevels over uncontrolled expensive runtime nav rebuilds if practical. World Partition is optional; use it only if map scale/workflow justifies it.

## O.22 Content validation

Automated validation must catch missing references, invalid evolution graphs, illegal Madness associations, broken HTN decompositions/repair branches, invalid faction matrices, impossible objective transitions, missing boss-map affordances, missing AI valuation metadata, invalid Lead conditions and mutator conflicts. Errors capable of producing impossible runs should block integration.

## O.23 Automated test classes

**Unit:** threat, Break, Injury rolling window, Grievous revive calculation, Madness floor, evolution graph, relic rules, RNG reproducibility.

**Planner:** legal deterministic decomposition, repair existence, no contradiction with revealed facts, premature-Apocalypse completion path.

**Simulation:** full four-bot runs covering expected victory, expected TPK, Shub, Nyarlathotep, heavy Injury/Madness, HTN repair and mixed factions.

**Network:** disconnect, AI takeover, reconnect, backfill, host migration and snapshot restore.

## O.24 Required developer tools

Seed override/share, force Elder One/Madness/faction/HTN plan, Ritual stage advance/rewind, objective/repair inspector, threat overlay, Break overlay, AI team-plan/utility traces, per-investigator perception view, host-migration trigger/snapshot inspector and headless batch runner.

## O.25 Recommended build order

1. Project/server structure, GAS, input/tags and deterministic RNG.
2. Combat core: Health/basic attack/threat/CC/Break/Downed/revive/Injury.
3. Headless common AI + four-bot harness.
4. Objective state machine, Ritual and HTN/repair.
5. Four hero resources/kits and full evolution lattices.
6. Madness/private perception/Crisis.
7. Factions, persistent ecology, director and intrusion.
8. Shub as default boss architecture test.
9. Nyarlathotep as mould-breaking architecture test.
10. Host migration with representative active run state.
11. Investigation event/Lead pipeline.
12. Final UI/VFX/audio/accessibility polish.

# Appendix P - Detailed MVP production scope and acceptance

## P.1 Product question

The MVP must answer whether traditional MOBA combat remains compelling as cooperative PvE when the opposing-team race is replaced by an escalating authored scenario, run-specific Madness and a hidden Elder One. It must be replayable on its own without relying on promises of more maps, progression grind or future live-service content.

## P.2 Mechanically complete required content

The current MVP proof set consists of the rural swamp fishing-village scenario; the Sapper, Photographer, Medium and Smuggler; Shub-Niggurath and Nyarlathotep; Local Cult, Smugglers/Bootleggers and Swamp Things; Perception, Compulsion, Dissociation and Obsession; the current representative relic/Mutator/Injury/objective sets; and the complete Ritual-to-Apocalypse/meta-investigation loop. These are current scope choices, not evidence that future catalogue counts are permanently fixed.

Each investigator is mechanically complete only when Basic/Passive/Q/W/E/R, bespoke resource, full evolution lattices, Madness interaction, threat/Break interaction, AI support, controller support, HUD, replication and snapshot state all work.

Each enemy is mechanically representative when silhouette/role, attacks, AI, telegraphs, threat/Break/faction behaviour work. Final visual polish can lag behind.

## P.3 Systems that cannot be faked in the MVP

The MVP must use production-intent implementations for hero resource loops, Q/W/E/R, evolution, Madness, Injury, threat, Break, objectives, HTN/repair, Ritual, director, boss mechanics, AI, fog/private perception, networking and migration state. A scripted demo that bypasses these relationships does not validate the design.

## P.4 Placeholder-friendly areas

Investigator/enemy final models, environment dressing, final VFX/music/VO, detailed prose, archive visual styling and cosmetic UI motion can remain provisional provided gameplay readability is preserved.

## P.5 Explicitly out of scope

Do not add persistent combat-power progression, universal gold/shop/crafting economies, competitive PvP, battle passes/daily-retention systems, guild/social platforms, procedural map/narrative generation, deep faction economy/diplomacy, physical fluid simulation, underground/stacked gameplay layers or final hardware/performance targets before representative profiling.

## P.6 Milestone sequence

**Combat sandbox:** movement, basic attacks, abilities, Health, threat, CC, Break, Downed/revive in greybox.

**Investigator core:** all current MVP base kits/resources feel distinct before evolution.

**Evolution/Madness sandbox:** full Q/W/E lattice, current/floor, representative symptoms/Crisis and R interactions.

**Scenario greybox:** route hierarchy, fog, landmarks, faction zones, basin drainage/boss site.

**Objective framework:** HTN, critical path, Disruptions, Power-ups, escalation and repair.

**Ritual/director:** staged scenario pressure replaces PvP race.

**Shub:** validates standard-ish Elder One architecture.

**Nyarlathotep:** proves the architecture supports a radically different boss.

**Four-bot completion:** production AI can run full scenario without human commands.

**Multiplayer state integrity:** listen-server play, disconnect takeover, backfill and migration.

**Meta-investigation:** event tracking, Leads, evidence and post-run archive.

**Representative presentation:** one complete path reaches near-target readability/art/audio/accessibility so external testers can judge the experience rather than imagine it.

## P.7 Acceptance by system

**Combat:** feels like PvE MOBA, not action shooter; threat/Break/control are understandable; hero resources create distinct rhythms; enemy-role combinations matter more than stat inflation.

**PvE structure:** teams feel they are racing the Ritual; optional objectives create real opportunity cost; splitting/regrouping occurs naturally.

**Variation:** repeated runs create decision changes through HTN route, faction prevalence, boss, Madness, relics, optional objectives and mutators rather than cosmetic shuffle.

**Madness:** symptoms are surprising but learnable; players sometimes choose more Madness; Crisis retains agency; resonance makes one investigator unusually important without sidelining the rest.

**Injury:** burst damage has lasting cost; specific Injuries alter behaviour; Grievous stacks increase rescue risk without spectator downtime; medical objectives create triage.

**Objectives:** mechanical verbs remain distinct; narrative prose is optional; escalation changes objective execution; repair is understandable and costly.

**Ritual:** stages are mechanically distinct; early boss clues ambiguous, later clues useful; deliberate summon creates a meaningful go-now/wait decision; premature Apocalypse is worse but winnable.

**Bosses:** Shub and Nyarlathotep feel structurally different on the same basin; players improve through learning; resonance matters; unfinished Disruptions remain playable during Apocalypse.

**AI:** bots play every current hero, build adaptively, Need/Greed sensibly, split/regroup/revive, solve bosses and do not cheat on hidden information; four-bot runs complete or fail for understandable reasons.

**Multiplayer:** any human/AI mixture works; disconnect/backfill preserve state; private information is filtered; host migration restores a representative active run correctly; pings/text suffice for ordinary coordination.

**Meta-investigation:** evidence -> observation -> hypothesis -> Lead -> revelation is understandable; multiple Leads can progress in parallel; TPK can still yield knowledge; replay challenges change how familiar content is approached.

## P.8 External-tester definition of MVP complete

External testers who did not participate in design can form/receive a four-investigator party, understand the briefing, traverse the swamp, fight distinct factions, complete generated objectives, make evolution/Madness/Injury/relic choices, infer the Elder One, reach Apocalypse deliberately or naturally, fight either boss to Victory or TPK, understand the consequence report and receive meaningful investigation progress. Afterward they can state concrete reasons they would approach another run differently.

# Appendix Q - Post-110 MVP decision register

This register groups the substantial content decisions made after the original 110-question design grill. It is a traceability index, not a second specification; detailed mechanics live in Appendices K-P.

## Q.1 Core resolved system changes

- Removed permanent individual Incapacitation. Repeated trauma becomes Grievous Injury; Grievous stacks make revival progressively slower on a nonlinear curve.
- Premature Apocalypse uses fail-forward conversion: unfinished mandatory objectives become harder boss-active versions.
- Ritual has five stages: Incipient, Stirring, Intrusion, Convergence, Apocalypse. Deliberate summoning jumps directly to Apocalypse.
- Gameplay-first narrative rule: objective mechanics remain fully understandable if flavor prose and speech/thought bubbles are ignored.

## Q.2 MVP scenario decisions

- Rural fishing village embedded in a large swamp; small village centre, scattered huts, isolated church and lighthouse, multiple swamp zones.
- Single top-down gameplay plane. The pre-human ritual structure is submerged and revealed by draining the basin, not located in an underground level.
- Causeway/boardwalk vs shallow-swamp/reed route hierarchy, with deep water/mud shaping paths.
- Current compulsory HTN variants: Missing Fisherman / Impossible Catch; Bell Network / Waterworks; Main Pump / Emergency Sluices; Counter-Sigil / Ritual Components.
- Current Disruptions: Bell Sequence (no false tones), Counter-Ritualist, Marsh Idols. Late Idols buff nearby enemies.
- Current Power-up templates: Smuggler Cache, Lost Curio, Open Surgery, Rescue Doctor, Lighthouse.
- Food is authored world sustain: auto-pickup when injured, Heal-over-Time, no generic enemy Health drops.
- Lighthouse is optional vision over most of the map and is not part of the boss contract.
- Drained basin is structured around central pre-human geometry, outer sites, channels/ribs, entrances and open combat space.

## Q.3 MVP factions and enemies

- Native factions: Local Cult, Smugglers/Bootleggers, Swamp Things.
- Cult identity: coordination/channels/protection; Zealot, Acolyte, Ritualist, Enforcer, Cult Leader.
- Smuggler identity: ranged formation/focus fire/area denial; Gunman, Bruiser, Lookout, Bomber, Gang Boss.
- Swamp Thing identity: ambush/isolation/movement disruption; Crawler, Lurker, Spitter, Grasper, Old Thing.
- Cult/Smugglers neutral-transactional; Cult/Swamp Things hostile but exploitative; Smugglers/Swamp Things hostile.
- Primary faction means greater prevalence/territory/encounter weighting, not exclusive unit variants.
- Corpse gameplay state persists independently of cosmetic ragdolls.

## Q.4 MVP investigators

- Current four: Trench Raider/Sapper (Vanguard/Controller), Expedition Photographer (Striker/Support), Stage Medium (Controller/Support), Bare-Knuckle Smuggler (Striker/Vanguard).
- Sapper resource Prepared Charges replenished from human-enemy components; core tools Satchel/Suppression/Tripwire; R Dead Ground delays legitimate trap effects onto tagged enemies.
- Photographer uses per-target Exposure; Frame/Flash/Develop; Develop's information branch applies a type-wide debuff; R Impossible Photograph.
- Medium uses per-spirit Attention; Bind/Beckon/Intercession; R Open Seance.
- Smuggler uses Momentum; Clinch/Shoulder Through/Dig In; Make It Personal adds both threat and target-specific damage; R Drowned Man Walking.
- Full evolution lattices for all current Q/W/E abilities are detailed in Appendix K.

## Q.5 MVP Madness and relics

- Current Madnesses: Perception, Compulsion, Dissociation, Obsession.
- Perception = additional subjective reality layer; Crisis makes it dominant; Nyarlathotep resonance identifies true avatars.
- Compulsion = contextual temptations/urges with no control theft; Crisis presents several at once.
- Dissociation = predictable delayed echoes; Crisis creates an echo storm.
- Obsession = contextual fixation; Crisis creates a singular overwhelming fixation; Shub resonance privileges true reproductive nodes.
- Current relic mechanics: Officer's Swagger, Cracked Saint's Medal, Hangman's Knot, overheal-to-decaying-Shield relic, Soldier's Morphine Tin, Black Glass Rosary, Ferryman's Coin, Saint's Work Gloves.

## Q.6 Shub-Niggurath

- Standard-ish phased/vulnerability boss built around permanent corruption, Broodling multiplication and genuine reproductive nodes.
- Corruption starts in boss basin and unfinished Disruption sites, spreads along Shub's movement/from nodes, never recedes during Apocalypse, causes Madness pressure to investigators and gives Shub-linked enemies damage resistance.
- Broodlings consume corpse value, split interruptibly into two partially injured offspring, and fresh offspring heal unless damaged; Broodling corpses are worth only a fraction of standard corpse value.
- Spawn of the Black Goat is a charge/displacement bruiser, not a summoner.
- Core attacks: Trampling Advance, Black Milk, Call the Brood, Horned Sweep.

## Q.7 Nyarlathotep

- Mould-breaking avatar hunt: several manifestations, one true, false ones deal full damage and vanish after modest testing damage; true avatar advances the fight.
- Perception-resonant investigator privately sees true avatar.
- Borrowed Face is an exact presentation copy of an investigator; no bespoke distorted duplicate kit.
- Crossing Paths uses damaging telegraphed dash lines, repositions avatars and temporarily disables resonant true sight afterward.
- Unwelcome Attention makes all avatars focus one marked investigator with resonant-player bias.
- Black Tongue spikes Madness and triggers one valid symptom from the target's existing Madness.
- Final direct manifestation is short/conventional with Black Tongue, Crossing Blow and Many Hands.
- False Man replaces an ordinary faction unit, with subtle under-light as sole deliberate pre-reveal tell; at low Health transforms invulnerably while healing/telegraphing a radius attack, then becomes vulnerable again.
- Winged Hunter is a small hit-and-run flyer that crash-lands if struck by melee during the committed dive window.

## Q.8 Meta-investigation content direction

- Current MVP investigation is content-anchored: Elder One, scenario and investigator threads; boss/scenario threads span Discovery/Directed/Mastery while character threads use Discovery/Directed.
- Shub Discovery Lead accepted as Find the True Growth.
- Detailed proposed Lead set and Evidence -> Observation -> Hypothesis -> Lead -> Revelation flow are preserved in the investigation specification and should remain subject to content review rather than numeric quota locking.

# Appendix R - Design rationale and comparable-system notes

This appendix preserves design reasoning that would otherwise disappear from the canonical rules. It is not a source of mechanics by itself and does not override the locked/reviewed specification.

## R.1 Evercore Heroes

The project takes the high-level lesson that MOBA-style hero combat can support objective-driven PvE, while deliberately removing the simultaneous PvP race. The replacement pressure is the Ritual: an authored escalating world state that creates urgency, rotations and prioritisation without another player team.

## R.2 Cthulhu: Death May Die

The useful high-level inspiration is the structure of an overarching ritual/escalation that changes the board and culminates in an Elder One confrontation. This project does not copy scenario text, components, visual expression or proprietary content. Its objective generation, Madness-resonance system, hero progression and real-time MOBA combat are original design work.

## R.3 Investigation-system research

Research into investigation games produced several principles now encoded in the meta layer:

- **Outer Wilds:** automatic organisation of discovered knowledge is preferable to manual corkboard busywork.
- **Deathloop:** vague discoveries can mature into explicit actionable Leads.
- **Hitman:** replay challenges work best when they ask players to approach familiar systemic content differently rather than merely accumulate kills.
- **Tunic:** knowledge can reveal possibilities that were mechanically available before the player knew about them; this motivated retroactive Lead completion.
- **Alan Wake 2 / explicit case-board systems:** manually placing predetermined clues can become busywork; filing/obvious relationships should be automated.
- **Deduction-heavy games such as Obra Dinn / Golden Idol / Chants of Sennaar:** useful lesson is that evidence should support conclusions, but this project deliberately avoids turning the between-run layer into a separate full detective puzzle.

## R.4 Why the meta investigation is lightweight

The scenario is the primary game. The archive exists to make replay purposeful, deepen narrative and direct players toward interesting challenge conditions. It should not demand a second long play session between missions. Hence the rule: automatic filing, optional reading, explicit Leads once understood, many parallel opportunities and no combat-power rewards.


# Appendix S - Detailed MVP meta-investigation and Leads

This appendix restores the reviewed meta-investigation specification in full. Exact content counts and challenge thresholds are not canonical; the listed Leads are the current authored MVP set/direction and can be tuned during implementation.

## S.1 Purpose

The meta-investigation provides long-term narrative progression between self-contained scenarios. It does not grant permanent combat power. Investigators begin each scenario from their normal baseline; combat build development occurs during the run through evolution, Madness and relics.

Successful runs, failed expeditions and unusual events can reveal journals, letters, photographs, clippings, artefacts, witness statements, field notes and similar diegetic material. These discoveries expose connections between incidents, investigators, cults and Elder Ones and give players concrete reasons to replay familiar content differently.

The intended loop is:

**Play scenario -> observe unusual events -> satisfy Leads -> recover new evidence -> interpret investigation -> choose what to pursue next -> replay under different conditions.**

## S.2 Design principles

### Knowledge, not power

Investigation progression does not increase starting Health, damage, equipment quality or permanent hero strength. Experienced players gain context, knowledge, challenge opportunities and narrative access rather than vertical combat advantage.

### Gameplay remains primary

A player can ignore every journal, speech/thought bubble and archive entry and still understand objectives, threats, boss mechanics and victory conditions.

### Evidence reveals possibilities that already exist

A Lead does not make a challenge possible. If a player performs the relevant feat before learning that it matters, the game records the qualifying event and can recognise it retroactively once the Lead becomes available.

### Many Leads remain viable in parallel

The archive should normally expose several plausible goals across different bosses, scenarios, investigators, objectives, Ritual states, Madness/Injury conditions and mutators. Random run generation should rarely create a mission with no reasonable chance of meta progress.

## S.3 Evidence model

The archive automatically organises evidence. Players are not required to drag clues to predetermined corkboard positions or manually create obvious links.

**Evidence:** raw document/object/event record.

**Observation:** a potentially significant fact extracted from evidence.

**Hypothesis:** an interpretation supported by enough related evidence.

**Lead:** a clear machine-verifiable gameplay condition once the hypothesis is sufficiently understood.

**Revelation:** new diegetic material/meta-narrative unlocked by completing the Lead.

The UI should visually distinguish observed fact, interpretation, hypothesis, confirmed conclusion and actionable Lead so uncertain information is not presented as omniscient truth.

## S.4 Lead categories and difficulty

Leads are anchored primarily to **Elder Ones**, **scenarios**, or **investigators**, then cross-reference systems such as mutators, objectives, Madness, Injury or unusual run states.

**Discovery:** likely to occur through attentive normal play; introduces a mechanic/thread.

**Directed:** requires deliberate setup or a changed approach.

**Mastery:** tests strong understanding/execution and belongs primarily to boss/scenario threads rather than forcing extreme character-specific feats.

## S.5 Shub-Niggurath investigation thread

### Discovery - Find the True Growth

**Condition direction:** destroy a genuine reproductive node identified through the Obsession-resonant investigator during a Shub encounter.

**Purpose:** establishes that apparent growths are not equivalent and that the resonant investigator's fixation reveals something objectively significant.

**Possible revelation:** a prior field account records witnesses becoming irrationally obsessed with particular growths immediately before those growths proved central to manifestation.

### Directed - Starve the Brood

**Condition direction:** complete a successful Shub encounter while preventing meaningful Broodling corpse-fed multiplication, potentially under an appropriate challenge state.

**Purpose:** teaches that Brood reproduction consumes the consequences of violence around it rather than occurring spontaneously.

**Possible revelation:** notes from an earlier expedition describe markedly fewer offspring when corpses were denied or feeding was repeatedly interrupted.

### Mastery - Black Ground

**Condition direction:** defeat Shub after allowing substantial corruption to accumulate across the boss area and/or unresolved Disruption locations.

**Purpose:** tests mastery of displacement, positioning, growth-node control and permanent arena deterioration.

**Possible revelation:** a survivor's account suggests Shub becomes strongest where manifestation has physically rewritten the land.

## S.6 Nyarlathotep investigation thread

### Discovery - The True Face

**Condition direction:** correctly identify and destroy a genuine Nyarlathotep avatar.

**Purpose:** introduces the distinction between apparent and meaningful manifestations.

**Possible revelation:** an old photograph contains several figures but only one casts an impossible shadow.

### Directed - Follow the Crossing

**Condition direction:** successfully track and destroy the true avatar after Crossing Paths while the resonant investigator's supernatural certainty is temporarily unavailable.

**Purpose:** requires the party to learn visual tracking rather than depend entirely on true sight.

**Possible revelation:** a witness describes the entity deliberately confusing those who could normally recognise it.

### Mastery - No More Masks

**Condition direction:** complete the avatar hunt with minimal wasted effort against false manifestations, optionally in combination with an appropriate mutator/challenge state.

**Purpose:** tests mastery of the deception loop rather than raw boss damage.

**Possible revelation:** evidence suggests the avatars are disposable manifestations of a larger identity rather than ordinary illusions.

## S.7 Swamp-village investigation thread

### Discovery - Beneath the Water

**Condition direction:** expose the pre-human ritual structure through either valid drainage route.

**Purpose:** introduces the location's central mystery.

**Possible revelation:** historical evidence indicates the village was built long after the structure had already disappeared beneath the marsh.

### Directed - Let It Ring

**Condition direction:** complete the Bell Sequence after deliberately allowing it to escalate into a later, more demanding form.

**Purpose:** encourages players to experience objective escalation rather than always resolving the same early version.

**Possible revelation:** local accounts suggest the bell pattern predates the church that now performs it.

### Mastery - Too Late to Stop It

**Condition direction:** win after the Ritual reaches Apocalypse naturally before all required Disruptions are resolved, forcing at least one into its boss-active Apocalypse form.

**Purpose:** tests mastery of the fail-forward scenario architecture.

**Possible revelation:** recovered case material establishes that manifestation does not mean interference has become impossible; ritual work can continue after the Elder One arrives.

## S.8 Sapper investigation thread

### Discovery - Prepared Ground

**Condition direction:** cause several enemies to be meaningfully affected by prepared Sapper battlefield tools during one encounter.

**Purpose:** introduces the hero's defining battlefield-planning identity.

**Possible revelation:** wartime notes explain where the Sapper learned to think of territory as something constructed rather than merely occupied.

### Directed - Dead Ground

**Condition direction:** use Dead Ground so several legitimate trap triggers are delayed and resolve together against tagged enemies.

**Purpose:** connects military planning to impossible causality as the hero descends into Madness.

**Possible revelation:** later notebook pages describe battlefields that no longer correspond to physical maps.

## S.9 Photographer investigation thread

### Discovery - Clear Evidence

**Condition direction:** use Develop to turn study of one specimen into an advantage against other enemies of the same type.

**Purpose:** reinforces the Photographer's principle that observation of one subject can reveal truths about the category.

**Possible revelation:** annotated photographs demonstrate shared anatomy/behaviour among creatures previously thought unrelated.

### Directed - Impossible Photograph

**Condition direction:** use Impossible Photograph during a major Mythos manifestation and successfully exploit the resulting Exposure state.

**Purpose:** connects the camera to the question of whether photography records objective reality or only one accessible layer.

**Possible revelation:** the developed plate contains information the Photographer did not consciously see when it was taken.

## S.10 Medium investigation thread

### Discovery - Thin Places

**Condition direction:** build substantial Spirit Attention around deaths, a Downed investigator or active ritual phenomena and convert it through Intercession.

**Purpose:** establishes that the Medium does not simply summon the dead; supernatural/mortal events attract usable presence.

**Possible revelation:** personal notes suggest the dead are already present and the Medium's talent is noticing when the boundary becomes thin.

### Directed - Open Seance

**Condition direction:** use Open Seance while several spirits are already active and meaningfully exploit their full manifestation.

**Purpose:** marks the transition from controlled mediumship to allowing the dead direct access to the world.

**Possible revelation:** later testimony questions whether every entity that answered was ever human.

## S.11 Smuggler investigation thread

### Discovery - Make It Personal

**Condition direction:** use Clinch/Make It Personal to deliberately pull a dangerous enemy's attention onto the Smuggler and defeat it while sustaining that personal engagement.

**Purpose:** establishes the Smuggler's willingness to solve threats physically and personally.

**Possible revelation:** a dockside account recounts the drowning incident and the disbelief of witnesses who expected him to be dead.

### Directed - Drowned Man Walking

**Condition direction:** use Drowned Man Walking while maintaining sustained close engagement through a dangerous encounter.

**Purpose:** exposes more of the supernatural consequence of the supposed drowning.

**Possible revelation:** witness testimony indicates the person recovered from the water did not behave entirely like the man who went in.

## S.12 Credit rules

Every Lead explicitly declares **Team** or **Personal** scope.

Team conditions give credit to every eligible player who has the Lead; no killing blow, last hit or final interaction ownership is required.

Personal conditions explicitly depend on that player's investigator, Madness/Injury state or action.

Different players can progress different personal investigation threads in the same run.

## S.13 Failed runs

Knowledge survives failure. A condition fulfilled before a later TPK remains fulfilled unless the Lead explicitly requires victory. Rare manifestations, Madness interactions, faction events and intermediate objectives can therefore make a failed expedition narratively productive.

## S.14 During-run and post-run presentation

During play, Lead completion produces only a small notification. Narrative exposition is deferred.

Post-run flow:

1. clear Victory/Failure result;
2. flavour consequence list;
3. Lead completion/progress;
4. return to investigation layer;
5. newly available evidence/revelations.

## S.15 Personal briefing shortlist

Each player receives a short personal shortlist of Leads relevant to the chosen scenario, investigator, mutators and current investigation state. It is advisory rather than exclusive; other active Leads can still complete.

## S.16 Anti-patterns

- No manual filing/corkboard busywork for relationships the game already knows.
- No required prose parsing during missions.
- No narrow RNG gates that routinely create dud runs with no possible progress.
- No grind Leads based primarily on cumulative kill counts/routine repetition.
- No permanently hidden arbitrary conditions once a hypothesis has matured into an actionable Lead.
- No combat-power rewards from the investigation tree.

## S.17 Validation goals

The investigation succeeds when players understand that evidence reveals challenge opportunities, can ignore the archive without impairing scenario comprehension, can use clues to choose productive goals, can progress several Leads in one run, can still gain knowledge from TPKs, recognise Discovery/Directed/Mastery intent, benefit from retroactive completion, understand Team/Personal credit, and find that Lead pursuit changes how they approach familiar gameplay rather than merely asking them to repeat it.
