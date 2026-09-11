#include "DMGameMode.h"
#include "DMGameState.h"
#include "DMPlayerController.h"
#include "DMHarnessHUD.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SpectatorPawn.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/Parse.h"
#include "PlaytraceCaptureSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDreadMeridian, Log, All);

ADMGameMode::ADMGameMode()
{
    GameStateClass = ADMGameState::StaticClass();
    PlayerControllerClass = ADMPlayerController::StaticClass();
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    HUDClass = ADMHarnessHUD::StaticClass();
}

void ADMGameMode::StartPlay()
{
    Super::StartPlay();
    int32 Seed = DefaultRunSeed;
    FParse::Value(FCommandLine::Get(), TEXT("DMSeed="), Seed);
    RandomStreams = MakeUnique<FDMRandomStreams>(Seed);

    if (RitualPointsPerStage <= 0)
    {
        UE_LOG(LogDreadMeridian, Warning, TEXT("Invalid ritual tuning; using provisional 100 points per stage."));
        RitualPointsPerStage = 100;
    }

    TSharedRef<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetStringField(TEXT("project_id"), TEXT("dread-meridian"));
    Metadata->SetStringField(TEXT("scenario_id"), TEXT("foundation-harness"));
    Metadata->SetStringField(TEXT("run_kind"), TEXT("foundation_harness"));
    Metadata->SetNumberField(TEXT("seed"), Seed);
    Metadata->SetNumberField(TEXT("rng_schema_version"), 1);
    Metadata->SetNumberField(TEXT("ritual_points_per_stage"), RitualPointsPerStage);
    Metadata->SetNumberField(TEXT("investigator_slots"), FDMRunState::InvestigatorCount);
    Metadata->SetNumberField(TEXT("production_bots"), 0);
    Metadata->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
    Metadata->SetStringField(TEXT("capture_version"), TEXT("0.1.0"));
    Metadata->SetStringField(TEXT("started_at_utc"), FDateTime::UtcNow().ToIso8601());
    // The launcher fingerprints current inputs; unrecorded is never treated as provenance.
    for (const FString& Key : { FString(TEXT("GameRevision")), FString(TEXT("SourceDigest")), FString(TEXT("GDDDigest")) })
    {
        FString Value = TEXT("unrecorded");
        FParse::Value(FCommandLine::Get(), *(TEXT("DM") + Key + TEXT("=")), Value);
        Metadata->SetStringField(Key, Value);
    }
    Metadata->SetBoolField(TEXT("working_tree_dirty"), FParse::Param(FCommandLine::Get(), TEXT("DMDirty")));
    ConfigureCaptureMetadata(Metadata);
    GetGameInstance()->GetSubsystem<UPlaytraceCaptureSubsystem>()->BeginCapture(Metadata);
    State.Start();
    Publish(TEXT("run.state_changed"));

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("DMSmokeTest")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &ADMGameMode::RunSmokeTest);
    }
#endif
}

void ADMGameMode::Publish(const FString& EventType, int32 RitualDelta)
{
    if (ADMGameState* PublicState = GetGameState<ADMGameState>()) { PublicState->Publish(State); }
    TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("phase"), StaticEnum<EDMRunPhase>()->GetNameStringByValue(static_cast<int64>(State.Phase)));
    Data->SetStringField(TEXT("ritual_stage"), StaticEnum<EDMRitualStage>()->GetNameStringByValue(static_cast<int64>(State.RitualStage)));
    Data->SetNumberField(TEXT("ritual_progress"), State.RitualProgress);
    if (EventType == TEXT("ritual.advanced")) { Data->SetNumberField(TEXT("points"), RitualDelta); }
    GetGameInstance()->GetSubsystem<UPlaytraceCaptureSubsystem>()->RecordEvent(EventType, Data);
    UE_LOG(LogDreadMeridian, Display, TEXT("%s: phase=%s ritual=%s progress=%d"), *EventType,
        *Data->GetStringField(TEXT("phase")), *Data->GetStringField(TEXT("ritual_stage")), State.RitualProgress);
}

bool ADMGameMode::AdvanceRitual(int32 Points)
{
    if (!HasAuthority() || !State.AdvanceRitual(Points, RitualPointsPerStage)) { return false; }
    Publish(TEXT("ritual.advanced"), Points);
    return true;
}

bool ADMGameMode::Summon()
{
    if (!HasAuthority() || !State.Summon()) { return false; }
    Publish(TEXT("ritual.summoned"));
    return true;
}

bool ADMGameMode::FinishRun(bool bVictory)
{
    if (!HasAuthority() || !State.Finish(bVictory)) { return false; }
    Publish(TEXT("run.state_changed"));
    GetGameInstance()->GetSubsystem<UPlaytraceCaptureSubsystem>()->EndCapture(bVictory ? TEXT("victory") : TEXT("defeat"));
    return true;
}

void ADMGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetGameInstance()) { GetGameInstance()->GetSubsystem<UPlaytraceCaptureSubsystem>()->EndCapture(TEXT("aborted")); }
    Super::EndPlay(EndPlayReason);
}

void ADMGameMode::RunSmokeTest()
{
    // Exercises the real GameMode -> replicated projection -> capture path, not production AI.
    const bool bPassed = !AdvanceRitual(-1) && !FinishRun(true)
        && AdvanceRitual(RitualPointsPerStage) && State.RitualStage == EDMRitualStage::Stirring
        && Summon() && State.Phase == EDMRunPhase::Apocalypse
        && FinishRun(true) && !AdvanceRitual(1) && !FinishRun(false);
    UE_LOG(LogDreadMeridian, Display, TEXT("DREAD_MERIDIAN_SMOKE_%s"), bPassed ? TEXT("PASSED") : TEXT("FAILED"));
    FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}
