#include "DMMechanicFixture.h"
#include "DMRelicComponent.h"
#include "DMObjective.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMSwaggerTest,"DreadMeridian.Editor.RelicSwagger",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMSwaggerTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Enemy=M->SpawnEncounterActor(TEXT("test.swagger"),Hero->GetActorLocation()+FVector(150,0,0),1000,1); Enemy->bCommonEnemy=false; Enemy->Resolve->Reset();
        ADMCombatant* Ally=nullptr; for (ADMCombatant* A : M->GetCombatants()) { if (!A->bIsEnemy && A!=Hero) { Ally=A; break; } }
        Hero->Relics->Acquire(EDMRelic::Swagger,TEXT("test.swagger")); Enemy->Threat.Add(Ally->EntityId,1000);
        FDMControl C; C.BreakPressure=10; Enemy->ApplyControl(C,Hero,TEXT("test.swagger_control"));
        TestEqual(TEXT("First control takes threat lead"),Enemy->Threat.Highest(),Hero->EntityId);
        Enemy->SetAttackTarget(Hero); const float Before=Enemy->Break; Enemy->ApplyControl(C,Ally,TEXT("test.ally_break"));
        TestTrue(TEXT("Allies gain Break effectiveness"),Enemy->Break-Before>10);
        Enemy->Threat.Add(Ally->EntityId,5000); Enemy->ApplyControl(C,Hero,TEXT("test.repeated_control"));
        TestEqual(TEXT("Threat promotion is first interaction only"),Enemy->Threat.Highest(),Ally->EntityId);
    })); return true;
}
#endif

