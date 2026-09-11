# Smuggler presentation

All five native `SKM_<Role>` assets under `/Game/DreadMeridian/Characters/Enemies/Smugglers` are selected by replicated faction role. Weapons are held at their authored grip anchors; Lookout binoculars and Bomber satchel remain holstered.

Each role has idle, jog, start, stop and left/right turn variants with the supplied equipped arm/finger pose baked onto existing Manny-compatible movement. This is provisional pose baking, not runtime two-hand IK. Imported concept models and precise finger contact still need production polish.

| Role | Basic attack | Signature presentation |
|---|---|---|
| Gunman | Rifle fire, muzzle flash and tracer | Equipped rifle stance; existing set-position status |
| Bruiser | Retargeted right hook adapted to the truncheon | Brace windup; strike and dust burst on confirmed shove |
| Lookout | Retargeted one-handed gun clip, flash and tracer | Calling gesture and contracting red mark |
| Bomber | Grenade throw and arcing projectile | Firebomb arc, existing Niagara explosion, persistent embers inside the burning area |
| Gang Boss | Rifle recoil with three cosmetic SMG tracers | Retargeted machine-gun command gesture and converging red focus cue |

All models share existing hit reaction and death clips. Cosmetic events never apply damage, movement, resources or cooldowns. Burning visuals follow the replicated marker lifetime and end with it.

New source clips are cherry-picked from HoldingProject: FightingAnimsetPro `KB_Gun`, `KB_p_Hook_R`; OpenWorldAnimset `MG_gestures1`. `tools/prepare_smuggler_presentation.py` retargets these and builds the 30 equipped locomotion variants; source/output mappings are in `smuggler-animation-validation.json`. Existing GameAnimationSample start/stop/turn retargets and Explosions_W3Vol1 Niagara explosion are reused.
