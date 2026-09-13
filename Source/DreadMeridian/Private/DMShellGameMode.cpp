#include "DMShellGameMode.h"
#include "DMShellHUD.h"
#include "DMShellPlayerController.h"
#include "DMGameState.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"

ADMShellGameMode::ADMShellGameMode()
{
    PlayerControllerClass = ADMShellPlayerController::StaticClass();
    HUDClass = ADMShellHUD::StaticClass();
    bFishingVillage=true;
    MissionLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/DreadMeridian/Maps/L_FishingVillage")));
}

void ADMShellGameMode::StartPlay()
{
    Super::StartPlay();
    PublishPhase();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("DMShellProbe"))) { StartProbe(); }
#endif
}

void ADMShellGameMode::StartProbe()
{
    bProbeShots = FParse::Param(FCommandLine::Get(), TEXT("DMShellShot"));
    // Without shots this is two immediate clicks; with them each screen gets a second on screen.
    GetWorldTimerManager().SetTimer(ProbeTimer, this, &ADMShellGameMode::StepProbe, bProbeShots ? 1.f : .2f, true);
}

void ADMShellGameMode::StepProbe()
{
    const auto Shot = [this](const TCHAR* Name)
    {
        if (bProbeShots) { FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("shell-%s"), Name), false, false); }
    };
    switch (ProbeStep++)
    {
    case 0:
        UE_LOG(LogTemp, Display, TEXT("DREAD_SHELL_PROBE_MENU"));
        Shot(TEXT("mainmenu"));
        break;
    case 1:
        RequestStart();
        UE_LOG(LogTemp, Display, TEXT("DREAD_SHELL_PROBE_LOBBY phase=%d"), static_cast<int32>(Shell.Phase));
        break;
    case 2:
        Shot(TEXT("lobby"));
        break;
    case 3:
        RequestLaunch();
        GetWorldTimerManager().ClearTimer(ProbeTimer);
        break;
    default:
        GetWorldTimerManager().ClearTimer(ProbeTimer);
        break;
    }
}

void ADMShellGameMode::RequestStart() { Apply(Shell.Start()); }
void ADMShellGameMode::RequestLaunch() { Apply(Shell.Launch()); }
void ADMShellGameMode::RequestDismiss() { Apply(Shell.Dismiss()); }
void ADMShellGameMode::RequestQuit() { Apply(Shell.RequestQuit()); }

void ADMShellGameMode::Apply(EDMShellAction Action)
{
    PublishPhase();
    switch (Action)
    {
    case EDMShellAction::StreamMission:
        StreamMission();
        break;
    case EDMShellAction::BeginEncounter:
        BeginEncounter();
        break;
    case EDMShellAction::RestartShell:
        UGameplayStatics::OpenLevel(this, ShellLevelName);
        break;
    case EDMShellAction::Quit:
        UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
        break;
    case EDMShellAction::None:
        break;
    }
}

void ADMShellGameMode::PublishPhase()
{
    if (ADMGameState* Public = GetGameState<ADMGameState>())
    { Public->SetShellPhase(Shell.Phase, Shell.bVictory); }
}

void ADMShellGameMode::StreamMission()
{
    if (!IsFishingVillage()) { MissionLevel=TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox"))); }
    if (MissionStream)
    {
        // Already streamed once this session; the level cannot host a second encounter.
        UE_LOG(LogTemp, Warning, TEXT("DREAD_SHELL_MISSION_ALREADY_STREAMED"));
        return;
    }
    bool bLoaded = false;
    MissionStream = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
        this, MissionLevel, FVector::ZeroVector, FRotator::ZeroRotator, bLoaded);
    if (!MissionStream || !bLoaded)
    {
        // Without the authored map the encounter still runs: ADMCombatGameMode::BeginEncounter
        // spawns ADMSandboxArena when none is present.
        UE_LOG(LogTemp, Error, TEXT("DREAD_SHELL_MISSION_STREAM_FAILED level=%s"), *MissionLevel.ToString());
        Apply(Shell.MissionReady());
        return;
    }
    MissionStream->OnLevelShown.AddDynamic(this, &ADMShellGameMode::OnMissionShown);
}

void ADMShellGameMode::OnMissionShown()
{
    UE_LOG(LogTemp, Display, TEXT("DREAD_SHELL_MISSION_SHOWN"));
    Apply(Shell.MissionReady());
}

void ADMShellGameMode::OnEncounterComplete(bool bVictory)
{
    Apply(Shell.Complete(bVictory));
    UE_LOG(LogTemp, Display, TEXT("DREAD_SHELL_CASE_REPORT outcome=%s"), bVictory ? TEXT("victory") : TEXT("defeat"));
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("DMShellProbe")) && Shell.Phase == EDMShellPhase::CaseReport)
    {
        UE_LOG(LogTemp, Display, TEXT("DREAD_SHELL_PROBE_COMPLETE outcome=%s"), bVictory ? TEXT("victory") : TEXT("defeat"));
        // Requested inside the tick that finished the run, so it is captured by that frame's
        // render pass. A delayed request would lose the race with the smoke profile's exit.
        if (bProbeShots) { FScreenshotRequest::RequestScreenshot(TEXT("shell-casereport"), false, false); }
    }
#endif
}
