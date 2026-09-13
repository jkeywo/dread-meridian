#include "DMMechanicFixture.h"
#include "DMRelicComponent.h"
#include "DMObjective.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMKnotTest,"DreadMeridian.Editor.RelicKnot",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMKnotTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Target=M->SpawnEncounterActor(TEXT("test.knot.target"),Hero->GetActorLocation()+FVector(100,0,0),100,1);
        auto* Nearby=M->SpawnEncounterActor(TEXT("test.knot.near"),Hero->GetActorLocation()+FVector(350,0,0),100,1);
        auto* Far=M->SpawnEncounterActor(TEXT("test.knot.far"),Hero->GetActorLocation()+FVector(600,0,0),100,1);
        Hero->Relics->Acquire(EDMRelic::Knot,TEXT("test.knot")); FDMControl C; C.StunTicks=10; Target->ApplyControl(C,Hero,TEXT("test.hard_cc"));
        TestTrue(TEXT("Primary takes hard control"),Target->IsStunned()); TestTrue(TEXT("Nearby takes secondary slow"),Nearby->Slows.Num()>0);
        TestFalse(TEXT("Secondary is not equal hard control"),Nearby->IsStunned()); TestTrue(TEXT("Secondary does not chain outward"),Far->Slows.IsEmpty());
    })); return true;
}
#endif

