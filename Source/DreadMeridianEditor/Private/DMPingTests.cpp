#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMEditorPlaySelection.h"
#include "DMEncounterLayout.h"
#include "DMGameState.h"
#include "DMPing.h"
#include "DMSquadController.h"
#include "DMUtilityAI.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS
/**
 * PIE check for the ping board and its consumption by a companion bot. The human plays the Smuggler; the Medium bot
 * is the subject (its Q never takes the Move channel). Actors are frozen as in the utility AI check. Stages are
 * spaced at least six ticks apart because an author may only ping every five ticks. The site sits in the west of the
 * arena so that, once the two camp enemies are sent home, no enemy (the patrol pair idles at (-650, -850) and
 * (-650, -730) plus spawn jitter) is inside the companion tether (850u of the leader or 500u of the bot): the
 * GoHere and Perceive stages need a bot with nothing to focus.
 */
class FDMCheckPings : public IAutomationLatentCommand
{
public:
    explicit FDMCheckPings(FAutomationTestBase* InTest) : Test(InTest)
    { bHadPreference = GConfig->GetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), Original, GEditorPerProjectIni); }
    virtual ~FDMCheckPings() override
    {
        if (bHadPreference) { GConfig->SetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), *Original, GEditorPerProjectIni); }
        else { GConfig->RemoveKey(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), GEditorPerProjectIni); }
        GConfig->Flush(false, GEditorPerProjectIni);
        if (Settings) { Settings->RemoveFromRoot(); }
    }
    static void Place(ADMCombatant* Enemy, FVector At)
    { Enemy->SetActorLocation(At); if (auto* Bot = Cast<ADMSquadController>(Enemy->GetController())) { Bot->ConfigureEncounter(Bot->GetEncounterGroup(), At, false, 0); } }
    /** Bots call out Enemy and Help pings of their own; cancel them as their authors so only the human's ping shapes the next decision. */
    static void ClearOtherPings(ADMCombatGameMode& Mode, const FString& Keep)
    {
        const TArray<FDMPing> Copy = Mode.GetPingBoard().Pings;
        for (const FDMPing& P : Copy) { if (P.AuthorId != Keep) { Mode.CancelPing(P.Id, P.AuthorId); } }
    }
    virtual bool Update() override
    {
        if (Stage == 0)
        {
            DMEditorPlaySelection::Save(TEXT("Smuggler"));
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone);
            Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Ping verification"))).ClientSize(FVector2D(640, 480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams Params; Params.EditorPlaySettings = Settings;
            Params.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox");
            Params.CustomPIEWindow = Window; Params.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(Params); GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 60; Stage = 1; return false;
        }
        UWorld* World = GEditor->PlayWorld;
        auto* Mode = World ? World->GetAuthGameMode<ADMCombatGameMode>() : nullptr;
        auto* PC = World ? World->GetFirstPlayerController() : nullptr;
        auto* Hero = PC ? Cast<ADMCombatant>(PC->GetPawn()) : nullptr;
        if (Stage == 1 && Mode && Hero && Mode->IsCombatActive())
        {
            int32 Parked = 0;
            for (ADMCombatant* X : Mode->GetCombatants())
            {
                X->NextAttackTick = 100000; X->Smuggler->NextSignatureTick = 100000; X->GetCharacterMovement()->DisableMovement();
                X->StopGoal(); X->SetAttackTarget(nullptr);
                const auto* XBot = Cast<ADMSquadController>(X->GetController());
                if (!X->bIsEnemy && X != Hero)
                {
                    X->SetActorLocation(FVector(-2600, (Parked++ - 1) * 150, 95));
                    if (XBot && X->Investigator->Kind == EDMInvestigator::Medium) { Bot = X; }
                }
                else if (XBot && X->bIsEnemy && !X->IsDown() && XBot->GetEncounterGroup() >= 0 && XBot->GetEncounterGroup() < DMEncounterLayout::CampCount)
                {
                    if (!A.IsValid()) { A = X; HomeA = X->GetActorLocation(); }
                    else if (!B.IsValid()) { B = X; HomeB = X->GetActorLocation(); }
                }
            }
            if (!Bot.IsValid() || !A.IsValid() || !B.IsValid())
            { Test->AddError(TEXT("Roster lacks a Medium bot or two camp enemies (the human must play the Smuggler)")); GEditor->RequestEndPlayMap(); Stage = 5; return false; }
            ADMCombatant* C = Bot.Get();
            C->SetActorLocation(Site); Hero->SetActorLocation(Site + FVector(-200, 0, 0));
            Place(A.Get(), Site + FVector(300, 0, 0)); Place(B.Get(), Site + FVector(0, 350, 0));
            A->Threat.Add(C->EntityId, 1.f); B->Threat.Add(C->EntityId, 1.f);
            auto* Brain = CastChecked<ADMSquadController>(C->GetController());
            Brain->Think(*Mode);
            Test->TestTrue(TEXT("Bot engages the nearest enemy"), C->GetAttackTarget() == A.Get());
            FocusId = Mode->CreatePing(EDMPingKind::Focus, Hero->EntityId, false, B->GetActorLocation(), B->EntityId);
            Test->TestTrue(TEXT("Human Focus ping accepted"), FocusId != INDEX_NONE);
            Brain->Think(*Mode);
            Test->TestTrue(TEXT("Focus ping retargets the bot in one Think"), C->GetAttackTarget() == B.Get());
            const FDMPing* Ping = Mode->GetPingBoard().Find(FocusId);
            Test->TestTrue(TEXT("Bot answers the Focus ping with on_it"), Ping && Ping->OnIt.Contains(C->EntityId));
            Tick = Mode->GetCombatTick(); Stage = 2;
        }
        if (Stage == 2 && Mode && Hero && Mode->GetCombatTick() >= Tick + 6)
        {
            const ADMGameState* Projection = Mode->GetGameState<ADMGameState>();
            int32 Live = 0; bool bMirrored = Projection != nullptr;
            for (const FDMPing& P : Mode->GetPingBoard().Pings)
            {
                if (!P.IsLive(Mode->GetCombatTick())) { continue; }
                ++Live; bMirrored = bMirrored && Projection->Pings.ContainsByPredicate([&](const FDMPing& Q) { return Q.Id == P.Id && Q.Kind == P.Kind; });
            }
            Test->TestTrue(TEXT("Game state mirrors the live board after a combat step"), bMirrored && Live > 0 && Projection->Pings.Num() == Live);
            const int32 Replacement = Mode->CreatePing(EDMPingKind::Focus, Hero->EntityId, false, A->GetActorLocation(), A->EntityId);
            Test->TestTrue(TEXT("Second Focus ping accepted after the author cooldown"), Replacement != INDEX_NONE);
            Test->TestNull(TEXT("Replaced Focus ping leaves the board"), Mode->GetPingBoard().Find(FocusId));
            int32 HeroFocus = 0; bool bOnA = false;
            for (const FDMPing& P : Mode->GetPingBoard().Pings)
            { if (P.Kind == EDMPingKind::Focus && P.AuthorId == Hero->EntityId) { ++HeroFocus; bOnA = P.TargetId == A->EntityId; } }
            Test->TestTrue(TEXT("One Focus ping per author, the newest"), HeroFocus == 1 && bOnA);
            FocusId = Replacement; Tick = Mode->GetCombatTick(); Stage = 3;
        }
        if (Stage == 3 && Mode && Hero && Mode->GetCombatTick() >= Tick + 6)
        {
            ADMCombatant* C = Bot.Get();
            Test->TestFalse(TEXT("Only the author cancels a ping"), Mode->CancelPing(FocusId, C->EntityId));
            Test->TestTrue(TEXT("Author cancels the ping"), Mode->CancelPing(FocusId, Hero->EntityId));
            Test->TestNull(TEXT("Cancelled ping leaves the board"), Mode->GetPingBoard().Find(FocusId));
            Place(A.Get(), HomeA); Place(B.Get(), HomeB);
            ClearOtherPings(*Mode, Hero->EntityId);
            RallyPoint = Site + FVector(500, 0, 0);
            RallyId = Mode->CreatePing(EDMPingKind::GoHere, Hero->EntityId, false, RallyPoint, FString());
            Test->TestTrue(TEXT("GoHere ping accepted"), RallyId != INDEX_NONE);
            auto* Brain = CastChecked<ADMSquadController>(C->GetController());
            Brain->Think(*Mode);
            Test->TestTrue(TEXT("Bot rallies to the GoHere ping"), Brain->GetLastDecision().Chose(EDMAIAction::RallyToPing));
            Test->TestTrue(TEXT("Rally goal is the ping location"), C->HasMoveGoal() && FVector::Dist2D(C->GetMoveGoal(), RallyPoint) <= 5.f);
            Tick = Mode->GetCombatTick(); Stage = 4;
        }
        if (Stage == 4 && Mode && Hero && Mode->GetCombatTick() >= Tick + 6)
        {
            ADMCombatant* C = Bot.Get();
            Test->TestTrue(TEXT("Author cancels the rally"), Mode->CancelPing(RallyId, Hero->EntityId));
            ClearOtherPings(*Mode, Hero->EntityId);
            const FVector Perceived = Site + FVector(0, 400, 0);
            const int32 PerceiveId = Mode->CreatePing(EDMPingKind::Perceive, Hero->EntityId, false, Perceived, FString());
            const FDMPing* Ping = Mode->GetPingBoard().Find(PerceiveId);
            Test->TestTrue(TEXT("Perceive ping is subjective and carries no target"), Ping && Ping->bSubjective && Ping->TargetId.IsEmpty());
            auto* Brain = CastChecked<ADMSquadController>(C->GetController());
            Brain->Think(*Mode);
            Test->TestTrue(TEXT("Bot investigates the perceived location"), Brain->GetLastDecision().Chose(EDMAIAction::InvestigatePing));
            Test->TestTrue(TEXT("Investigation moves toward the ping"), C->HasMoveGoal() && FVector::Dist2D(C->GetMoveGoal(), Perceived) < FVector::Dist2D(C->GetActorLocation(), Perceived));
            Test->TestNull(TEXT("Perceive never becomes a target"), C->GetAttackTarget());
            Test->TestEqual(TEXT("Perceive never becomes a focus"), Brain->GetLastDecision().Focus, static_cast<int32>(INDEX_NONE));
            GEditor->RequestEndPlayMap(); Stage = 5;
        }
        if (Stage == 5 && !GEditor->PlayWorld) { if (Window) { Window->RequestDestroyWindow(); } return true; }
        if (FPlatformTime::Seconds() > Deadline)
        { Test->AddError(FString::Printf(TEXT("Ping PIE check timed out at stage %d"), Stage)); GEditor->RequestEndPlayMap(); return true; }
        return false;
    }
private:
    FAutomationTestBase* Test; ULevelEditorPlaySettings* Settings = nullptr; TSharedPtr<SWindow> Window;
    FString Original; bool bHadPreference = false;
    TWeakObjectPtr<ADMCombatant> Bot, A, B;
    FVector HomeA = FVector::ZeroVector, HomeB = FVector::ZeroVector, RallyPoint = FVector::ZeroVector;
    const FVector Site = FVector(-1600, 0, 95);
    int32 FocusId = INDEX_NONE, RallyId = INDEX_NONE, Tick = 0, Stage = 0; double Deadline = 0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMEditorPingsTest, "DreadMeridian.Editor.Pings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMEditorPingsTest::RunTest(const FString& Parameters)
{
    if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run the ping check in an idle editor.")); return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FDMCheckPings(this)); return true;
}
#endif
