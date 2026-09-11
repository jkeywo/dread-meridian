#include "DMRandomStreams.h"
#include "DMRunState.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMRitualTest, "DreadMeridian.Foundation.RitualLifecycle",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FDMRitualTest::RunTest(const FString& Parameters)
{
    FDMRunState Run;
    TestFalse(TEXT("Cannot progress before run start"), Run.AdvanceRitual(100, 100));
    TestFalse(TEXT("Cannot finish briefing"), Run.Finish(false));
    TestTrue(TEXT("Start"), Run.Start());
    TestFalse(TEXT("No second start"), Run.Start());
    TestFalse(TEXT("Victory requires Apocalypse"), Run.Finish(true));
    TestFalse(TEXT("Negative input rejected"), Run.AdvanceRitual(-1, 100));
    TestFalse(TEXT("Zero input rejected"), Run.AdvanceRitual(0, 100));
    TestFalse(TEXT("Invalid tuning rejected"), Run.AdvanceRitual(1, 0));
    TestTrue(TEXT("Partial progress"), Run.AdvanceRitual(99, 100));
    TestEqual(TEXT("No early stage change"), Run.RitualStage, EDMRitualStage::Incipient);
    TestTrue(TEXT("Carry across multiple stages"), Run.AdvanceRitual(102, 100));
    TestEqual(TEXT("Intrusion reached"), Run.RitualStage, EDMRitualStage::Intrusion);
    TestEqual(TEXT("Remainder preserved"), Run.RitualProgress, 1);
    TestTrue(TEXT("Overflow-sized advance safely clamps"), Run.AdvanceRitual(MAX_int32, 100));
    TestEqual(TEXT("Apocalypse is not an automatic defeat"), Run.Phase, EDMRunPhase::Apocalypse);
    TestEqual(TEXT("Apocalypse progress normalized"), Run.RitualProgress, 0);
    TestFalse(TEXT("No repeat summoning"), Run.Summon());
    TestTrue(TEXT("Victory"), Run.Finish(true));
    TestFalse(TEXT("Terminal state cannot mutate"), Run.AdvanceRitual(1, 100));
    TestFalse(TEXT("Terminal result cannot flip"), Run.Finish(false));
    FDMRunState EarlyWipe;
    EarlyWipe.Start();
    TestTrue(TEXT("Early TPK allowed"), EarlyWipe.Finish(false));
    TestEqual(TEXT("TPK is defeat"), EarlyWipe.Phase, EDMRunPhase::Defeat);
    FDMRunState Summoned;
    Summoned.Start();
    TestTrue(TEXT("Deliberate summon skips ahead"), Summoned.Summon());
    TestEqual(TEXT("Summon starts Apocalypse"), Summoned.RitualStage, EDMRitualStage::Apocalypse);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMRandomTest, "DreadMeridian.Foundation.RandomStreams",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FDMRandomTest::RunTest(const FString& Parameters)
{
    FDMRandomStreams Baseline(1927), Perturbed(1927), DifferentSeed(1928);
    TestNotEqual(TEXT("Different seeds produce different state"),
        Baseline.Capture().Seeds[0], DifferentSeed.Capture().Seeds[0]);
    for (int32 Index = 0; Index < 100; ++Index)
    {
        Perturbed.Next(EDMRandomStream::Combat);
        TestEqual(TEXT("Combat rolls do not perturb relic rolls"),
            Baseline.Next(EDMRandomStream::Relics), Perturbed.Next(EDMRandomStream::Relics));
    }
    const FDMRandomSnapshot Snapshot = Perturbed.Capture();
    TArray<uint32> Expected;
    for (int32 Index = 0; Index < static_cast<int32>(EDMRandomStream::Count); ++Index)
    {
        Expected.Add(Perturbed.Next(static_cast<EDMRandomStream>(Index)));
    }
    TestTrue(TEXT("Restore valid snapshot"), Perturbed.Restore(Snapshot));
    for (int32 Index = 0; Index < Expected.Num(); ++Index)
    {
        TestEqual(TEXT("All stream positions resume exactly"), Perturbed.Next(static_cast<EDMRandomStream>(Index)), Expected[Index]);
    }
    const FDMRandomSnapshot BeforeInvalid = Perturbed.Capture();
    FDMRandomSnapshot Invalid = Snapshot;
    Invalid.SchemaVersion = 99;
    TestFalse(TEXT("Unknown schema rejected"), Perturbed.Restore(Invalid));
    Invalid = Snapshot;
    Invalid.Seeds.Pop();
    TestFalse(TEXT("Partial snapshot rejected"), Perturbed.Restore(Invalid));
    TestTrue(TEXT("Invalid restore is atomic"), Perturbed.Capture().Seeds == BeforeInvalid.Seeds);
    return true;
}
#endif
