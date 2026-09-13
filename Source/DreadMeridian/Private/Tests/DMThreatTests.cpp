#include "DMThreat.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMThreatTest,"DreadMeridian.Foundation.ThreatService",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMThreatTest::RunTest(const FString&)
{
    FDMThreat T; T.Add(TEXT("a"),20); T.Add(TEXT("b"),10); T.AtLeastHighest(TEXT("b"));
    TestEqual(TEXT("Minimum highest position"),T.Highest(),FString(TEXT("b")));
    T.Scale(TEXT("b"),.25f); TestEqual(TEXT("Threat reduction"),T.Highest(),FString(TEXT("a")));
    T.Override(TEXT("b"),10); TestEqual(TEXT("Temporary override"),T.Forced(9),FString(TEXT("b")));
    const auto S=T.Capture(); T.Empty(); TestTrue(TEXT("Restore threat"),T.Restore(S));
    T.Cleanup([](const FString& Id) { return Id!=TEXT("b"); },9);
    TestTrue(TEXT("Invalid override removed"),T.Forced(9).IsEmpty()); TestEqual(TEXT("Stale threat removed"),T.FindRef(TEXT("b")),0.f);
    T.Override(TEXT("a"),10); T.Cleanup([](const FString&) { return true; },10); TestTrue(TEXT("Override expires"),T.Forced(10).IsEmpty());
    auto Bad=S; Bad.Version=2; TestFalse(TEXT("Unknown schema rejected"),T.Restore(Bad)); return true;
}
#endif
