#include "DMMechanicFixture.h"
#include "DMRelicDrop.h"
#include "DMObjective.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMRelicAllocationTest,"DreadMeridian.Editor.RelicAllocation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMRelicAllocationTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld* W,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Inventory=Hero->FindComponentByClass<UDMRelicComponent>(); if (!TestNotNull(TEXT("Investigators own relic inventory"),Inventory)) { return; }
        auto* Roll=W->SpawnActor<ADMRelicDrop>(); TestTrue(TEXT("Reward offered"),Roll->Initialize(TEXT("test.cache")));
        TestTrue(TEXT("Need accepted"),Roll->Vote(Hero,EDMRelicChoice::Need));
        auto* Duplicate=W->SpawnActor<ADMRelicDrop>(); TestFalse(TEXT("Duplicate live award rejected"),Duplicate->Initialize(TEXT("test.cache"))); Duplicate->Destroy();
        TestFalse(TEXT("Submitted vote cannot be overwritten"),Roll->Vote(Hero,EDMRelicChoice::Pass));
        for (ADMCombatant* A : M->GetCombatants()) { if (!A->bIsEnemy && A!=Hero) { TestTrue(TEXT("Others greed"),Roll->Vote(A,EDMRelicChoice::Greed)); } }
        Roll->Step(M->GetCombatTick()+1); TestTrue(TEXT("All votes resolve"),Roll->Roll.bResolved);
        TestEqual(TEXT("Need beats every Greed"),Roll->Roll.Winner,Hero->EntityId); TestTrue(TEXT("Winner owns item"),Inventory->Has(Roll->Roll.Relic));
        Roll->Step(M->GetCombatTick()+2); TestEqual(TEXT("Reward granted once"),Inventory->Capture().Items.Num(),1);
        const auto S=Roll->Capture(); TestTrue(TEXT("Resolved roll restores"),Roll->Restore(S)); Roll->Step(M->GetCombatTick()+3);
        TestEqual(TEXT("Restored result cannot re-award"),Inventory->Capture().Items.Num(),1);
        auto* Passed=W->SpawnActor<ADMRelicDrop>(); Passed->Initialize(TEXT("test.pass"));
        for (ADMCombatant* A : M->GetCombatants()) { if (!A->bIsEnemy) { Passed->Vote(A,EDMRelicChoice::Pass); } }
        Passed->Step(M->GetCombatTick()+1); TestTrue(TEXT("All-pass discard resolves"),Passed->Roll.bResolved && Passed->Roll.Winner.IsEmpty());
        auto* Cache=W->SpawnActor<ADMObjective>(); FDMObjectiveStep Step; Step.Location=Hero->GetActorLocation(); Step.WorkTicks=1;
        Cache->Configure(TEXT("test.reward_site"),TEXT("Cache"),{Step},EDMObjectiveReward::Relic); Cache->Interact(Hero); Cache->Step(M->GetCombatTick()+1);
        TestTrue(TEXT("Objective reaches reward"),Cache->PublicState.bRewarded);
    })); return true;
}
#endif
