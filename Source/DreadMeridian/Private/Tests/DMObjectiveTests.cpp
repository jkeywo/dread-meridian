#include "DMObjective.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMObjectiveLifecycleTest,"DreadMeridian.Foundation.ObjectiveLifecycle",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMObjectiveLifecycleTest::RunTest(const FString& Parameters)
{
    UWorld* W = UWorld::CreateWorld(EWorldType::Game,false);
    auto* O = W->SpawnActor<ADMObjective>();
    FDMObjectiveStep S; S.Location = FVector(100,0,0);
    TestTrue(TEXT("Configure authored objective"),O->Configure(TEXT("pump"),TEXT("Pump"),{S,S},EDMObjectiveReward::DrainBasin,100));
    TestFalse(TEXT("Reject duplicate configuration"),O->Configure(TEXT("other"),TEXT("Other"),{S},EDMObjectiveReward::None));
    TestFalse(TEXT("No actor cannot interact"),O->Interact(nullptr));
    O->Step(1); TestEqual(TEXT("Final countdown"),O->PublicState.State,EDMObjectiveState::Imminent);
    O->Step(100); TestEqual(TEXT("Deadline fails"),O->PublicState.State,EDMObjectiveState::Failed);
    TestFalse(TEXT("Revealed repair needs explanation"),O->Repair({S},TEXT("")));
    TestTrue(TEXT("Explicit repair accepted"),O->Repair({S},TEXT("Use the emergency valve")));
    TestTrue(TEXT("Apocalypse preserves mechanics"),O->ConvertApocalypse());
    TestEqual(TEXT("Repair verb retained"),O->Current()->Verb,EDMObjectiveVerb::Inspect);
    auto Snapshot = O->Capture(); TestTrue(TEXT("Roundtrip accepted"),O->Restore(Snapshot));
    Snapshot.Version++; TestFalse(TEXT("Unknown version rejected"),O->Restore(Snapshot));
    Snapshot = O->Capture(); Snapshot.bRewarded = true; TestFalse(TEXT("Cannot restore unearned reward"),O->Restore(Snapshot));
    W->DestroyWorld(false); return true;
}
#endif
