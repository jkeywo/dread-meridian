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
    TSet<FString> Ids;
    for (uint8 K = 1; K <= 4; ++K)
    { for (uint8 Slot = 0; Slot < 3; ++Slot)
      {
          for (uint8 N = 0; N < 6; ++N)
          {
              const auto* Entry = DMEvolution::Find(static_cast<EDMInvestigator>(K), Slot, N);
              TestNotNull(TEXT("All catalog entries exist"), Entry);
              if (Entry) { TestFalse(TEXT("Stable IDs unique"), Ids.Contains(Entry->Id)); Ids.Add(Entry->Id); }
          }
          FDMProgressionState B; B.Add(1200);
          TestFalse(TEXT("No tier skip"), B.Choose(Slot, 3));
          TestTrue(TEXT("First branch B"), B.Choose(Slot, 1));
          TestFalse(TEXT("No cross branch F"), B.Choose(Slot, 5));
          TestTrue(TEXT("Hybrid E from B"), B.Choose(Slot, 4));
          TestFalse(TEXT("No repeat charge"), B.Choose(Slot, 4));
          FDMProgressionState C; C.Add(1200); C.Choose(Slot, 2);
          TestTrue(TEXT("Hybrid E from C"), C.Choose(Slot, 4));
          TestEqual(TEXT("Shared hybrid state"), B.Node(Slot), C.Node(Slot));
      } }
    TestFalse(TEXT("Invalid slot rejected"), S.Choose(255, 1));
    TestFalse(TEXT("Invalid node rejected"), S.Choose(0, 255));
    FDMProgressionState Empty; TestFalse(TEXT("Cannot spend unearned opportunity"), Empty.Choose(0, 1));
    return true;
}
#endif
