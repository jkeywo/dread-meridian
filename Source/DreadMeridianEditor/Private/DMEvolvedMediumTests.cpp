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
class FDMVerifyMediumEvolution : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyMediumEvolution(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyMediumEvolution() { if (Settings) { Settings->RemoveFromRoot(); } }
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



            for (uint8 Node=1;Node<=5;++Node)
            {
                auto* A=W->SpawnActor<ADMCombatant>(FVector(-1200,0,95),FRotator::ZeroRotator);
                A->InitializeCombatant(FString::Printf(TEXT("test.medium.%d"),Node),false,300,10,0);
                A->InitializeInvestigator(EDMInvestigator::Medium,false); A->ControlKind=TEXT("human");
                A->GetCharacterMovement()->DisableMovement(); A->Progression->Award(TEXT("test.build"),1200);
                for(uint8 Slot=0;Slot<3;++Slot)
                { A->Progression->Choose(Slot,Node<=2?Node:Node==5?2:1); if(Node>2){A->Progression->Choose(Slot,Node);} }
                Enemy->InitializeCombatant(TEXT("test.enemy"),true,2000,10,0); Enemy->SetActorLocation(FVector(-1000,0,95));
                Hero->InitializeCombatant(TEXT("test.ally"),false,300,10,0); Hero->SetActorLocation(FVector(-1100,100,95));
                ADMCombatant* Bound=Node==2 || Node==5 ? Enemy.Get() : Hero.Get();
                Test->TestTrue(TEXT("Evolved binding accepted"),A->Primary->Request(Bound,Bound->GetActorLocation()));
                Test->TestEqual(TEXT("Binding created"),A->Primary->Bindings.Num(),1);
                if(A->Primary->Bindings.IsEmpty()){A->Destroy();continue;}
                ADMAbilityMarker* Spirit=A->Primary->Bindings[0].Get();
                A->Investigator->AddAttention(Spirit->SpiritId,100);
                if(Node==3){Enemy->DealCombatDamage(Hero.Get(),150,TEXT("test.threat"));}
                A->Primary->Step(M->GetCombatTick());
                if(Node==3){Test->TestTrue(TEXT("Vigil shields threatened ally"),Hero->Shield()>0);}
                if(Node==5){Test->TestTrue(TEXT("Possession spends Attention"),A->Investigator->PeekAttention(Spirit->SpiritId)<100);}
                A->Investigator->AddAttention(Spirit->SpiritId,100); Spirit->Attention=A->Investigator->PeekAttention(Spirit->SpiritId);
                if(Node==3){Enemy->DealCombatDamage(Hero.Get(),1000,TEXT("test.down"));}
                const float Before=Enemy->Health();
                Test->TestTrue(TEXT("Evolved Intercession accepted"),A->Kit->Request(EDMKitSlot::E,Bound,Bound->GetActorLocation()));
                if(Node==2 || Node==5){Test->TestTrue(TEXT("Hostile intervention damages"),Enemy->Health()<Before);}
                if(Node==3)
                {
                    Test->TestTrue(TEXT("Not Yet never instantly revives"),Hero->IsDown());
                    Test->TestTrue(TEXT("Not Yet supports revive channel"),Hero->Progression->ReviveTicks(100,M->GetCombatTick())<100);
                    Test->TestEqual(TEXT("Support expires"),Hero->Progression->ReviveTicks(100,M->GetCombatTick()+61),100);
                }
                if(Node==4){Test->TestTrue(TEXT("Between Worlds leaves presence"),!A->Kit->Zones.IsEmpty());}
                if(Node==5)
                {
                    auto* Second=W->SpawnActor<ADMAbilityMarker>(A->GetActorLocation(),FRotator::ZeroRotator);
                    Second->SetOwner(A); Second->bSpirit=true; Second->SpiritId=TEXT("test.second"); Second->Attention=100;
                    A->Primary->Bindings.Add(Second); A->Investigator->BindSpirit(Second->SpiritId,TEXT(""),Second->GetActorLocation());
                    A->Investigator->AddAttention(Second->SpiritId,100);
                    Hero->SetActorLocation(FVector(-900,0,95));
                }
                Test->TestTrue(TEXT("Evolved Beckon accepted"),A->Kit->Request(EDMKitSlot::W,nullptr,FVector(-900,0,95)));
                Test->TestTrue(TEXT("Spirit begins travelling"),Spirit->bTravelling);
                if(Node==1 || Node==3){Test->TestTrue(TEXT("Procession travels faster"),Spirit->TravelRate>ADMAbilityMarker::TravelSpeed);}
                if(Node==3 || Node==4){Test->TestTrue(TEXT("Route is persistent marker"),A->Kit->Zones.ContainsByPredicate([](const ADMAbilityMarker* Z){return IsValid(Z)&&Z->Shape==EDMMarkerShape::Wire;}));}
                Spirit->SetActorLocation(Spirit->TravelGoal); A->Kit->Step(M->GetCombatTick());
                Test->TestFalse(TEXT("Arrival resolves"),Spirit->bTravelling);
                if(Node==5)
                {
                    auto* Second=A->Primary->Bindings[1].Get(); Second->SetActorLocation(Second->TravelGoal);
                    const float BeforeShield=Hero->Shield(); A->Kit->Step(M->GetCombatTick());
                    Test->TestTrue(TEXT("Seance adds combined arrival payoff"),Hero->Shield()-BeforeShield>40);
                }
                A->Kit->Cancel(true); A->Destroy();
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMMediumEvolutionTest, "DreadMeridian.Editor.Evolution.Medium",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMMediumEvolutionTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyMediumEvolution(this)); return true; }
#endif
