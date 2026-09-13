#include "DMMadnessRules.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMMadnessRulesTest, "DreadMeridian.Foundation.Madness.Rules",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMMadnessRulesTest::RunTest(const FString&)
{
    FDMMadnessSettings S; S.Sanitize(); FDMMadnessState A;
    TestFalse(TEXT("Reject NaN pressure"), A.Add(NAN, 0, S));
    TestFalse(TEXT("Reject negative pressure"), A.Add(-1, 0, S));
    A.Add(24, 1, S); TestEqual(TEXT("Below first threshold"), A.Band(S), 0);
    A.Add(1, 1, S); TestEqual(TEXT("Threshold boundary unlocks pool"), A.Band(S), 1);
    A.Add(25, 2, S); TestEqual(TEXT("Mid pool"), A.Band(S), 2);
    A.Add(25, 3, S); TestEqual(TEXT("High pool"), A.Band(S), 3);
    A.RaiseFloor(60, 4, S); A.Recover(1000);
    TestEqual(TEXT("Recovery stops at floor"), A.Current, 60.f);
    TestFalse(TEXT("Floor cannot fall"), A.RaiseFloor(10, 4, S));
    TestFalse(TEXT("Non-finite floor rejected"), A.RaiseFloor(INFINITY, 4, S));
    A.RaiseFloor(80, 5, S); TestEqual(TEXT("Raising floor lifts current"), A.Current, 80.f);
    A.Add(1000, 10, S); TestEqual(TEXT("Maximum enters Crisis"), A.CrisisUntil, 110);
    A.Add(1, 20, S); TestEqual(TEXT("Pressure cannot extend active Crisis"), A.CrisisUntil, 110);
    A.Recover(10); TestEqual(TEXT("Recovery does not prematurely end Crisis"), A.CrisisUntil, 110);
    const auto Saved = A; A.Step(109, S); TestEqual(TEXT("Crisis persists until boundary"), A.CrisisUntil, 110);
    A.Step(110, S); TestEqual(TEXT("Crisis expires"), A.CrisisUntil, 0);
    TestEqual(TEXT("Crisis recovery respects high floor"), A.Current, 80.f);
    auto Restored = Saved; Restored.Step(110, S);
    TestEqual(TEXT("Restored Crisis result"), Restored.Current, A.Current);
    TestEqual(TEXT("Restored refractory clock"), Restored.RestUntil, A.RestUntil);
    A.RaiseFloor(100, 111, S); A.Step(209, S); TestEqual(TEXT("Maximum floor has a real Crisis respite"), A.CrisisUntil, 0);
    A.Step(210, S); TestEqual(TEXT("Maximum floor re-enters only after respite"), A.CrisisUntil, 310);
    A.Step(310, S); TestEqual(TEXT("Maximum floor never falls"), A.Current, 100.f);
    A = FDMMadnessState(); A.Add(30, 0, S); A.Step(10000, S);
    TestEqual(TEXT("No unauthored passive decay"), A.Current, 30.f);
    S.Early = 200; S.Mid = NAN; S.High = -1; S.CrisisTicks = 0; S.Sanitize();
    TestTrue(TEXT("Malformed tuning remains ordered and finite"), S.Early < S.Mid && S.Mid < S.High && S.High < 100 && S.CrisisTicks > 0);
    return true;
}
#endif
