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
class FDMVerifyCompulsion : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyCompulsion(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyCompulsion() { if (Settings) { Settings->RemoveFromRoot(); } }
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
            C->AssignFamily(EDMMadnessFamily::Compulsion); C->Add(60, TEXT("test")); C->Step(M->GetCombatTick());
            Test->TestEqual(TEXT("One normal urge"), C->View().Cues.Num(), 1);
            if (!C->View().Cues.Num()) { GEditor->RequestEndPlayMap(); Stage=4; return false; }
            auto* Target = M->FindCombatant(C->View().Cues[0].TargetId);
            Hero->DealCombatDamage(Target, 1, TEXT("test.indulge"));
            Test->TestEqual(TEXT("Indulging eases pressure"), C->View().Current, 56.f);
            Test->TestTrue(TEXT("Urge completed"), C->View().Cues.IsEmpty());
            C->Add(100, TEXT("test.crisis")); C->Step(M->GetCombatTick());
            Test->TestEqual(TEXT("Crisis offers three simultaneous urges"), C->View().Cues.Num(), 3);
            Test->TestFalse(TEXT("No forced control"), Hero->IsStunned());
            const float Shield = Hero->Shield();
            auto V = C->View();
            for (const auto& Cue : V.Cues) { if (Cue.TargetId.IsEmpty()) { Hero->SetActorLocation(Cue.Location+FVector(0,0,95)); C->Step(M->GetCombatTick()); break; } }
            Test->TestTrue(TEXT("Visiting location indulges one urge"), C->View().Cues.Num() < 3);
            Test->TestTrue(TEXT("Crisis indulgence grants a brief benefit"), Hero->Shield() > Shield);
            C->ResolveCrisis(TEXT("test")); C->Recover(1000, TEXT("test")); C->Add(60, TEXT("test")); C->Step(M->GetCombatTick());
            Due = M->GetCombatTick() + C->Settings.FamilyInterval + 1; Before = C->View().Current; Stage=2; return false;
        }
        if (Stage == 2 && M && M->GetCombatTick() >= Due)
        {
            Test->TestTrue(TEXT("Resisting an urge increases pressure"), Hero->MadnessCore->View().Current > Before);
            GEditor->RequestEndPlayMap(); Stage=4; return false;
        }
        if (Stage == 4 && !W) { if (Window) { Window->RequestDestroyWindow(); } return true; }
        if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("Family test timed out")); GEditor->RequestEndPlayMap(); return true; }
        return false;
    }
private:
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    TWeakObjectPtr<ADMCombatant> Hero, Enemy;
    int32 Stage = 0, Due = 0;
    float Before = 0;
    double Deadline = 0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMCompulsionTest, "DreadMeridian.Editor.Madness.Compulsion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMCompulsionTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyCompulsion(this)); return true; }
#endif
