#include "DMElderOne.h"
#include "DMRandomStreams.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMElderLifecycleTest,"DreadMeridian.Foundation.ElderLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMElderLifecycleTest::RunTest(const FString&)
{
    auto* W = UWorld::CreateWorld(EWorldType::Game,false);
    auto* A = W->SpawnActor<AActor>(); auto* B = NewObject<UDMElderOne>(A);
    FDMRandomStreams RNG(1927); const auto RandomBefore = RNG.Capture();
    TestFalse(TEXT("Cannot start before selection"),B->Begin());
    TestTrue(TEXT("Select using ElderOne stream"),B->Select(RNG.Next(EDMRandomStream::ElderOne)));
    const auto Selected = B->Capture(); TestFalse(TEXT("Cannot reroll"),B->Select(1));
    TestTrue(TEXT("Starts once"),B->Begin()); TestFalse(TEXT("Cannot skip phase"),B->Advance(EDMBossPhase::Frenzy));
    TestTrue(TEXT("Mobile transition"),B->Advance(EDMBossPhase::Mobile));
    const auto Mid = B->Capture(); TestTrue(TEXT("Frenzy"),B->Advance(EDMBossPhase::Frenzy));
    TestTrue(TEXT("Win"),B->Finish(true)); TestFalse(TEXT("Cannot overwrite outcome"),B->Finish(false));
    TestTrue(TEXT("Restore phase"),B->Restore(Mid)); TestTrue(TEXT("Loss from restored state"),B->Finish(false));
    auto Invalid = Mid; Invalid.Version++; TestFalse(TEXT("Unknown snapshot rejected"),B->Restore(Invalid));
    TestTrue(TEXT("Restore RNG"),RNG.Restore(RandomBefore));
    TestEqual(TEXT("Restored selection draw"),static_cast<uint8>(RNG.Next(EDMRandomStream::ElderOne)%2),static_cast<uint8>(Selected.Identity));
    W->DestroyWorld(false); return true;
}
#endif
