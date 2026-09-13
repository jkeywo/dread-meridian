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
class FDMVerifyProgression : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyProgression(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyProgression() { if (Settings) { Settings->RemoveFromRoot(); } }
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
            Hero->MadnessCore->Reset();
            auto* C = Hero->Progression.Get();
            Hero->ControlKind = TEXT("human");
            Test->TestFalse(TEXT("Cannot spend without XP"), C->Choose(0, 1));
            Test->TestTrue(TEXT("Accepted event awards XP"), C->Award(TEXT("objective:test"), 1200));
            Test->TestFalse(TEXT("Event not awarded twice"), C->Award(TEXT("objective:test"), 1200));
            Test->TestFalse(TEXT("No tier skip"), C->Choose(0, 3));
            Test->TestTrue(TEXT("First Q evolution"), C->Choose(0, 1));
            Test->TestEqual(TEXT("First evolution floor"), Hero->MadnessCore->View().Floor, 5.f);
            Test->TestTrue(TEXT("Hybrid via B"), C->Choose(0, 4));
            Test->TestFalse(TEXT("Invalid cross branch"), C->Choose(0, 5));
            Test->TestEqual(TEXT("Rejected choice adds no floor"), Hero->MadnessCore->View().Floor, 15.f);
            Test->TestTrue(TEXT("First W evolution"), C->Choose(1, 2));
            Test->TestTrue(TEXT("Hybrid via C"), C->Choose(1, 4));
            Test->TestFalse(TEXT("Enemies cannot gain XP"), Enemy->Progression->Award(TEXT("objective:test"), 100));
            Hero->ControlKind = TEXT("bot"); C->ChooseForBot();
            Test->TestEqual(TEXT("Bot spends remaining earned choices"), C->Get().Opportunities(), 0);
            Test->TestTrue(TEXT("All three abilities fully evolved"), C->Node(2) >= 3);
            Hero->MadnessCore->Recover(100, TEXT("test"));
            Test->TestEqual(TEXT("Six choices total floor"), Hero->MadnessCore->View().Floor, 45.f);
            Test->TestEqual(TEXT("Recovery respects evolution floor"), Hero->MadnessCore->View().Current, 45.f);
            Hero->ControlKind = TEXT("human");
            Test->TestEqual(TEXT("Handoff preserves build"), C->Node(0), uint8(4));
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMProgressionEditorTest, "DreadMeridian.Editor.Progression.Choices",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMProgressionEditorTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyProgression(this)); return true; }
#endif
