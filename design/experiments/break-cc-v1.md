# Break/CC v1 verification protocol

Scope: implement GDD 4.4 and O.6 in the existing combat sandbox. The GDD's statuses
are unchanged. Settings and the pre-hit protection ordering are provisional
implementation choices documented in `docs/break-cc.md`.

This intentionally changes combat outcomes (`combat_rules_version=break-cc-v1`,
capture version `0.4.0`). Do not compare old AI tuning scores as if the gameplay
rules were unchanged. Named RNG streams are untouched. This is not a balance
experiment or a deterministic gameplay replay claim.

Required gates, using Unreal 5.8.2 through `tools/Unreal.ps1`:

1. `-Action Test`: pure Resolve/data boundaries plus all foundation regressions.
2. `-Action EditorTest`: production kit and Break APIs in live PIE worlds.
   `DreadMeridian.Editor.Break.Control` isolates actors from autonomous decisions,
   then tests protected control, depletion, Broken, clinch expiry, recovery
   resistance, independent interrupts, common stun/root, released fire and death.
3. `-Action Smoke`: original harness lifecycle and source provenance.
4. `-Action NetworkTest`: two clients and a handoff, comparing final public
   Resolve/control projections alongside health, resources and kits. This final
   snapshot check alone does not verify every intermediate Broken transition.
5. `-Action SmugglerSmoke`: production four-bot encounter with real elite Resolve.
   Outcome is observed, not required to improve over earlier tuning.
6. `python -m unittest discover -s tests -v`; validate the fresh foundation log
   with `tools/validate_capture.py <capture> --require-provenance`, and the fresh
   combat log through Play Trace's `validate-telemetry` command. Inspect the latter
   for version, omissions and real pressure/Break/recovery observations.

Reports and regenerable logs belong in ignored `Saved/`. Actual Mythos bosses,
relics/evolutions, full replay and migration remain outside this verification.

The first captured smuggler run exposed missing shield-gain observations in the
existing kit capture path. Record accepted `combat.shield_gained` transitions and
validate their arithmetic with the reusable Play Trace validator (synthetic
fixtures only in that repository). Do not accept a damage-state mismatch or
retrofit missing gains into an already captured log; generate a fresh capture.
