#include "DMMechanicFixture.h"
#include "DMRelicComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMMorphineTest,"DreadMeridian.Editor.RelicMorphine",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMMorphineTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Enemy=M->SpawnEncounterActor(TEXT("test.morphine_hit"),Hero->GetActorLocation()+FVector(100,0,0),100,10);
        Enemy->DealCombatDamage(Hero,60,TEXT("test.initial_injury")); TestTrue(TEXT("Specific Injury acquired"),Hero->InjuryCount>0);
        Hero->Relics->Acquire(EDMRelic::Morphine,TEXT("test.morphine")); TestEqual(TEXT("Acquisition removes one specific Injury"),Hero->InjuryCount,0);
        Hero->HealHealth(1000); Enemy->DealCombatDamage(Hero,40,TEXT("test.future_injury"));
        TestEqual(TEXT("Future wound is not specific"),Hero->InjuryCount,0); TestTrue(TEXT("Grievous attrition remains"),Hero->GrievousCount>0);
        TestTrue(TEXT("Revive modifier accelerates channel"),Hero->Relics->ReviveFactor()<1);
    })); return true;
}
#endif
