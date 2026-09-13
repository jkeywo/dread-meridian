# Run progression evidence protocol

GDD 4.5 and Appendix K govern. Numeric tuning here is provisional, not approved balance.
Cumulative XP gives a level each 100 XP, capped at level 13; levels 2/4/6/8/10/12 award six bankable major evolution opportunities.
Every level adds 1.5% of base basic-attack damage. R stays available through its existing Madness mechanics.
Accepted enemy deaths award each investigator 5 XP, occupation completion 500, encounter victory 650.
Significant encounters dominate rewards; objective generation is not implemented. Future accepted objective completions may call the authority-only Award API with a unique objective-instance identifier.
Repeated event identifiers never award again; downed investigators share team progression. Possession changes retain the component and its reward ledger.
This is live run state, not a host-migration save format.

Verification: foundation cadence/overflow/bankability test, Unreal Test and Smoke; editor tests added alongside evolution choice integration.

Evolution framework: Q/W/E nodes are publicly replicated; server validates earned token, predecessor, slot, node, ownership and living pawn. R has no normal-level branch.
U / gamepad View opens the selector; Tab / RB cycles, Space / A (or left-click) accepts, U / B closes. Combat actions are suppressed while selecting; movement remains available.
Catalog contains 72 unique stable IDs including 12 bases and 60 evolved nodes, semantic GDD descriptions, and situational offense/control/protection valuation hooks. Bots evaluate nearby enemies, elites and team danger.
This framework commit exposes choices; evolved runtime effects follow as separate investigator commits. No claim of controller parity or host-migration restoration.

Madness integration: accepted first-tier choices add 5 to the existing floor; second-tier choices add 10 (45 for all six). These are provisional. Other sources' floor contributions remain intact. Rejected or repeated requests do not charge; recovery cannot remove this contribution. The selector exposes each cost. Private floor stays on MadnessCore's owner-only view, outside the public build projection.
PIE verifies accepted/rejected costs, both hybrid routes, full-build recovery floor and retained build through control handoff.
