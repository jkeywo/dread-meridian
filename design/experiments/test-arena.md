# Configurable test arena validation protocol

Purpose: isolate implemented investigator and enemy behavior with a chosen local
party and reproducible authored placement. All harness values remain provisional.
The arena is developer evidence, not a mission, migration save, or gameplay replay.

1. Build/generate with Unreal 5.8.2 using `tools/Unreal.ps1 -Action GenerateArena`.
2. Run the foundation, smoke, arena smoke, and arena PIE checks listed in
   `docs/test-arena.md`. Run existing enemy-encounter and character-picker PIE
   checks after modifying shared roster assignment.
3. Run the rendered arena probe. Inspect the initial setup, paused reinforcement,
   and restored four-person setup screenshots. Check the entire arena is framed,
   panel labels do not overlap, and placement previews distinguish invalid ground.
4. For manual feel checks, choose each investigator, test one enemy of each type,
   then test presets and mixed custom groups. Test signatures near cover and reeds.
5. Pause during a telegraph; add an enemy, resume, finish or wipe, and reset.
   Repeat with 1, 2, 3, and 4 investigators. Verify fresh resources and no old
   hazards, corpses, targets, or AI orders after reload.
6. Validate captures from a new launcher-driven arena probe with
   `tools/validate_capture.py --require-provenance`. Keep transient captures,
   reports, logs, and screenshots under ignored `Saved/`.

Acceptance: no partial invalid placements, no combat advancement under pause,
correct human possession and AI companions, clean configuration-preserving reloads,
and unchanged fixed encounter behavior in existing missions. Record build or test
limitations honestly rather than relaxing unsupported checks.
