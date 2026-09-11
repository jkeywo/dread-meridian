#include "DMShellState.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMShellStateTest, "DreadMeridian.Foundation.ShellState", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMShellStateTest::RunTest(const FString& Parameters)
{
    FDMShellState S;
    TestTrue(TEXT("Shell opens on the main menu"), S.Phase == EDMShellPhase::MainMenu);
    TestTrue(TEXT("Main menu accepts input"), S.AcceptsInput());
    TestFalse(TEXT("Main menu is not the mission"), S.ShowsMission());

    TestTrue(TEXT("Launching before the lobby does nothing"), S.Launch() == EDMShellAction::None);
    TestTrue(TEXT("A rejected request leaves the phase alone"), S.Phase == EDMShellPhase::MainMenu);

    TestTrue(TEXT("Start needs no side effect"), S.Start() == EDMShellAction::None);
    TestTrue(TEXT("Start opens the lobby"), S.Phase == EDMShellPhase::Lobby);
    TestTrue(TEXT("Start is not repeatable"), S.Start() == EDMShellAction::None && S.Phase == EDMShellPhase::Lobby);
    TestTrue(TEXT("Quitting is only offered by the main menu"), S.RequestQuit() == EDMShellAction::None);

    TestTrue(TEXT("Launch streams the mission"), S.Launch() == EDMShellAction::StreamMission);
    TestTrue(TEXT("Launch shows loading"), S.Phase == EDMShellPhase::Loading);
    TestFalse(TEXT("Loading ignores pointer input"), S.AcceptsInput());
    TestTrue(TEXT("A second launch cannot stream twice"), S.Launch() == EDMShellAction::None);
    TestTrue(TEXT("Completion before the mission starts is rejected"), S.Complete(true) == EDMShellAction::None);

    TestTrue(TEXT("A shown sublevel begins the encounter"), S.MissionReady() == EDMShellAction::BeginEncounter);
    TestTrue(TEXT("The mission owns the screen"), S.ShowsMission());
    TestFalse(TEXT("The mission ignores shell input"), S.AcceptsInput());
    TestTrue(TEXT("A repeated streaming callback cannot spawn a second roster"), S.MissionReady() == EDMShellAction::None);
    TestTrue(TEXT("Dismiss does nothing mid-mission"), S.Dismiss() == EDMShellAction::None && S.ShowsMission());

    TestTrue(TEXT("Defeat needs no side effect"), S.Complete(false) == EDMShellAction::None);
    TestTrue(TEXT("A finished run shows the case report"), S.Phase == EDMShellPhase::CaseReport);
    TestFalse(TEXT("The case report carries the real outcome"), S.bVictory);
    TestTrue(TEXT("Completion cannot be reported twice"), S.Complete(true) == EDMShellAction::None);
    TestFalse(TEXT("A rejected completion cannot rewrite the outcome"), S.bVictory);

    TestTrue(TEXT("Dismissing the case report reopens the shell"), S.Dismiss() == EDMShellAction::RestartShell);
    TestTrue(TEXT("The case report stays up until the level reopens"), S.Phase == EDMShellPhase::CaseReport);

    FDMShellState Won;
    Won.Start(); Won.Launch(); Won.MissionReady();
    TestTrue(TEXT("Victory reaches the case report"), Won.Complete(true) == EDMShellAction::None);
    TestTrue(TEXT("Victory is recorded"), Won.bVictory && Won.Phase == EDMShellPhase::CaseReport);

    FDMShellState Back;
    Back.Start();
    TestTrue(TEXT("Lobby Back needs no side effect"), Back.Dismiss() == EDMShellAction::None);
    TestTrue(TEXT("Lobby Back returns to the main menu"), Back.Phase == EDMShellPhase::MainMenu);
    TestTrue(TEXT("The main menu can quit"), Back.RequestQuit() == EDMShellAction::Quit);
    return true;
}
#endif
