#include "DMRelicComponent.h"
#include "DMCombatant.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMRelicInventoryTest,"DreadMeridian.Foundation.RelicInventory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMRelicInventoryTest::RunTest(const FString&)
{
    auto* W=UWorld::CreateWorld(EWorldType::Game,false); auto* A=W->SpawnActor<ADMCombatant>(); A->InitializeCombatant(TEXT("test.hero"),false,100,10);
    auto* R=NewObject<UDMRelicComponent>(A); A->AddInstanceComponent(R); R->RegisterComponent();
    TestTrue(TEXT("Acquire first reward"),R->Acquire(EDMRelic::Swagger,TEXT("cache.1")));
    TestFalse(TEXT("Cannot duplicate reward"),R->Acquire(EDMRelic::Medal,TEXT("cache.1")));
    TestFalse(TEXT("Cannot duplicate held relic"),R->Acquire(EDMRelic::Swagger,TEXT("cache.2")));
    TestTrue(TEXT("Second item fits provisional capacity"),R->Acquire(EDMRelic::Medal,TEXT("cache.2")));
    TestFalse(TEXT("Capacity enforced"),R->Acquire(EDMRelic::Knot,TEXT("cache.3")));
    const auto S=R->Capture(); TestTrue(TEXT("Inventory snapshot roundtrip"),R->Restore(S));
    auto Bad=S; Bad.Items[1]=Bad.Items[0]; TestFalse(TEXT("Duplicate snapshot rejected"),R->Restore(Bad));
    Bad=S; Bad.AwardIds.Empty(); TestFalse(TEXT("Reward provenance required"),R->Restore(Bad));
    TestTrue(TEXT("Rejected snapshots preserve state"),R->Has(EDMRelic::Medal));
    auto Full=R->CaptureFull(); Full.Runtime.PrimedTarget=TEXT("target"); Full.Runtime.PrimeUntil=15; Full.Runtime.OwnedShield=5;
    TestTrue(TEXT("Runtime budgets restore with inventory"),R->RestoreFull(Full));
    TestEqual(TEXT("Primed opportunity preserved"),R->CaptureFull().Runtime.PrimeUntil,15);
    Full.Runtime.ShieldHoldUntil=-1; TestFalse(TEXT("Invalid runtime rejected atomically"),R->RestoreFull(Full));
    TestEqual(TEXT("Rejected runtime leaves prior opportunity intact"),R->CaptureFull().Runtime.PrimeUntil,15);
    W->DestroyWorld(false); return true;
}
#endif
