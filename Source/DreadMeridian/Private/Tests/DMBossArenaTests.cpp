#include "DMBossArena.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMBossArenaTest,"DreadMeridian.Foundation.BossMapContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMBossArenaTest::RunTest(const FString&)
{
    auto* W=UWorld::CreateWorld(EWorldType::Game,false); auto* A=W->SpawnActor<ADMBossArena>(); FString Error;
    TestFalse(TEXT("Missing affordances fail"),A->Validate(EDMElderOne::Shub,Error)); TestFalse(TEXT("Error is actionable"),Error.IsEmpty());
    A->BuildFixture(FVector(300,0,0)); TestTrue(TEXT("Shared arena supports Shub"),A->Validate(EDMElderOne::Shub,Error));
    TestTrue(TEXT("Same arena supports Nyarlathotep"),A->Validate(EDMElderOne::Nyarlathotep,Error));
    TestFalse(TEXT("Out of bounds corruption rejected"),A->Contains(TEXT("CorruptionValid"),FVector(5000,0,0)));
    A->Sites.RemoveAll([](const FDMBossAffordance& S) { return S.Kind==TEXT("GrowthSite"); });
    TestFalse(TEXT("Shub cannot use map missing growth sites"),A->Validate(EDMElderOne::Shub,Error));
    TestTrue(TEXT("Nyarlathotep does not require Shub growth sites"),A->Validate(EDMElderOne::Nyarlathotep,Error));
    W->DestroyWorld(false); return true;
}
#endif
