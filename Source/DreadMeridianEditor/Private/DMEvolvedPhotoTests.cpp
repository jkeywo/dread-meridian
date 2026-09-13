#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMRecoverySupply.h"
#include "DMAbilityMarker.h"
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
class FDMVerifyPhotoEvolution : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyPhotoEvolution(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyPhotoEvolution() { if (Settings) { Settings->RemoveFromRoot(); } }
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


            ADMCombatant* Other = nullptr;
            for (ADMCombatant* Unit : M->GetCombatants()) { if (Unit->bIsEnemy && Unit != Enemy) { Other = Unit; break; } }
            for (uint8 Node = 1; Node <= 5; ++Node)
            {
                auto* A = W->SpawnActor<ADMCombatant>(FVector(-1200,0,95),FRotator::ZeroRotator);
                A->InitializeCombatant(FString::Printf(TEXT("test.photo.%d"),Node),false,200,10,0);
                A->InitializeInvestigator(EDMInvestigator::Photographer,false); A->ControlKind = TEXT("human");
                A->GetCharacterMovement()->DisableMovement(); A->Progression->Award(TEXT("test.build"),1200);
                for (uint8 Slot=0; Slot<3; ++Slot)
                { A->Progression->Choose(Slot,Node<=2?Node:Node==5?2:1); if(Node>2) { A->Progression->Choose(Slot,Node); } }
                Enemy->InitializeCombatant(TEXT("test.subject"),true,2000,10,0);
                Other->InitializeCombatant(TEXT("test.secondary"),true,2000,10,0); Other->Smuggler->Initialize(Enemy->Smuggler->Role);
                Enemy->SetActorLocation(FVector(-1000,0,95)); Other->SetActorLocation(FVector(-1000,100,95));
                Enemy->bTelegraphActive=true; Enemy->TelegraphEndTick=M->GetCombatTick()+10;
                Test->TestTrue(TEXT("Evolved Frame starts"), A->Primary->Request(Enemy.Get(),Enemy->GetActorLocation()));
                A->Primary->Step(M->GetCombatTick());
                Test->TestTrue(TEXT("Frame adds Exposure"), A->Investigator->PeekExposure(Enemy->EntityId)>0);
                if(Node==2 || Node==4 || Node==5) { Test->TestTrue(TEXT("Area Frame adds secondary Exposure"),A->Investigator->PeekExposure(Other->EntityId)>0); }
                if(Node==3)
                {
                    const float First=A->Investigator->PeekExposure(Enemy->EntityId); A->Primary->Step(M->GetCombatTick());
                    Test->TestTrue(TEXT("Tell bonus only once per telegraph"),A->Investigator->PeekExposure(Enemy->EntityId)-First<10);
                    Test->TestTrue(TEXT("Tell opens vulnerability"),Enemy->Progression->Vulnerability>0);
                }
                Test->TestTrue(TEXT("Evolved Flash accepted"),A->Kit->Request(EDMKitSlot::W,Enemy.Get(),Enemy->GetActorLocation()));
                if(Node==4) { Test->TestTrue(TEXT("Caught in Light improves framing window"),Enemy->Progression->FrameBonusUntil>M->GetCombatTick()); }
                A->Investigator->AddExposure(Enemy->EntityId,100,false,M->GetCombatTick());
                const float Before=Enemy->Health(), OtherBefore=Other->Health();
                Test->TestTrue(TEXT("Evolved Develop accepted"),A->Kit->Request(EDMKitSlot::E,Enemy.Get(),Enemy->GetActorLocation()));
                Test->TestTrue(TEXT("Develop damages"),Enemy->Health()<Before);
                Test->TestEqual(TEXT("Develop consumes subject Exposure"),A->Investigator->PeekExposure(Enemy->EntityId),0.f);
                if(Node==5) { Test->TestTrue(TEXT("Group Portrait shares payoff"),Other->Health()<OtherBefore); }
                if(Node==2 || Node==4 || Node==5) { Test->TestTrue(TEXT("Evidence reaches archetype peers"),Other->Progression->VulnerableUntil>M->GetCombatTick()); }
                A->Destroy();
            }
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMPhotoEvolutionTest, "DreadMeridian.Editor.Evolution.Photographer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMPhotoEvolutionTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyPhotoEvolution(this)); return true; }
#endif

