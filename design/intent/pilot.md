# Dread Meridian pilot intent

Source: master GDD v0.3, especially sections 1, 13, 16 and Appendix O.
This file curates implementation intent; it does not alter the source's authority.

- Preserve the four-investigator expedition across 0-4 human control for testing.
- Make accepted authoritative gameplay actions observable as structured events.
- Separate seeded gameplay state, public replication and subjective information.
- Keep canonical design and native Unreal assets in this repository.
- Keep a Play Trace assessment explicitly tied to source revisions, seed,
  build/engine identity, configuration and known unsupported semantics.
- Use simulation and telemetry as evidence for human design judgments.

Current hypothesis: a narrow Unreal authority -> event capture -> validated local
artifact path can support Play Trace adapter development without adding a runtime
service dependency to the game. The foundation smoke test exercises this path.
It provides no evidence about MOBA feel, difficulty, session length or balance.
