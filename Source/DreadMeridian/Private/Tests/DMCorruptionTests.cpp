#include "DMCorruption.h"
#include "DMBossArena.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMCorruptionTest,"DreadMeridian.Foundation.PermanentCorruption",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMCorruptionTest::RunTest(const FString&)
{
    auto* W=UWorld::CreateWorld(EWorldType::Game,false); auto* A=W->SpawnActor<ADMBossArena>(); A->BuildFixture(FVector::ZeroVector);
    auto* C=W->SpawnActor<ADMCorruption>(); TestTrue(TEXT("Validated arena accepted"),C->Initialize(A));
    TestTrue(TEXT("Spread accepted"),C->Spread(FVector::ZeroVector)); TestFalse(TEXT("Duplicate cell rejected"),C->Spread(FVector(20,0,0)));
    TestFalse(TEXT("Invalid ground rejected"),C->Spread(FVector(3000,0,0))); TestTrue(TEXT("Corrupted ground query"),C->Contains(FVector(30,0,0)));
    const auto S=C->Capture(); C->Step(500); TestTrue(TEXT("Time does not remove corruption"),C->Contains(FVector::ZeroVector));
    C->Spread(FVector(300,0,0)); TestTrue(TEXT("Snapshot restore accepted"),C->Restore(S)); TestFalse(TEXT("Restoration returns original footprint"),C->Contains(FVector(300,0,0)));
    auto Bad=S; Bad.Cells.Add(FVector(5000,0,0)); TestFalse(TEXT("Invalid restore rejected atomically"),C->Restore(Bad));
    TestEqual(TEXT("Rejected restore preserves footprint"),C->Capture().Cells.Num(),S.Cells.Num()); W->DestroyWorld(false); return true;
}
#endif
