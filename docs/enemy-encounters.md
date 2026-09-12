# Sandbox encounters

Interactive play contains three camps of three smugglers and a two-person patrol.
Camps sit south, east and north of the central route; the squad starts west.
Camp signs, supply crates and a dashed patrol circuit mark the layout.

Clear all eleven occupation enemies to trigger a three-second arrival warning.
The Gang Boss then enters from the east with a Gunman, Bruiser, Lookout and Bomber.
All five must fall for victory; killing only the Gang Boss is insufficient.
A squad wipe at any stage is defeat. Dead enemies do not respawn.

The utility AI handles sight, group alerts, pursuit, retreat, target selection
and positioning. Tuning lives in role profiles; see [AI tuning](ai-tuning.md).
Companions follow the player, rescue allies, avoid hazards and answer [pings](pings.md).
The Gang Boss uses the protected [Break/Resolve layer](break-cc.md); ordinary
smugglers receive full control. Interrupting a pending firebomb prevents its
release; interrupting or restraining the caster after release preserves its fire.

These are provisional sandbox encounters, not the Mythos boss system or encounter
director. Explicit network and smoke probes keep their compact three-enemy
fixture. The normal PIE map uses the full encounter layout. The original encounter
design is recorded in [smuggler sandbox](../design/encounters/smuggler-sandbox.md);
its historical implementation omissions predate the current Break/CC work.
