#include "DMMechanicFixture.h"
#include "DMRelicComponent.h"
#include "DMObjective.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMRelicHookTest,"DreadMeridian.Editor.RelicHooks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMRelicHookTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld* W,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Enemy=M->SpawnEncounterActor(TEXT("test.hooks"),Hero->GetActorLocation()+FVector(100,0,0),500,1); Enemy->bCommonEnemy=false; Enemy->Resolve->Reset();
        int32 Controls=0,Breaks=0,Tiers=0,Objectives=0; float Excess=0;
        Hero->Relics->AcceptedControl.AddLambda([&](ADMCombatant*,const FDMControl&) { ++Controls; });
        Enemy->Relics->AcceptedBreak.AddLambda([&](ADMCombatant*,float,bool) { ++Breaks; });
        Hero->Relics->TierEntered.AddLambda([&](int32,int32) { ++Tiers; });
        Hero->Relics->ExcessHealing.AddLambda([&](float Amount) { Excess+=Amount; });
        Hero->Relics->ObjectiveFinished.AddLambda([&]() { ++Objectives; });
        FDMControl C; C.BreakPressure=-1; Enemy->ApplyControl(C,Hero,TEXT("test.invalid")); TestEqual(TEXT("Invalid control has no relic event"),Controls,0);
        C.BreakPressure=10; Enemy->ApplyControl(C,Hero,TEXT("test.accepted")); TestEqual(TEXT("Accepted control observed"),Controls,1); TestEqual(TEXT("Accepted Break contribution observed"),Breaks,1);
        Hero->HealHealth(20); TestEqual(TEXT("Excess healing observed"),Excess,20.f); Hero->HealHealth(-20); TestEqual(TEXT("Invalid heal not observed"),Excess,20.f);
        Hero->MadnessCore->Add(25,TEXT("test.tier")); TestEqual(TEXT("Upward Madness tier observed"),Tiers,1);
        auto* O=W->SpawnActor<ADMObjective>(); FDMObjectiveStep S; S.Location=Hero->GetActorLocation(); S.WorkTicks=1;
        O->Configure(TEXT("test.hook_objective"),TEXT("Operate"),{S},EDMObjectiveReward::None); O->Interact(Hero); O->Step(M->GetCombatTick()+1);
        TestEqual(TEXT("Completion observed once"),Objectives,1);
        Hero->Relics->AcceptedControl.Clear(); Enemy->Relics->AcceptedBreak.Clear(); Hero->Relics->TierEntered.Clear(); Hero->Relics->ExcessHealing.Clear(); Hero->Relics->ObjectiveFinished.Clear();
    })); return true;
}
#endif
