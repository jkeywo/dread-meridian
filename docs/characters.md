# Investigator basics

Names and rules follow GDD Appendix K. Exact numbers below are provisional sandbox tuning.
Basic, Q and the named W/E/R are available; see [abilities](abilities.md) and [named kits](kits.md). Evolutions remain deferred. See [combat presentation](combat-presentation.md) for investigator skins, held/stowed gear and animation.

| Investigator | Basic attack | Resource and passive |
|---|---|---|
| Trench Raider / Sapper | Carbine: 18 damage, 550 range, 0.9 seconds; +15% versus Suppressed targets | Prepared Charges: 2/3 initially, two components restore one. Scrounger auto-collects within 160 units. Human raiders drop one component; non-human enemies do not. Charges are not consumed by basics. |
| Expedition Photographer | Precision rifle: 30 damage, 850 range, 1.8 seconds; scales up to double with per-target Exposure | Perfect Moment multiplies Exposure gains by 1.5 during visible commitments. Exposure decays after two seconds disengaged. Frame supplies Exposure; Flashbulb remains deferred. Basics do not create Exposure. |
| Stage Medium | Spirit Lash: 12 damage, 500 range, 1.1 seconds; +12 Attention to spirits bound to that enemy | Attention is per spirit. Thin Places amplifies nearby death/down/ritual/Madness inputs; Q Bind Spirit creates ally, enemy or ground bindings. |
| Bare-Knuckle Smuggler | Melee: 10 damage, 155 range, 0.5 seconds; repeating three-hit cadence, small common-enemy finisher shove/stagger | Hits and incoming meaningful pressure build Momentum; decays after three seconds without pressure. Keep Your Feet grants banded slow/displacement resistance and melee stickiness. Switching target resets combo. |

Server authority owns resources and damage. Resources remain on the same replicated actor across bot/human handoff. Exposure and Attention changes are server-only and driven by the implemented Q abilities. Public HUD shows each investigator's resource and passive.

Manny/Quinn template content is copied from the installed Unreal 5.8.2 distribution into its original `/Game/Characters` package root. It remains Epic-licensed content. The supplied investigator skins share Manny's skeleton and use retargeted combat animations. Attacks and abilities emit local effects from accepted server multicasts. Cosmetic timing never controls damage.

The existing Victory/Defeat/Revive smoke profiles retain their explicitly declared range/damage/cooldown tuning to test infrastructure. Interactive and network runs use investigator defaults. These profiles do not establish character balance.

Choose your starting investigator with `tools/Unreal.ps1 -Action Play -Investigator Smuggler` (Sapper, Photographer, Medium or Smuggler). Occupied human slots are never stolen.

In Unreal Editor, use **Play as** immediately beside Play. The four-option picker remembers your project-local preference, applies it when PIE starts, and is disabled during PIE. In multiplayer PIE the preferred available investigator is assigned first; additional humans take remaining slots. This preference does not change standalone launcher selection.
