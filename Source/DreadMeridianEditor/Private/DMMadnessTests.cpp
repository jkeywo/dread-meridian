#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMRecoverySupply.h"
#include "DMHudModel.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS
class FDMVerifyMadness : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyMadness(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyMadness() { if (Settings) { Settings->RemoveFromRoot(); } }
    bool Update() override
    {
        if (Stage == 0)
        {
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone); Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Madness verification"))).ClientSize(FVector2D(640,480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams P; P.EditorPlaySettings = Settings; P.CustomPIEWindow = Window;
            P.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox"); P.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(P); GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 90; Stage = 1; return false;
        }
        UWorld* W = GEditor->PlayWorld;
        auto* M = W ? W->GetAuthGameMode<ADMCombatGameMode>() : nullptr;
        if (Stage == 1 && M && M->GetCombatants().Num() >= 5)
        {
            int32 N = 0;
            for (ADMCombatant* A : M->GetCombatants())
            {
                if (A->GetController()) { A->GetController()->UnPossess(); }
                A->StopGoal(); A->SetAttackTarget(nullptr); A->NextAttackTick = 100000;
                A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(1500, -650 + N++ * 100, 95));
                if (!A->bIsEnemy && !Hero.IsValid()) { Hero = A; }
                if (A->bIsEnemy && !Enemy.IsValid()) { Enemy = A; }
            }
            Hero->InitializeInvestigator(EDMInvestigator::Photographer, false);
            Hero->bProfileRange = true; Hero->SetActorLocation(FVector(-1200,0,95)); Enemy->SetActorLocation(FVector(-1080,0,95));
            auto* C = Hero->MadnessCore.Get();
            Test->TestFalse(TEXT("Locked pools cannot manifest"), C->Manifest(TEXT("test.private"), TEXT("A private contextual cue"), 1, 5));
            Test->TestTrue(TEXT("Real R executes"), Hero->Kit->Request(EDMKitSlot::R, nullptr, Hero->GetActorLocation()));
            Test->TestEqual(TEXT("R pressure reaches core"), C->View().Current, 30.f);
            Test->TestTrue(TEXT("Unlocked pool can manifest contextual cue"), C->Manifest(TEXT("test.private"), TEXT("A private contextual cue"), 1, 5));
            Test->TestFalse(TEXT("Active manifestation cannot be replaced"), C->Manifest(TEXT("test.other"), TEXT("Other"), 1, 5));
            Test->TestFalse(TEXT("Shared summaries omit Madness"), Hero->Investigator->ResourceSummary().Contains(TEXT("Madness")));
            Test->TestEqual(TEXT("Other HUD rows cannot expose state"), FDMHudModel::Read(*Hero, TEXT("other")).Madness, 0.f);
            Test->TestEqual(TEXT("Owner HUD exposes state"), FDMHudModel::Read(*Hero, Hero->EntityId).Madness, 30.f);
            C->RaiseFloor(20, TEXT("test.evolution"));
            Test->TestTrue(TEXT("Grounding starts"), C->BeginGrounding());
            Enemy->DealCombatDamage(Hero.Get(), 1, TEXT("test.interrupt"));
            Test->TestEqual(TEXT("Incoming hit interrupts grounding"), C->View().GroundUntil, 0);
            C->BeginGrounding(); Hero->SetActorLocation(Hero->GetActorLocation() + FVector(0,30,0)); C->Step(M->GetCombatTick());
            Test->TestEqual(TEXT("Movement interrupts grounding"), C->View().GroundUntil, 0);
            C->BeginGrounding(); Hero->Injuries->OnCast();
            Test->TestEqual(TEXT("Accepted cast interrupts grounding"), C->View().GroundUntil, 0);
            C->BeginGrounding(); Due = C->View().GroundUntil + 1; Stage = 2; return false;
        }
        if (Stage == 2 && M && M->GetCombatTick() >= Due)
        {
            auto* C = Hero->MadnessCore.Get();
            Test->TestEqual(TEXT("Quiet grounding reduces current to floor"), C->View().Current, 20.f);
            Test->TestFalse(TEXT("Grounding at floor refuses"), C->BeginGrounding());
            Test->TestTrue(TEXT("Private cue expires"), C->View().SymptomId.IsEmpty());
            C->Add(100, TEXT("test.crisis"));
            Test->TestTrue(TEXT("Crisis begins"), C->View().CrisisUntil > 0);
            Test->TestFalse(TEXT("Crisis never stuns"), Hero->IsStunned());
            Hero->NextAttackTick = 0;
            Test->TestTrue(TEXT("Player attack remains accepted during Crisis"), Hero->TryAttack(Enemy.Get()));
            Due = C->View().CrisisUntil + 1; Stage = 3; return false;
        }
        if (Stage == 3 && M && M->GetCombatTick() >= Due)
        {
            Test->TestEqual(TEXT("Live Crisis expires"), Hero->MadnessCore->View().CrisisUntil, 0);
            Test->TestEqual(TEXT("Live Crisis recovers current"), Hero->MadnessCore->View().Current, 50.f);
            Test->TestEqual(TEXT("Floor survives Crisis"), Hero->MadnessCore->View().Floor, 20.f);
            GEditor->RequestEndPlayMap(); Stage = 4; return false;
        }
        if (Stage == 4 && !W) { if (Window) { Window->RequestDestroyWindow(); } return true; }
        if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("Madness test timed out")); GEditor->RequestEndPlayMap(); return true; }
        return false;
    }
private:
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    TWeakObjectPtr<ADMCombatant> Hero, Enemy;
    int32 Stage = 0, Due = 0;
    double Deadline = 0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMMadnessIntegrationTest, "DreadMeridian.Editor.Madness.Core",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMMadnessIntegrationTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyMadness(this)); return true; }
#endif
