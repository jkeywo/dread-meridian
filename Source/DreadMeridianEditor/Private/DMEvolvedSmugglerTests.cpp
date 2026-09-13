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
class FDMVerifySmugglerEvolution : public IAutomationLatentCommand
{
public:
    explicit FDMVerifySmugglerEvolution(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifySmugglerEvolution() { if (Settings) { Settings->RemoveFromRoot(); } }
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




            ADMCombatant* Other=nullptr;
            for(ADMCombatant* Unit:M->GetCombatants()){if(Unit->bIsEnemy && Unit!=Enemy){Other=Unit;break;}}
            for(uint8 Node=1;Node<=5;++Node)
            {
                auto* A=W->SpawnActor<ADMCombatant>(FVector(-1200,0,95),FRotator::ZeroRotator);
                A->InitializeCombatant(FString::Printf(TEXT("test.smuggler.%d"),Node),false,300,10,0);
                A->InitializeInvestigator(EDMInvestigator::Smuggler,false); A->ControlKind=TEXT("human");
                A->GetCharacterMovement()->DisableMovement(); A->Progression->Award(TEXT("test.build"),1200);
                for(uint8 Slot=0;Slot<3;++Slot)
                {A->Progression->Choose(Slot,Node<=2?Node:Node==5?2:1);if(Node>2){A->Progression->Choose(Slot,Node);}}
                Enemy->InitializeCombatant(TEXT("test.held"),true,2000,10,0); Enemy->bCommonEnemy=true;
                Other->InitializeCombatant(TEXT("test.collision"),true,2000,10,0); Other->bCommonEnemy=true;
                Enemy->SetActorLocation(FVector(-1070,0,95));Other->SetActorLocation(FVector(-800,0,95));
                Test->TestTrue(TEXT("Evolved Clinch accepted"),A->Primary->Request(Enemy.Get(),Enemy->GetActorLocation()));
                Test->TestTrue(TEXT("Clinch holds"),A->Primary->HeldTarget==Enemy.Get());
                Test->TestTrue(TEXT("Evolved throw accepted"),A->Primary->Request(nullptr,FVector(-600,0,95)));
                Test->TestFalse(TEXT("Throw releases hold"),A->Primary->HeldTarget!=nullptr);
                if(Node==3)
                {Test->TestTrue(TEXT("Personal target bonus"),A->Kit->OutgoingTo(Enemy.Get())>1);
                 Test->TestTrue(TEXT("Own threat rises"),A->Threat.FindRef(Enemy->EntityId)>=100);}
                if(Node==2 || Node==5){Test->TestTrue(TEXT("Thrown collision damages secondary"),Other->Health()<2000);}
                if(Node==4){Test->TestTrue(TEXT("Rough Handling staggers both"),Enemy->StaggeredUntilTick>0 && Other->StaggeredUntilTick>0);}
                Enemy->SetActorLocation(FVector(-1000,0,95));Other->SetActorLocation(FVector(1200,1000,95));
                Test->TestTrue(TEXT("Evolved charge accepted"),A->Kit->Request(EDMKitSlot::W,nullptr,FVector(-700,0,95)));
                A->SetActorLocation(FVector(-950,0,95)); A->Kit->Step(M->GetCombatTick()+1);
                const float After=Enemy->Health(); A->Kit->Step(M->GetCombatTick()+2);
                Test->TestEqual(TEXT("Charge cannot hit one target twice"),Enemy->Health(),After);
                if(Node==1 || Node==3){Test->TestTrue(TEXT("Head Down adds resistance"),A->Kit->ChargeResistance()>0);}
                const FVector BeforeCarry=A->GetActorLocation();
                A->Kit->Step(M->GetCombatTick()+20);
                if(Node==3){Test->TestTrue(TEXT("Single-target charge grants bonus"),A->Kit->OutgoingTo(Enemy.Get())>1); Test->TestTrue(TEXT("Single target carries Smuggler farther"),A->GetActorLocation().X>BeforeCarry.X);}
                A->SetActorLocation(FVector(-1200,0,95));Enemy->SetActorLocation(FVector(-1070,0,95));
                Test->TestTrue(TEXT("Evolved brace accepted"),A->Kit->Request(EDMKitSlot::E,nullptr,A->GetActorLocation()));
                const float Health=A->Health();
                Enemy->DealCombatDamage(A,60,TEXT("test.brace_pressure"));
                Test->TestTrue(TEXT("Brace mitigates pressure"),Health-A->Health()<60);
                Test->TestTrue(TEXT("Counter-shove accepted"),A->Kit->Request(EDMKitSlot::E,nullptr,A->GetActorLocation()));
                if(Node==3)
                {const float Hurt=A->Health();A->Kit->Step(M->GetCombatTick()+1);Test->TestTrue(TEXT("Still Standing sustains after absorption"),A->Health()>Hurt);}
                if(Node==2 || Node==5){Test->TestTrue(TEXT("Actual attacker primed"),A->Kit->OutgoingTo(Enemy.Get())>1);}
                A->Kit->Cancel(true);A->Destroy();
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMSmugglerEvolutionTest, "DreadMeridian.Editor.Evolution.Smuggler",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMSmugglerEvolutionTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifySmugglerEvolution(this)); return true; }
#endif
