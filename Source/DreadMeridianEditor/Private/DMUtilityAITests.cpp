#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMAbilityMarker.h"
#include "DMEditorPlaySelection.h"
#include "DMEncounterLayout.h"
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
 * PIE check for the utility brain on the authored encounter. The human plays the Smuggler so the Sapper,
 * Photographer and Medium are bots. Every actor is frozen (no basic attacks or signatures, movement disabled)
 * and each scenario reads one explicit Think: the decision plus the verbs it issued. All scenarios run inside
 * one Update so the game mode's own Think loop never interleaves.
 */
class FDMCheckUtilityAI : public IAutomationLatentCommand
{
public:
    explicit FDMCheckUtilityAI(FAutomationTestBase* InTest) : Test(InTest)
    { bHadPreference = GConfig->GetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), Original, GEditorPerProjectIni); }
    virtual ~FDMCheckUtilityAI() override
    {
        if (bHadPreference) { GConfig->SetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), *Original, GEditorPerProjectIni); }
        else { GConfig->RemoveKey(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), GEditorPerProjectIni); }
        GConfig->Flush(false, GEditorPerProjectIni);
        if (Settings) { Settings->RemoveFromRoot(); }
    }
    virtual bool Update() override
    {
        if (Stage == 0)
        {
            DMEditorPlaySelection::Save(TEXT("Smuggler"));
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone);
            Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Utility AI verification"))).ClientSize(FVector2D(640, 480));
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
            Test->TestTrue(TEXT("Normal PIE uses authored encounters"), Mode->UsesEncounterLayout());
            const TArray<TObjectPtr<ADMCombatant>>& Actors = Mode->GetCombatants();
            auto BotOf = [](ADMCombatant* A) { return Cast<ADMSquadController>(A->GetController()); };
            auto Companion = [&](EDMInvestigator Kind) -> ADMCombatant*
            {
                for (ADMCombatant* A : Actors) { if (!A->bIsEnemy && A != Hero && A->Investigator->Kind == Kind && BotOf(A)) { return A; } }
                return nullptr;
            };
            auto CampEnemy = [&](EDMSmuggler Role, ADMCombatant* Except) -> ADMCombatant*
            {
                for (ADMCombatant* A : Actors)
                {
                    const ADMSquadController* Bot = A->bIsEnemy ? BotOf(A) : nullptr;
                    if (A != Except && Bot && !A->IsDown() && A->Smuggler->Role == Role
                        && Bot->GetEncounterGroup() >= 0 && Bot->GetEncounterGroup() < DMEncounterLayout::CampCount) { return A; }
                }
                return nullptr;
            };
            ADMCombatant* Sapper = Companion(EDMInvestigator::Sapper);
            ADMCombatant* Photographer = Companion(EDMInvestigator::Photographer);
            ADMCombatant* Medium = Companion(EDMInvestigator::Medium);
            ADMCombatant* Gunman = CampEnemy(EDMSmuggler::Gunman, nullptr);
            ADMCombatant* Bomber = CampEnemy(EDMSmuggler::Bomber, nullptr);
            ADMCombatant* Bruiser = CampEnemy(EDMSmuggler::Bruiser, nullptr);
            ADMCombatant* Second = CampEnemy(EDMSmuggler::Bruiser, Bruiser);
            if (!Sapper || !Photographer || !Medium || !Gunman || !Bomber || !Bruiser || !Second)
            { Test->AddError(TEXT("Roster lacks a bot companion or camp role (the human must play the Smuggler)")); GEditor->RequestEndPlayMap(); Stage = 2; return false; }
            TMap<ADMCombatant*, FVector> Homes;
            for (ADMCombatant* A : Actors)
            {
                A->NextAttackTick = 100000; A->Smuggler->NextSignatureTick = 100000; A->GetCharacterMovement()->DisableMovement();
                if (A->bIsEnemy) { Homes.Add(A, A->GetActorLocation()); }
            }
            // Re-anchoring a camp enemy where it is placed keeps the leash latch out of the behaviour under test.
            auto Place = [&](ADMCombatant* Enemy, FVector At)
            { Enemy->SetActorLocation(At); if (auto* Bot = BotOf(Enemy)) { Bot->ConfigureEncounter(Bot->GetEncounterGroup(), At, false, 0); } };
            auto Reset = [&]()
            {
                int32 Parked = 0;
                for (ADMCombatant* A : Actors)
                {
                    A->StopGoal(); A->SetAttackTarget(nullptr); A->Primary->CancelChannel();
                    if (!A->bIsEnemy) { if (A != Hero) { A->SetActorLocation(FVector(-2600, (Parked++ - 1) * 150, 95)); } }
                    else if (const ADMSquadController* Bot = BotOf(A); Bot && Bot->GetEncounterGroup() < DMEncounterLayout::CampCount) { Place(A, Homes[A]); }
                }
            };

            // (a) Conservation: a Sapper holds its last charge until two enemies share the blast radius.
            Reset();
            const FVector SiteA(-900, 300, 95);
            Sapper->SetActorLocation(SiteA); Hero->SetActorLocation(SiteA + FVector(-300, 0, 0));
            Place(Bruiser, SiteA + FVector(300, 0, 0));
            Sapper->Investigator->Charges = 1;
            ADMSquadController* SapperBot = BotOf(Sapper);
            SapperBot->Think(*Mode);
            {
                const FDMAIDecision& D = SapperBot->GetLastDecision();
                const FDMAIOption* Satchel = D.Ranked.FindByPredicate([](const FDMAIOption& O) { return O.Action == EDMAIAction::PlaceSatchel; });
                Test->TestTrue(TEXT("Sapper targets the lone enemy"), Sapper->GetAttackTarget() == Bruiser);
                Test->TestTrue(TEXT("Last charge is ranked but vetoed or below threshold for one enemy"), Satchel && (Satchel->Veto != nullptr || Satchel->Value < Satchel->Threshold));
                Test->TestFalse(TEXT("Last charge is not spent on one enemy"), D.Chose(EDMAIAction::PlaceSatchel));
                Test->TestEqual(TEXT("No satchel placed for one enemy"), Sapper->Primary->Satchels.Num(), 0);
            }
            Place(Second, SiteA + FVector(300, 150, 0));
            SapperBot->Think(*Mode);
            Test->TestTrue(TEXT("Two enemies in the blast radius clear the threshold"), SapperBot->GetLastDecision().Chose(EDMAIAction::PlaceSatchel));
            Test->TestEqual(TEXT("Satchel placed for two enemies"), Sapper->Primary->Satchels.Num(), 1);

            // (b) A damaged companion steps out of a hostile circle without dropping its target or holding fire.
            Reset();
            const FVector SiteB(-900, -300, 95);
            Medium->SetActorLocation(SiteB); Hero->SetActorLocation(SiteB + FVector(-300, 0, 0));
            Place(Bruiser, SiteB + FVector(400, 0, 0));
            Test->TestTrue(TEXT("Test wound brings the Medium to half health"),
                Bruiser->DealCombatDamage(Medium, Medium->Shield() + Medium->Health() - Medium->MaxHealth() * .5f, TEXT("ability.test.wound")));
            ADMAbilityMarker* Circle = World->SpawnActor<ADMAbilityMarker>(SiteB + FVector(-30, 0, -70), FRotator::ZeroRotator);
            Circle->bHostile = true; Circle->Radius = 180; Circle->CustomLabel = TEXT("FIREBOMB - MOVE");
            ADMSquadController* MediumBot = BotOf(Medium);
            MediumBot->Think(*Mode);
            {
                const FDMAIDecision& D = MediumBot->GetLastDecision();
                Test->TestTrue(TEXT("Companion inside a hostile circle evades"), D.Chose(EDMAIAction::EvadeHazard));
                Test->TestTrue(TEXT("Evade goal is outside the circle"),
                    Medium->HasMoveGoal() && FVector::Dist2D(Medium->GetMoveGoal(), Circle->GetActorLocation()) > Circle->Radius);
                Test->TestTrue(TEXT("Evading keeps the attack target"), Medium->GetAttackTarget() == Bruiser);
                Test->TestFalse(TEXT("Evading leaves the attack channel open"), Medium->IsAttackHeld());
            }
            Circle->SetActorLocation(FVector(2500, 2000, -500)); Circle->Destroy();

            // (c) A ranged enemy backs away from an investigator inside half its attack range.
            Reset();
            const FVector SiteC(-900, 0, 95);
            Place(Gunman, SiteC); Hero->SetActorLocation(SiteC + FVector(150, 0, 0));
            ADMSquadController* GunBot = BotOf(Gunman);
            GunBot->Think(*Mode);
            Test->TestTrue(TEXT("Gunman acquires the close investigator"), Gunman->GetAttackTarget() == Hero);
            Test->TestTrue(TEXT("Gunman keeps distance"), GunBot->GetLastDecision().Chose(EDMAIAction::KeepDistance));
            Test->TestTrue(TEXT("Keep-distance goal opens the range"), Gunman->HasMoveGoal()
                && FVector::Dist2D(Gunman->GetMoveGoal(), Hero->GetActorLocation()) > FVector::Dist2D(Gunman->GetActorLocation(), Hero->GetActorLocation()));

            // (d) A companion at 20% health retreats toward the human leader and still fires at an enemy in range.
            Reset();
            Photographer->SetActorLocation(SiteC); Hero->SetActorLocation(SiteC + FVector(-600, 0, 0));
            Place(Bruiser, SiteC + FVector(250, 0, 0));
            Test->TestTrue(TEXT("Test wound brings the Photographer to a fifth of its health"),
                Bruiser->DealCombatDamage(Photographer, Photographer->Shield() + Photographer->Health() - Photographer->MaxHealth() * .2f, TEXT("ability.test.wound")));
            ADMSquadController* PhotoBot = BotOf(Photographer);
            PhotoBot->Think(*Mode);
            {
                const FDMAIDecision& D = PhotoBot->GetLastDecision();
                Test->TestTrue(TEXT("Low-health companion flees"), D.Chose(EDMAIAction::Flee));
                Test->TestTrue(TEXT("Flee goal closes on the leader"), Photographer->HasMoveGoal()
                    && FVector::Dist2D(Photographer->GetMoveGoal(), Hero->GetActorLocation()) < FVector::Dist2D(Photographer->GetActorLocation(), Hero->GetActorLocation()));
                Test->TestTrue(TEXT("Fleeing companion still fires at an enemy in range"), D.Chose(EDMAIAction::BasicAttack) && !D.bAttackHold);
                Test->TestFalse(TEXT("Attack channel is not held while fleeing"), Photographer->IsAttackHeld());
                Test->TestTrue(TEXT("Fleeing keeps the attack target"), Photographer->GetAttackTarget() == Bruiser);
            }

            // (e) A casting bomber holds every channel and drops its target.
            Reset();
            Place(Bomber, SiteC); Hero->SetActorLocation(SiteC + FVector(400, 0, 0));
            Bomber->Smuggler->NextSignatureTick = 0;
            Test->TestTrue(TEXT("Bomber starts its firebomb cast"), Bomber->Smuggler->TrySignature(*Mode, Hero));
            ADMSquadController* BomberBot = BotOf(Bomber);
            BomberBot->Think(*Mode);
            {
                const FDMAIDecision& D = BomberBot->GetLastDecision();
                Test->TestTrue(TEXT("Casting bomber chooses only HoldCast"), D.Chosen.Num() == 1 && D.Chosen[0].Action == EDMAIAction::HoldCast);
                Test->TestEqual(TEXT("Casting bomber has no focus"), D.Focus, static_cast<int32>(INDEX_NONE));
                Test->TestNull(TEXT("Casting bomber drops its attack target"), Bomber->GetAttackTarget());
            }
            Bomber->Smuggler->Cancel();
            GEditor->RequestEndPlayMap(); Stage = 2;
        }
        if (Stage == 2 && !GEditor->PlayWorld) { if (Window) { Window->RequestDestroyWindow(); } return true; }
        if (FPlatformTime::Seconds() > Deadline)
        { Test->AddError(TEXT("Utility AI PIE check timed out")); GEditor->RequestEndPlayMap(); return true; }
        return false;
    }
private:
    FAutomationTestBase* Test; ULevelEditorPlaySettings* Settings = nullptr; TSharedPtr<SWindow> Window;
    FString Original; bool bHadPreference = false;
    int32 Stage = 0; double Deadline = 0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMEditorUtilityAITest, "DreadMeridian.Editor.UtilityAI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMEditorUtilityAITest::RunTest(const FString& Parameters)
{
    if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run the utility AI check in an idle editor.")); return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FDMCheckUtilityAI(this)); return true;
}
#endif
