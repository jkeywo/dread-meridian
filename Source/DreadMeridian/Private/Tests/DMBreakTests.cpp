#include "DMKitRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMBreakRulesTest, "DreadMeridian.Foundation.Break.Rules",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMBreakRulesTest::RunTest(const FString&)
{
    FDMBreakMeter M;
    M.Settings.MaxResolve = 80; M.Settings.BrokenTicks = 10; M.Settings.ResistTicks = 20;
    M.Settings.ResistFactor = .25f;
    TestTrue(TEXT("Authored threshold opens window"), M.Add(80, 5));
    TestEqual(TEXT("Authored window end"), M.BrokenUntil, 15);
    TestFalse(TEXT("Pressure cannot extend Broken"), M.Add(1000, 14));
    TestEqual(TEXT("Window unchanged"), M.BrokenUntil, 15);
    // Add must recover at the boundary even if the caller has not separately stepped the meter.
    TestFalse(TEXT("Boundary hit uses recovery resistance"), M.Add(80, 15));
    TestEqual(TEXT("Boundary pressure reduced"), M.Value, 20.f);
    TestEqual(TEXT("Resistance anchored to the window end"), M.ResistUntil, 35);
    TestTrue(TEXT("Rebreak requires fresh pressure"), M.Add(240, 16));
    TestTrue(TEXT("Skipped steps recover"), M.Step(100));
    TestFalse(TEXT("Skipped time does not extend resistance"), M.IsResisting(100));
    TestEqual(TEXT("Recovery clears previous pressure"), M.Value, 0.f);
    M.Settings.ResistFactor = 0;
    TestTrue(TEXT("Break after expired resistance"), M.Add(80, 100));
    M.Step(110);
    TestFalse(TEXT("Full authored resistance prevents immediate rebreak"), M.Add(100000, 110));
    TestEqual(TEXT("Full resistance banks nothing"), M.Value, 0.f);
    TestTrue(TEXT("Resistance end is exclusive"), M.Add(80, 130));

    FDMControl C; C.Slow = 1; C.SlowTicks = 20; C.StunTicks = 20; C.StaggerTicks = 5;
    C.Displacement = FVector(100, 0, 0); C.bInterrupt = true; C.BreakPressure = 15;
    const auto Common = DMKitRules::ResolveControl(C, true, false);
    TestEqual(TEXT("Commons take full stun"), Common.StunTicks, 20);
    const auto Protected = DMKitRules::ResolveControl(C, false, false, .25f, true);
    TestEqual(TEXT("Protected stun is refused"), Protected.StunTicks, 0);
    TestEqual(TEXT("Protected root converts to authored slow"), Protected.Slow, .25f);
    TestTrue(TEXT("Explicit window permits interrupt"), Protected.bInterrupt);
    TestTrue(TEXT("Interrupt window grants no displacement"), Protected.Displacement.IsZero());
    TestEqual(TEXT("Interrupt still contributes pressure"), Protected.BreakPressure, 15.f);
    const auto Broken = DMKitRules::ResolveControl(C, false, true);
    TestEqual(TEXT("Broken permits stun"), Broken.StunTicks, 20);

    FDMBreakSettings Bad; Bad.MaxResolve = NAN; Bad.BrokenTicks = -1; Bad.ResistTicks = -1;
    Bad.ResistFactor = INFINITY; Bad.ProtectedSlowFactor = -2; Bad.ControlPressure = NAN; Bad.Sanitize();
    TestTrue(TEXT("Invalid data gets a usable threshold"), FMath::IsFinite(Bad.MaxResolve) && Bad.MaxResolve > 0);
    TestEqual(TEXT("Window is at least one tick"), Bad.BrokenTicks, 1);
    TestEqual(TEXT("Resistance duration cannot be negative"), Bad.ResistTicks, 0);
    TestTrue(TEXT("Resistance factor finite"), FMath::IsFinite(Bad.ResistFactor));
    TestEqual(TEXT("Slow factor clamped"), Bad.ProtectedSlowFactor, 0.f);
    return true;
}
#endif
