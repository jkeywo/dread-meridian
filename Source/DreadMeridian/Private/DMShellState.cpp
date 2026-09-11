#include "DMShellState.h"

EDMShellAction FDMShellState::Start()
{
    if (Phase != EDMShellPhase::MainMenu) { return EDMShellAction::None; }
    Phase = EDMShellPhase::Lobby;
    return EDMShellAction::None;
}

EDMShellAction FDMShellState::Launch()
{
    if (Phase != EDMShellPhase::Lobby) { return EDMShellAction::None; }
    Phase = EDMShellPhase::Loading;
    return EDMShellAction::StreamMission;
}

EDMShellAction FDMShellState::MissionReady()
{
    // Only a launch that is still loading may start the encounter; a late or duplicate
    // streaming callback must not spawn a second roster.
    if (Phase != EDMShellPhase::Loading) { return EDMShellAction::None; }
    Phase = EDMShellPhase::Mission;
    return EDMShellAction::BeginEncounter;
}

EDMShellAction FDMShellState::Complete(bool bInVictory)
{
    if (Phase != EDMShellPhase::Mission) { return EDMShellAction::None; }
    Phase = EDMShellPhase::CaseReport;
    bVictory = bInVictory;
    return EDMShellAction::None;
}

EDMShellAction FDMShellState::Dismiss()
{
    if (Phase == EDMShellPhase::Lobby) { Phase = EDMShellPhase::MainMenu; return EDMShellAction::None; }
    // A finished run leaves spawned combatants, pickups and markers in the persistent level, and the
    // encounter state inside ADMCombatGameMode is not resettable in place. Reopening the shell level
    // is the honest teardown until a real run-teardown path exists.
    if (Phase == EDMShellPhase::CaseReport) { return EDMShellAction::RestartShell; }
    return EDMShellAction::None;
}

EDMShellAction FDMShellState::RequestQuit()
{
    return Phase == EDMShellPhase::MainMenu ? EDMShellAction::Quit : EDMShellAction::None;
}
