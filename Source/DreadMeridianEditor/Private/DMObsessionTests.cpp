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
class FDMVerifyObsession : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyObsession(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyObsession() { if (Settings) { Settings->RemoveFromRoot(); } }
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
            C->AssignFamily(EDMMadnessFamily::Obsession); C->Add(30, TEXT("test")); C->Step(M->GetCombatTick());
            Test->TestEqual(TEXT("One fixation"), C->View().Cues.Num(), 1);
            if (C->View().Cues.Num() != 1) { GEditor->RequestEndPlayMap(); Stage = 4; return false; }
            auto* Target = M->FindCombatant(C->View().Cues[0].TargetId);
            for (int32 I=0; I<3; ++I) { Hero->DealCombatDamage(Target, 1, TEXT("test.fixation")); }
            Test->TestTrue(TEXT("Acting eases pressure"), C->View().Current < 30);
            Test->TestTrue(TEXT("Normal fixation resolved"), C->View().Cues.IsEmpty());
            C->Add(100, TEXT("test.crisis")); C->Step(M->GetCombatTick());
            Test->TestEqual(TEXT("One overwhelming fixation"), C->View().Cues.Num(), 1);
            Target = M->FindCombatant(C->View().Cues[0].TargetId);
            Hero->DealCombatDamage(Target, 1, TEXT("test.fixation"));
            Test->TestTrue(TEXT("Crisis benefit escalates"), C->Outgoing(Target) > 1);
            for (int32 I=0; I<4; ++I) { Hero->DealCombatDamage(Target, 1, TEXT("test.fixation")); }
            Test->TestEqual(TEXT("Resolution ends Crisis"), C->View().CrisisUntil, 0);
            Test->TestFalse(TEXT("Crisis never steals control"), Hero->IsStunned());
            Test->TestEqual(TEXT("Other HUD rows remain private"), FDMHudModel::Read(*Hero, TEXT("other")).Madness, 0.f);
            GEditor->RequestEndPlayMap(); Stage = 4; return false;
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
    int32 Stage = 0;
    double Deadline = 0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMObsessionTest, "DreadMeridian.Editor.Madness.Obsession",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMObsessionTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyObsession(this)); return true; }
#endif
