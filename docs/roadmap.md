# Next implementation slices

These are AI-origin implementation proposals. They sequence the existing GDD;
they do not replace its requirements or define a finished MVP.

Implemented starter slice: native arena, GAS basic attack/Health/Shield, downing
and rescue, threat-based companions, four persistent investigator slots,
human/bot handoffs, Enhanced Input mappings and local Play Trace source/telemetry
ingestion. The rows below retain the full GDD acceptance scope. Named kits,
burst/named Injuries, Break/CC and scenario-aware team planning remain outstanding.

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
