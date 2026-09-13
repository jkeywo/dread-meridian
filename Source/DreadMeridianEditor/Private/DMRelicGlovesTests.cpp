#include "DMMechanicFixture.h"
#include "DMRelicComponent.h"
#include "DMObjective.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMGlovesTest,"DreadMeridian.Editor.RelicGloves",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMGlovesTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld* W,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        Hero->Relics->Acquire(EDMRelic::Gloves,TEXT("test.gloves"));
        auto* Enemy=M->SpawnEncounterActor(TEXT("test.gloves_enemy"),Hero->GetActorLocation()+FVector(100,0,0),500,1);
        auto* O=W->SpawnActor<ADMObjective>(); FDMObjectiveStep S; S.Verb=EDMObjectiveVerb::Operate; S.Location=Hero->GetActorLocation(); S.WorkTicks=10;
        O->Configure(TEXT("test.gloves_objective"),TEXT("Operate"),{S},EDMObjectiveReward::None); TestTrue(TEXT("Interaction starts"),O->Interact(Hero));
        TestTrue(TEXT("Gloves protect active operation"),Hero->Relics->ProtectsObjective());
        Enemy->DealCombatDamage(Hero,10,TEXT("test.ordinary_damage")); O->Step(M->GetCombatTick()+1); TestTrue(TEXT("Ordinary damage cannot interrupt"),O->IsInteracting(Hero));
        FDMControl C; C.bInterrupt=true; Hero->ApplyControl(C,Enemy,TEXT("test.explicit_interrupt")); TestFalse(TEXT("Explicit interrupt still works"),O->IsInteracting(Hero));
        ADMCombatant* Ally=nullptr; for (ADMCombatant* A : M->GetCombatants()) { if (!A->bIsEnemy && A!=Hero) { Ally=A; break; } }
        Ally->SetActorLocation(Hero->GetActorLocation()+FVector(0,80,0)); TestTrue(TEXT("Interaction resumes"),O->Interact(Hero)); O->Release(Hero);
        TestTrue(TEXT("Deliberate release grants nearby defense"),Ally->Relics->IncomingMultiplier()<1);
        const float Before=Ally->Health(); Enemy->DealCombatDamage(Ally,10,TEXT("test.gloves_defense")); TestTrue(TEXT("Defense reduces accepted melee damage"),Before-Ally->Health()<10);
        TestFalse(TEXT("Protection stops outside interaction"),Hero->Relics->ProtectsObjective());
    })); return true;
}
#endif
