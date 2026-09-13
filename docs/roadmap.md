# Next implementation slices

These are AI-origin implementation proposals. They sequence the existing GDD;
they do not replace its requirements or define a finished MVP.

Implemented starter slice: native arena, GAS basic attack/Health/Shield, downing
and rescue, threat-based companions, four persistent investigator slots,
human/bot handoffs, Enhanced Input mappings and local Play Trace source/telemetry
ingestion. All four investigators now have Basic, Q and their named W/E/R at the
A nodes, used by humans and companions alike; see [named kits](kits.md). The rows
below retain the full GDD acceptance scope. Ability evolutions, Madness families/resonance, and
scenario-aware team planning remain outstanding. Break/CC now uses configurable
Resolve, bounded control windows and recovery resistance; see [Break/CC](break-cc.md).
[Injuries](injuries.md) now cover burst/down triggers, six effects, Grievous and finite
recovery supplies. Objective-earned medical resources and authored food placement remain open.
[Madness core](madness-core.md) now covers current/floor, thresholds, timed Crisis,
grounding and private owner delivery. The next content slice can build one complete
family on those APIs; assignment/resonance still depends on actual boss selection.

| Slice | Deliverable | Acceptance |
|---|---|---|
| 1. Investigator sandbox | Authored greybox map, GAS investigator/attributes, replicated movement, Enhanced Input mouse/keyboard and controller paths, one basic attack and target dummy | Two clients see the same authoritative damage; both input devices can move/target/attack; public state excludes hidden data; events identify stable actor/ability IDs |
| 2. Combat survival | Health/Shield, threat, Break/CC, Downed/revive and Injury rules from GDD K/M/O | Rules tests cover edge cases; seed/input replay agrees within the chosen simulation boundary; no frame-time-dependent Injury windows |
| 3. Four-person harness | Four actual investigators, production companion execution and team decisions, 0-4 human ownership | Four bots use the same gameplay APIs as humans; disconnect/reconnect transfers control; headless tests include real victory and TPK |
| 4. Scenario pressure | Objective states, ritual time source, deterministic HTN generation and visible repair | Same content/seed yields same plan; natural Apocalypse converts outstanding mandatory tasks and remains completable |
| 5. Hidden-state and migration spikes | Subjective entitlement plus versioned full run snapshot/restore | Unauthorized client/AI cannot inspect hidden state; resume preserves RNG and active combat/objective state; actual host loss can reconnect |
| 6. Representative content | Investigator kits, Madness, factions/director, Shub and Nyarlathotep, investigation Leads | GDD's per-system acceptance passes; both boss-map contracts validated; no placeholder counted as a complete mechanic |

The first reusable Play Trace Git-source and telemetry adapters now exist, with
synthetic fixtures in Play Trace and this game's actual observations only here.
Extend semantic coverage alongside gameplay. Do not begin balance comparisons until the same production
gameplay and AI run headlessly, with complete provenance and explicit omissions.

The earliest useful experiment is whether two input paths and companion control
produce equivalent authoritative verbs. Ritual pressure/rotation experiments
become meaningful only once objectives, travel and production bots exist.
