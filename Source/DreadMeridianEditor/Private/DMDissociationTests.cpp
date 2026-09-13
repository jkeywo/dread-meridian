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
class FDMVerifyDissociation : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyDissociation(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyDissociation() { if (Settings) { Settings->RemoveFromRoot(); } }
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
            C->AssignFamily(EDMMadnessFamily::Dissociation); C->Add(75, TEXT("test"));
            Hero->Investigator->AddExposure(Enemy->EntityId,40,false,M->GetCombatTick());
            Test->TestTrue(TEXT("Actual Develop cast accepted"), Hero->Kit->Request(EDMKitSlot::E, Enemy.Get(), Enemy->GetActorLocation()));
            Test->TestEqual(TEXT("One scheduled echo"), C->View().Cues.Num(), 1);
            Before = Enemy->Health();
            Test->TestFalse(TEXT("Cooldown rejects repeated cast"), Hero->Kit->Request(EDMKitSlot::E, Enemy.Get(), Enemy->GetActorLocation()));
            Test->TestEqual(TEXT("Rejected cast queues nothing"), C->View().Cues.Num(), 1);
            Hero->SetActorLocation(Hero->GetActorLocation()+FVector(0,500,0));
            Due = M->GetCombatTick()+21; Stage=2; return false;
        }
        if (Stage == 2 && M && M->GetCombatTick() >= Due)
        {
            auto* C=Hero->MadnessCore.Get();
            Test->TestTrue(TEXT("Anchored echo hits original position after caster moves"), Enemy->Health()<Before);
            Test->TestTrue(TEXT("Echo is weaker than original Develop"), Before-Enemy->Health()<28);
            Test->TestTrue(TEXT("No recursive echo"), C->View().Cues.IsEmpty());
            Test->TestTrue(TEXT("Echo does not reset original cooldown"), Hero->Kit->CooldownSeconds(EDMKitSlot::E)<5);
            C->Add(100,TEXT("test.crisis"));
            Hero->SetActorLocation(FVector(-1200,0,95));
            Test->TestTrue(TEXT("Crisis Q accepted"), Hero->Primary->Request(Enemy.Get(),Enemy->GetActorLocation()));
            Hero->Primary->CancelChannel();
            Test->TestTrue(TEXT("Crisis W accepted"), Hero->Kit->Request(EDMKitSlot::W,nullptr,Enemy->GetActorLocation()));
            Test->TestEqual(TEXT("Crisis Q and W both echo"), C->View().Cues.Num(), 2);
            Enemy->DealCombatDamage(Hero.Get(),10000,TEXT("test.down")); C->Step(M->GetCombatTick());
            Test->TestTrue(TEXT("Downing cancels outstanding echoes"), C->View().Cues.IsEmpty());
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMDissociationTest, "DreadMeridian.Editor.Madness.Dissociation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMDissociationTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyDissociation(this)); return true; }
#endif
