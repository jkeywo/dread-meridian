#include "DMProgressionComponent.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMProgressionTest, "DreadMeridian.Foundation.Progression.Cadence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMProgressionTest::RunTest(const FString&)
{
    FDMProgressionState S;
    TestEqual(TEXT("Starts at level one"), S.Level(), 1);
    TestEqual(TEXT("No pre-run choice"), S.Opportunities(), 0);
    TestFalse(TEXT("Negative XP rejected"), S.Add(-1));
    S.Add(99); TestEqual(TEXT("Threshold not reached"), S.Level(), 1);
    S.Add(1); TestEqual(TEXT("First opportunity"), S.Opportunities(), 1);
    S.Spent = 1; S.Add(100); TestEqual(TEXT("Ordinary level"), S.Opportunities(), 0);
    S.Add(MAX_int32); TestEqual(TEXT("Saturates without overflow"), S.XP, 1200);
    TestEqual(TEXT("All six choices earnable"), S.Opportunities(), 5);
    const auto Saved = S; TestFalse(TEXT("Cap stable"), S.Add(5));
    TestEqual(TEXT("Copy retains progression"), Saved.Level(), S.Level());
    TestTrue(TEXT("Ordinary levels improve basic attack"), S.BasicMultiplier() > 1.f);
    return true;
}
#endif
