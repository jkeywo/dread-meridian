#include "DMKitRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMKitRulesTest, "DreadMeridian.Foundation.Kits.Rules", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FDMKitRulesTest::RunTest(const FString&)
{
    using namespace DMKitRules;
    const FVector O(0, 0, 0), Fwd(1, 0, 0);

    // Cone: inside along the axis, inside near the edge, outside by angle, outside by length, origin counts, Z ignored.
    TestTrue(TEXT("cone axis"), PointInCone(O, Fwd, 25, 600, FVector(300, 0, 80)));
    TestTrue(TEXT("cone edge"), PointInCone(O, Fwd, 25, 600, FVector(300, 300 * FMath::Tan(FMath::DegreesToRadians(24.f)), 0)));
    TestFalse(TEXT("cone angle"), PointInCone(O, Fwd, 25, 600, FVector(300, 300 * FMath::Tan(FMath::DegreesToRadians(26.f)), 0)));
    TestFalse(TEXT("cone length"), PointInCone(O, Fwd, 25, 600, FVector(601, 0, 0)));
    TestFalse(TEXT("cone behind"), PointInCone(O, Fwd, 25, 600, FVector(-10, 0, 0)));
    TestTrue(TEXT("cone origin"), PointInCone(O, Fwd, 25, 600, FVector(0, 0, 50)));
    TestFalse(TEXT("cone zero dir"), PointInCone(O, FVector::ZeroVector, 25, 600, FVector(100, 0, 0)));

    // Segment distance.
    const FVector A(0, 0, 0), B(200, 0, 0);
    TestEqual(TEXT("segment perpendicular"), DistanceToSegment2D(A, B, FVector(100, 40, 0)), 40.f, .01f);
    TestEqual(TEXT("segment beyond end"), DistanceToSegment2D(A, B, FVector(230, 40, 0)), 50.f, .01f);
    TestEqual(TEXT("segment degenerate"), DistanceToSegment2D(A, A, FVector(30, 40, 0)), 50.f, .01f);
    TestEqual(TEXT("segment ignores z"), DistanceToSegment2D(A, B, FVector(100, 0, 500)), 0.f, .01f);

    // Crossing.
    TestTrue(TEXT("cross proper"), SegmentsCross2D(A, B, FVector(100, -50, 0), FVector(100, 50, 0)));
    TestFalse(TEXT("cross miss"), SegmentsCross2D(A, B, FVector(100, 10, 0), FVector(100, 50, 0)));
    TestTrue(TEXT("cross touching endpoint"), SegmentsCross2D(A, B, FVector(200, -50, 0), FVector(200, 50, 0)));
    TestFalse(TEXT("cross parallel"), SegmentsCross2D(A, B, FVector(0, 10, 0), FVector(200, 10, 0)));
    TestTrue(TEXT("cross collinear overlap"), SegmentsCross2D(A, B, FVector(150, 0, 0), FVector(300, 0, 0)));
    TestFalse(TEXT("cross collinear apart"), SegmentsCross2D(A, B, FVector(250, 0, 0), FVector(300, 0, 0)));

    // Wire contact: a fast crossing that never samples near the wire still triggers; standing within slack triggers; stationary far away does not.
    TestTrue(TEXT("wire fast crossing"), CrossesWire(A, B, FVector(100, -80, 0), FVector(100, 80, 0), 35));
    TestTrue(TEXT("wire proximity"), CrossesWire(A, B, FVector(100, 30, 0), FVector(100, 30, 0), 35));
    TestFalse(TEXT("wire stationary far"), CrossesWire(A, B, FVector(100, 60, 0), FVector(100, 60, 0), 35));
    TestFalse(TEXT("wire walking past the end"), CrossesWire(A, B, FVector(260, -80, 0), FVector(260, 80, 0), 35));

    // Control resolution.
    FDMControl Req; Req.Damage = 15; Req.Slow = 1; Req.SlowTicks = 20; Req.Displacement = FVector(180, 0, 0); Req.StaggerTicks = 20; Req.bInterrupt = true; Req.BreakPressure = 35;
    const FDMControl Common = ResolveControl(Req, true, false);
    TestEqual(TEXT("common slow"), Common.Slow, 1.f); TestEqual(TEXT("common stagger"), Common.StaggerTicks, 20);
    TestTrue(TEXT("common interrupt"), Common.bInterrupt); TestEqual(TEXT("common no break"), Common.BreakPressure, 0.f);
    TestEqual(TEXT("common displacement"), static_cast<float>(Common.Displacement.X), 180.f);
    const FDMControl Elite = ResolveControl(Req, false, false);
    TestEqual(TEXT("elite damage"), Elite.Damage, 15.f); TestEqual(TEXT("elite half slow"), Elite.Slow, .5f);
    TestTrue(TEXT("elite no displacement"), Elite.Displacement.IsZero()); TestEqual(TEXT("elite no stagger"), Elite.StaggerTicks, 0);
    TestFalse(TEXT("elite no interrupt"), Elite.bInterrupt); TestEqual(TEXT("elite banks pressure"), Elite.BreakPressure, 35.f);
    const FDMControl Broken = ResolveControl(Req, false, true);
    TestEqual(TEXT("broken full slow"), Broken.Slow, 1.f); TestTrue(TEXT("broken interrupt"), Broken.bInterrupt);
    TestEqual(TEXT("broken displacement"), static_cast<float>(Broken.Displacement.X), 180.f); TestEqual(TEXT("broken no more pressure"), Broken.BreakPressure, 0.f);

    // Break meter: accumulate, break at threshold, ignore pressure while broken, recover with resistance, resist halves gains.
    FDMBreakMeter M;
    TestFalse(TEXT("meter 60"), M.Add(60, 10)); TestEqual(TEXT("meter value"), M.Value, 60.f);
    TestTrue(TEXT("meter breaks"), M.Add(40, 12)); TestTrue(TEXT("meter broken"), M.IsBroken(12));
    TestFalse(TEXT("meter ignores while broken"), M.Add(50, 20)); TestEqual(TEXT("meter capped"), M.Value, FDMBreakMeter::Threshold);
    TestFalse(TEXT("meter step early"), M.Step(51)); TestTrue(TEXT("meter still broken"), M.IsBroken(51));
    TestTrue(TEXT("meter recovers"), M.Step(52)); TestFalse(TEXT("meter recovered"), M.IsBroken(52));
    TestEqual(TEXT("meter reset"), M.Value, 0.f); TestTrue(TEXT("meter resisting"), M.IsResisting(100));
    TestFalse(TEXT("meter resist gain"), M.Add(40, 100)); TestEqual(TEXT("meter halved"), M.Value, 20.f);
    TestFalse(TEXT("meter resist ends"), M.IsResisting(152));
    TestFalse(TEXT("meter ignores nonsense"), M.Add(-5, 200)); TestFalse(TEXT("meter ignores nan"), M.Add(NAN, 200));
    TestFalse(TEXT("meter step idle"), M.Step(300));

    // Shield gain is capped in total, not per gift, so repeated spirit pulses cannot outgrow the cap.
    TestEqual(TEXT("shield first gift"), ShieldGain(0, 50, 20), 20.f);
    TestEqual(TEXT("shield partial top-up"), ShieldGain(40, 50, 20), 10.f);
    TestEqual(TEXT("shield at the cap"), ShieldGain(50, 50, 20), 0.f);
    TestEqual(TEXT("shield over the cap"), ShieldGain(80, 50, 20), 0.f);
    TestEqual(TEXT("shield ignores a negative gift"), ShieldGain(0, 50, -5), 0.f);
    TestEqual(TEXT("shield ignores nan"), ShieldGain(0, 50, NAN), 0.f);

    // Slows stack by strength: a root then a weaker suppression refresh keeps the root until it expires; equal strength extends.
    TArray<FDMSlow> Slows;
    AddSlow(Slows, 1.f, 120, 100); AddSlow(Slows, .4f, 115, 105);
    TestEqual(TEXT("slow keeps the root"), EffectiveSlow(Slows, 110), 1.f);
    TestEqual(TEXT("slow falls back to suppression"), EffectiveSlow(Slows, 121), 0.f);
    AddSlow(Slows, .4f, 140, 121); AddSlow(Slows, .4f, 130, 122);
    TestEqual(TEXT("slow equal strength extends"), Slows.Num(), 1); TestEqual(TEXT("slow later expiry kept"), Slows[0].UntilTick, 140);
    AddSlow(Slows, 0.f, 200, 123); AddSlow(Slows, .3f, 100, 123); AddSlow(Slows, NAN, 200, 123);
    TestEqual(TEXT("slow ignores nonsense"), Slows.Num(), 1);
    for (int32 I = 0; I < 6; ++I) { AddSlow(Slows, .5f + I * .05f, 300, 124); }
    TestTrue(TEXT("slow bounded"), Slows.Num() <= 4); TestEqual(TEXT("slow strongest survives"), EffectiveSlow(Slows, 125), .75f, .001f);
    TestEqual(TEXT("slow all expired"), EffectiveSlow(Slows, 400), 0.f);

    // NaN inputs never trigger anything.
    const FVector Bad(NAN, 0, 0);
    TestFalse(TEXT("cone nan"), PointInCone(O, Fwd, 25, 600, Bad));
    TestFalse(TEXT("wire nan"), CrossesWire(A, B, Bad, FVector(100, 0, 0), 35));

    // Dead Ground ledger: first tag opens the batch, duplicates rejected, due after the delay, other traps join the same batch,
    // dropping a trap removes its tags, and dropping the last tag closes the batch.
    FDMDeadGroundLedger L;
    TestFalse(TEXT("ledger idle"), L.Due(0));
    TestTrue(TEXT("ledger first tag"), L.Tag(3, FDMDeadGroundLedger::Wire, 7, 100));
    TestEqual(TEXT("ledger resolve tick"), L.ResolveTick, 100 + FDMDeadGroundLedger::DelayTicks);
    TestFalse(TEXT("ledger duplicate"), L.Tag(3, FDMDeadGroundLedger::Wire, 7, 105));
    TestTrue(TEXT("ledger second enemy same trap"), L.Tag(4, FDMDeadGroundLedger::Wire, 7, 105));
    TestTrue(TEXT("ledger satchel joins"), L.Tag(3, FDMDeadGroundLedger::Satchel, 2, 110));
    TestEqual(TEXT("ledger batch keeps its tick"), L.ResolveTick, 115);
    TestFalse(TEXT("ledger not due"), L.Due(114)); TestTrue(TEXT("ledger due"), L.Due(115));
    TestTrue(TEXT("ledger has wire 7"), L.HasTag(FDMDeadGroundLedger::Wire, 7)); TestFalse(TEXT("ledger no wire 8"), L.HasTag(FDMDeadGroundLedger::Wire, 8));
    TestEqual(TEXT("ledger tags"), L.Tags.Num(), 3);
    TestEqual(TEXT("ledger drop wire"), L.DropTrap(FDMDeadGroundLedger::Wire, 7), 2);
    TestEqual(TEXT("ledger one left"), L.Tags.Num(), 1); TestEqual(TEXT("ledger batch stays open"), L.ResolveTick, 115);
    TestEqual(TEXT("ledger drop unknown"), L.DropTrap(FDMDeadGroundLedger::Wire, 99), 0);
    TestEqual(TEXT("ledger drop satchel"), L.DropTrap(FDMDeadGroundLedger::Satchel, 2), 1);
    TestFalse(TEXT("ledger closed"), L.Due(200)); TestEqual(TEXT("ledger closed tick"), L.ResolveTick, 0);
    TestTrue(TEXT("ledger reopens"), L.Tag(5, FDMDeadGroundLedger::Satchel, 2, 300)); TestEqual(TEXT("ledger new batch"), L.ResolveTick, 315);
    L.Clear(); TestFalse(TEXT("ledger cleared"), L.Due(400)); TestEqual(TEXT("ledger reset tick"), L.ResolveTick, 0);
    return true;
}

#endif
