# Run progression evidence protocol

GDD 4.5 and Appendix K govern. Numeric tuning here is provisional, not approved balance.
Cumulative XP gives a level each 100 XP, capped at level 13; levels 2/4/6/8/10/12 award six bankable major evolution opportunities.
Every level adds 1.5% of base basic-attack damage. R stays available through its existing Madness mechanics.
Accepted enemy deaths award each investigator 5 XP, occupation completion 500, encounter victory 650.
Significant encounters dominate rewards; objective generation is not implemented. Future accepted objective completions may call the authority-only Award API with a unique objective-instance identifier.
Repeated event identifiers never award again; downed investigators share team progression. Possession changes retain the component and its reward ledger.
This is live run state, not a host-migration save format.

Verification: foundation cadence/overflow/bankability test, Unreal Test and Smoke; editor tests added alongside evolution choice integration.
