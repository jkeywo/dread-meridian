#include "DMMechanicFixture.h"
#include "DMRelicComponent.h"
#include "DMObjective.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMMedalTest,"DreadMeridian.Editor.RelicMedal",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMMedalTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Enemy=M->SpawnEncounterActor(TEXT("test.medal"),Hero->GetActorLocation()+FVector(150,0,0),1000,1); Enemy->bCommonEnemy=false; Enemy->Resolve->Reset();
        Hero->Relics->Acquire(EDMRelic::Medal,TEXT("test.medal")); Enemy->Resolve->AddPressure(100,Hero,TEXT("test.break"));
        const int32 Until=Enemy->BrokenUntilTick; const float Before=Enemy->Health(); Hero->DealCombatDamage(Enemy,20,TEXT("test.primed_ability"));
        TestTrue(TEXT("Primed ability strengthened"),Before-Enemy->Health()>20); TestTrue(TEXT("Vulnerability slightly extended"),Enemy->BrokenUntilTick>Until);
        const float After=Enemy->Health(); Hero->DealCombatDamage(Enemy,20,TEXT("test.spent")); TestEqual(TEXT("Primed payoff consumed"),After-Enemy->Health(),20.f);
    })); return true;
}
#endif

