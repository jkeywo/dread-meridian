#include "DMCombatPlayerController.h"
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
class FDMVerifyPerception : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyPerception(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyPerception() { if (Settings) { Settings->RemoveFromRoot(); } }
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
            auto* P=Cast<ADMCombatPlayerController>(W->GetFirstPlayerController());
            if (!P) { Test->AddError(TEXT("No player controller")); GEditor->RequestEndPlayMap(); Stage=9; return false; }
            P->Possess(Hero.Get());
            C->AssignFamily(EDMMadnessFamily::Perception); C->Add(30,TEXT("test")); C->Step(M->GetCombatTick());
            Test->TestEqual(TEXT("One harmless early anomaly"),C->View().Cues.Num(),1);
            if (!C->View().Cues.Num()) { GEditor->RequestEndPlayMap(); Stage=9; return false; }
            auto Cue=C->View().Cues[0]; Hero->SetActorLocation(Cue.Location+FVector(0,0,95));
            const float HP=Hero->Health();
            Test->TestTrue(TEXT("Early anomaly can be examined"),C->InteractPerception(Cue.Id));
            Test->TestEqual(TEXT("Early anomaly harmless"),Hero->Health(),HP);
            Test->TestFalse(TEXT("Private core is not replicated"),C->GetIsReplicated());
            C->Add(30,TEXT("test.mid")); Due=M->GetCombatTick()+11; Stage=2; return false;
        }
        if (Stage==2 && M && M->GetCombatTick()>=Due)
        {
            auto* C=Hero->MadnessCore.Get();
            Test->TestTrue(TEXT("Mid band has private entities and hazards"),C->View().Cues.Num()>=2);
            if (!C->View().Cues.Num()) { GEditor->RequestEndPlayMap(); Stage=9; return false; }
            const auto Cue=C->View().Cues[0]; Id=Cue.Id; Hero->SetActorLocation(Cue.Location+FVector(0,0,95));
            Test->TestTrue(TEXT("Private entity accepts first interaction"),C->InteractPerception(Id));
            Test->TestFalse(TEXT("Interaction cooldown prevents spam"),C->InteractPerception(Id));
            Due=M->GetCombatTick()+6; Stage=3; return false;
        }
        if (Stage==3 && M && M->GetCombatTick()>=Due)
        {
            auto* C=Hero->MadnessCore.Get();
            Test->TestTrue(TEXT("Second interaction clears mid entity"),C->InteractPerception(Id));
            Test->TestFalse(TEXT("Another investigator cannot invent a private entity"),C->InteractPerception(TEXT("other.private")));
            C->Add(100,TEXT("test.crisis")); C->Recover(1000,TEXT("test.recovery_during_crisis")); C->Step(M->GetCombatTick());
            Test->TestTrue(TEXT("Crisis has a dense subjective layer"),C->View().Cues.Num()>=5);
            Test->TestFalse(TEXT("Crisis retains player control"),Hero->IsStunned());
            Due=M->GetCombatTick()+6; Stage=4; return false;
        }
        if (Stage==4 && M && M->GetCombatTick()>=Due)
        {
            auto* C=Hero->MadnessCore.Get(); const auto Cue=C->View().Cues[0]; const int32 Count=C->View().Cues.Num();
            Hero->SetActorLocation(Cue.Location+FVector(0,0,95)); const float Shield=Hero->Shield();
            Test->TestTrue(TEXT("Crisis advantage clears entity in one interaction"),C->InteractPerception(Cue.Id));
            Test->TestEqual(TEXT("One subjective entity cleared"),C->View().Cues.Num(),Count-1);
            Test->TestTrue(TEXT("High/Crisis interaction changes real combat resources"),Hero->Shield()>Shield);
            Test->TestEqual(TEXT("Other HUD rows do not see private layer"),FDMHudModel::Read(*Hero,TEXT("other")).Madness,0.f);
            for (const auto& Hazard : C->View().Cues)
            { if (Hazard.Id.StartsWith(TEXT("perception.hazard"))) { Hero->SetActorLocation(Hazard.Location+FVector(0,0,95)); break; } }
            Due=M->GetCombatTick()+16; Stage=5; return false;
        }
        if (Stage==5 && M && M->GetCombatTick()>=Due)
        {
            Test->TestTrue(TEXT("High subjective hazard has observable slow consequence"),Hero->GetCharacterMovement()->MaxWalkSpeed<420);
            GEditor->RequestEndPlayMap(); Stage=9; return false;
        }
        if (Stage == 9 && !W) { if (Window) { Window->RequestDestroyWindow(); } return true; }
        if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("Family test timed out")); GEditor->RequestEndPlayMap(); return true; }
        return false;
    }
private:
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    TWeakObjectPtr<ADMCombatant> Hero, Enemy;
    int32 Stage = 0, Due = 0;
    FString Id;
    double Deadline = 0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMPerceptionTest, "DreadMeridian.Editor.Madness.Perception",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMPerceptionTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyPerception(this)); return true; }
#endif
