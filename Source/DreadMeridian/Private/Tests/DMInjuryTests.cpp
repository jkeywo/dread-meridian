#include "DMInjuryRules.h"
#include "DMRandomStreams.h"
#include "DMCombatGameMode.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMInjuryRulesTest, "DreadMeridian.Foundation.Injuries.Rules",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMInjuryRulesTest::RunTest(const FString&)
{
    FDMInjurySettings S; S.Sanitize(); FDMInjuryState A;
    TestFalse(TEXT("Shield-only hit grants no Injury"), A.RecordLoss(0, 100, 1, false, false, S));
    TestFalse(TEXT("Chip damage starts a window"), A.RecordLoss(20, 100, 10, false, false, S));
    TestFalse(TEXT("Old loss expires at exact boundary"), A.RecordLoss(20, 100, 40, false, false, S));
    TestTrue(TEXT("Concentrated Health loss crosses threshold"), A.RecordLoss(15, 100, 41, false, false, S));
    TestEqual(TEXT("Triggered loss is consumed"), A.RecentLoss(41, 30), 0.f);
    TestFalse(TEXT("Same sample cannot trigger twice"), A.RecordLoss(1, 100, 41, false, false, S));
    TestTrue(TEXT("Downing still grants an event below threshold"), A.RecordLoss(1, 100, 42, true, false, S));
    TestFalse(TEXT("Non-finite loss refused"), A.RecordLoss(NAN, 100, 43, true, false, S));
    TestTrue(TEXT("Lethal burst is one trigger"), A.RecordLoss(100, 100, 44, true, false, S));
    TestEqual(TEXT("Lethal burst consumes its window"), A.Losses.Num(), 0);
    A.Gain(0); A.Gain(0); A.Gain(0);
    TestEqual(TEXT("At most two mechanical Injuries"), A.Specific.Num(), 2);
    TestTrue(TEXT("Selection avoids duplicates"), A.Specific[0] != A.Specific[1]);
    TestEqual(TEXT("Overflow creates Grievous"), A.Grievous, 1);
    TestTrue(TEXT("Treat Grievous first"), A.Treat()); TestEqual(TEXT("Specific Injuries preserved"), A.Specific.Num(), 2);
    A.Treat(); TestEqual(TEXT("Next treatment removes one specific"), A.Specific.Num(), 1);
    A.Treat(); TestFalse(TEXT("No treatment spent on empty state"), A.Treat());

    A = FDMInjuryState(); A.Specific = { EDMInjury::BrokenRibs, EDMInjury::Burns };
    TestEqual(TEXT("First heavy hit unmodified"), A.Incoming(20, 100, 100, false, S), 1.f);
    A.RecordLoss(20, 100, 100, false, false, S);
    TestEqual(TEXT("Repeated heavy hit hurts more"), A.Incoming(20, 100, 101, false, S), S.RibsMultiplier);
    TestEqual(TEXT("Chip damage ignores ribs"), A.Incoming(5, 100, 101, false, S), 1.f);
    TestEqual(TEXT("Ribs recover after a pause"), A.Incoming(20, 100, 130, false, S), 1.f);
    A.RecordLoss(5, 100, 140, false, true, S);
    TestEqual(TEXT("Repeated hazard hurts more"), A.Incoming(5, 100, 141, true, S), S.BurnsMultiplier);
    TestEqual(TEXT("Normal attack is not hazard exposure"), A.Incoming(5, 100, 141, false, S), 1.f);
    TestEqual(TEXT("Leaving hazard resets susceptibility"), A.Incoming(5, 100, 160, true, S), 1.f);

    A = FDMInjuryState(); A.Specific = { EDMInjury::Concussion, EDMInjury::WoundedArm };
    A.Cast(100, S); TestEqual(TEXT("First cast allowed"), A.CastUntil, 0);
    A.Cast(101, S); TestEqual(TEXT("Rapid chaining forces a pause"), A.CastUntil, 111);
    A.Cast(120, S); TestTrue(TEXT("A deliberate pause avoids another lock"), A.CastUntil < 120);
    A.Attack(100, S); A.Attack(110, S); A.Attack(120, S);
    TestEqual(TEXT("Third uninterrupted basic forces recovery"), A.AttackUntil, 130);
    A.Attack(140, S); TestEqual(TEXT("Attack pause resets chain"), A.AttackChain, 1);
    A = FDMInjuryState(); A.Specific = { EDMInjury::TwistedKnee, EDMInjury::DeepCut };
    A.Move(150, 100, S); A.Move(150, 105, S);
    TestEqual(TEXT("Aggressive travel triggers limp"), A.LimpUntil, 115);
    A.RecordLoss(35, 100, 110, false, false, S);
    TestEqual(TEXT("Burst arms impaired recovery window"), A.RecoveryUntil, 140);

    // Restoring the pure rule state and stream state reproduces the next Injury and outstanding rolling window.
    FDMRandomStreams R(1927), Other(1927);
    for (int32 I = 0; I < 20; ++I) { R.Next(EDMRandomStream::Injury); }
    TestEqual(TEXT("Injury rolls do not perturb Combat"), R.Next(EDMRandomStream::Combat), Other.Next(EDMRandomStream::Combat));
    TestEqual(TEXT("Appended stable stream ID"), static_cast<int32>(EDMRandomStream::Injury), 10);
    const FDMRandomSnapshot Snapshot = R.Capture();
    TestEqual(TEXT("Versioned new snapshot"), Snapshot.SchemaVersion, 2);
    A = FDMInjuryState(); A.RecordLoss(20, 100, 10, false, false, S);
    const FDMInjuryState Saved = A;
    A.RecordLoss(15, 100, 11, false, false, S); const auto Expected = A.Gain(R.Next(EDMRandomStream::Injury));
    A = Saved; TestTrue(TEXT("Restore all stream positions"), R.Restore(Snapshot));
    TestTrue(TEXT("Restored rolling window triggers identically"), A.RecordLoss(15, 100, 11, false, false, S));
    TestTrue(TEXT("Restored Injury choice matches"), A.Gain(R.Next(EDMRandomStream::Injury)) == Expected);
    auto Old = Snapshot; Old.SchemaVersion = 1;
    const auto Before = R.Capture(); TestFalse(TEXT("Old snapshots require explicit migration"), R.Restore(Old));
    TestTrue(TEXT("Rejected restore changes no stream"), R.Capture().Seeds == Before.Seeds);
    TestTrue(TEXT("Huge Grievous counts cannot overflow revive"), ADMCombatGameMode::ReviveDurationTicks(1000000) > 0);
    TestEqual(TEXT("Negative stacks cannot shorten revive"), ADMCombatGameMode::ReviveDurationTicks(-1), 20);
    return true;
}
#endif
