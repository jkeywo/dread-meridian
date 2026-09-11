#include "DMUtilityAI.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
using namespace DMUtilityAI;

namespace
{
    /** Hand-built context: self is roster entry 0 at the origin; Add appends actors, Ping appends board views, Step runs Decide and carries memory. */
    struct FRig
    {
        FDMAIWeights W;
        FDMAIContext C;
        FRig(EDMSmuggler Role, EDMInvestigator Kind, bool bEnemy)
        {
            W = DefaultWeights(Role, Kind); C.W = &W;
            C.Actors.Reserve(16); C.Pings.Reserve(8);
            FDMAISelfView& S = C.Self;
            S.Index = 0; S.EntityId = TEXT("self"); S.bEnemy = bEnemy; S.Role = Role; S.Kind = Kind;
            S.bRanged = bEnemy && Role != EDMSmuggler::None && Role != EDMSmuggler::Bruiser;
            S.Health = 100; S.MaxHealth = 100; S.AttackRange = 300; S.AttackDamage = 10; S.AttackInterval = 10;
            S.Charges = 3; S.ChargeCapacity = 3; S.bQReady = true; S.bSignatureReady = true; S.bSignatureSight = true;
            S.bWReady = true; S.bEReady = true; S.bRReady = true;
            Add(FVector::ZeroVector, bEnemy);
        }
        FRig(const FRig&) = delete;
        /** One of the bot's own persistent markers, armed unless said otherwise. */
        FDMAIMarkerView& Marker(FDMAIMarkerView::EKind Kind, FVector Location, bool bArmed = true)
        {
            FDMAIMarkerView& M = C.Markers.AddDefaulted_GetRef();
            M.Kind = Kind; M.Location = Location; M.WireEnd = Location; M.bArmed = bArmed; M.Serial = C.Markers.Num();
            return M;
        }
        FDMAIActorView& Add(FVector Location, bool bEnemy, float Health = 100)
        {
            FDMAIActorView& A = C.Actors.AddDefaulted_GetRef();
            A.Index = C.Actors.Num() - 1; A.EntityId = FString::Printf(TEXT("a%d"), A.Index);
            A.bEnemy = bEnemy; A.Health = Health; A.MaxHealth = 100; A.Location = Location; A.AttackDamage = 10; A.AttackInterval = 10;
            return A;
        }
        FDMAIPingView& Ping(EDMPingKind Kind, FVector Location, int32 Target = INDEX_NONE, bool bHuman = true)
        {
            FDMAIPingView& P = C.Pings.AddDefaulted_GetRef();
            P.Id = C.Pings.Num(); P.Kind = Kind; P.bHuman = bHuman; P.Location = Location; P.TargetIndex = Target;
            P.Weight = FMath::Clamp((bHuman ? 1.f : W.BotPingWeight) * W.PingCompliance, 0.f, 1.f);
            return P;
        }
        void Refresh()
        {
            const bool bLeader = C.Actors.IsValidIndex(C.LeaderIndex);
            const FVector Leader = bLeader ? C.Actors[C.LeaderIndex].Location : FVector::ZeroVector;
            for (FDMAIActorView& A : C.Actors)
            {
                if (A.Index == C.Self.Index) { A.Location = C.Self.Location; A.Health = C.Self.Health; A.bEnemy = C.Self.bEnemy; }
                A.Distance = FVector::Distance(C.Self.Location, A.Location); A.Distance2D = FVector::Dist2D(C.Self.Location, A.Location);
                A.AnchorDistance2D = FVector::Dist2D(C.Self.Anchor, A.Location);
                A.LeaderDistance2D = bLeader ? FVector::Dist2D(Leader, A.Location) : 0.f;
            }
        }
        FDMAIDecision Step() { Refresh(); FDMAIDecision D = Decide(C); C.Memory = D.Memory; ++C.Tick; return D; }
        int32 Focus(EDMAIRank* Rank = nullptr) { Refresh(); return ChooseFocus(C, Rank); }
        static const FDMAIOption* Find(const FDMAIDecision& D, EDMAIAction A)
        { return D.Ranked.FindByPredicate([A](const FDMAIOption& O) { return O.Action == A; }); }
    };

    bool VetoIs(const FDMAIOption* O, const TCHAR* Veto) { return O && O->Veto && FCString::Strcmp(O->Veto, Veto) == 0; }
    bool Near(float A, float B, float Tolerance = 1.e-3f) { return FMath::IsNearlyEqual(A, B, Tolerance); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMUtilityAICurvesTest, "DreadMeridian.Foundation.UtilityAI.Curves", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMUtilityAICurvesTest::RunTest(const FString& Parameters)
{
    const FDMAICurveSpec Lin(EDMAICurve::Linear, 0, 1);
    TestTrue(TEXT("Linear endpoints"), Lin.Evaluate(0) == 0 && Lin.Evaluate(1) == 1 && Near(Lin.Evaluate(.5f), .5f));
    const FDMAICurveSpec Quad(EDMAICurve::Quadratic, 0, 1, 2);
    TestTrue(TEXT("Quadratic endpoints"), Quad.Evaluate(0) == 0 && Quad.Evaluate(1) == 1 && Near(Quad.Evaluate(.5f), .25f));
    const FDMAICurveSpec InvQ(EDMAICurve::InverseQuadratic, 0, 1, 2);
    TestTrue(TEXT("InverseQuadratic endpoints"), InvQ.Evaluate(0) == 1 && InvQ.Evaluate(1) == 0 && Near(InvQ.Evaluate(.5f), .75f));
    const FDMAICurveSpec Logi(EDMAICurve::Logistic, 0, 1, 10, .5f);
    TestTrue(TEXT("Logistic renormalised endpoints"), Near(Logi.Evaluate(0), 0) && Near(Logi.Evaluate(1), 1) && Near(Logi.Evaluate(.5f), .5f, .01f));
    TestTrue(TEXT("Logistic monotonic"), Logi.Evaluate(.3f) < Logi.Evaluate(.5f) && Logi.Evaluate(.5f) < Logi.Evaluate(.7f));
    const FDMAICurveSpec Step(EDMAICurve::Step, 0, 1, 2, .5f);
    TestTrue(TEXT("Step endpoints and midpoint"), Step.Evaluate(0) == 0 && Step.Evaluate(1) == 1 && Step.Evaluate(.49f) == 0 && Step.Evaluate(.5f) == 1);
    const FDMAICurveSpec Bell(EDMAICurve::Bell, 0, 1, 4, .5f);
    TestTrue(TEXT("Bell peaks at midpoint"), Near(Bell.Evaluate(.5f), 1) && Bell.Evaluate(0) < .15f && Bell.Evaluate(1) < .15f);
    const FDMAICurveSpec Inv(EDMAICurve::Linear, 0, 1, 2, .5f, true);
    TestTrue(TEXT("Invert flips"), Inv.Evaluate(0) == 1 && Inv.Evaluate(1) == 0);
    const FDMAICurveSpec Book(EDMAICurve::Linear, 100, 500);
    TestTrue(TEXT("Bookends clamp and normalise"), Book.Evaluate(50) == 0 && Near(Book.Evaluate(300), .5f) && Book.Evaluate(900) == 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMUtilityAICombineTest, "DreadMeridian.Foundation.UtilityAI.Combine", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMUtilityAICombineTest::RunTest(const FString& Parameters)
{
    const float Two[] = { .8f, .8f };
    const float Six[] = { .8f, .8f, .8f, .8f, .8f, .8f };
    const float C2 = Combine(Two), C6 = Combine(Six);
    const float Naive6 = FMath::Pow(.8f, 6.f);
    TestTrue(TEXT("Six considerations stay competitive"), C6 > .6f && C6 > Naive6 * 2);
    TestTrue(TEXT("Fewer considerations still edge ahead"), C2 > C6);
    const float Zero[] = { .9f, 0.f, .9f };
    TestTrue(TEXT("Any zero yields zero"), Combine(Zero) == 0);
    const float One[] = { .7f };
    TestTrue(TEXT("Single consideration is unchanged"), Near(Combine(One), .7f));
    TestTrue(TEXT("Empty is neutral"), Combine(TArrayView<const float>()) == 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMUtilityAIFocusTest, "DreadMeridian.Foundation.UtilityAI.Focus", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMUtilityAIFocusTest::RunTest(const FString& Parameters)
{
    {
        FRig R(EDMSmuggler::Gunman, EDMInvestigator::None, true);
        FDMAIActorView& A = R.Add(FVector(300, 0, 0), false);
        FDMAIActorView& B = R.Add(FVector(600, 0, 0), false); B.bMarked = true;
        FDMAIActorView& Cc = R.Add(FVector(900, 0, 0), false); Cc.bDiver = true;
        FDMAIActorView& Dd = R.Add(FVector(1200, 0, 0), false); Dd.bForced = true;
        EDMAIRank Rank = EDMAIRank::Routine;
        TestTrue(TEXT("Forced is Locked"), R.Focus(&Rank) == Dd.Index && Rank == EDMAIRank::Locked);
        Dd.bForced = false;
        TestTrue(TEXT("Diver is Reflex"), R.Focus(&Rank) == Cc.Index && Rank == EDMAIRank::Reflex);
        Cc.bDiver = false;
        TestTrue(TEXT("Marked is Tactical"), R.Focus(&Rank) == B.Index && Rank == EDMAIRank::Tactical);
        B.bMarked = false;
        TestTrue(TEXT("Nearest routine otherwise"), R.Focus(&Rank) == A.Index && Rank == EDMAIRank::Routine);
    }
    {
        FRig R(EDMSmuggler::Gunman, EDMInvestigator::None, true);
        FDMAIActorView& A = R.Add(FVector(300, 0, 0), false);
        FDMAIActorView& B = R.Add(FVector(-300, 0, 0), false);
        TestEqual(TEXT("Tie goes to roster order"), R.Focus(), A.Index);
        R.C.Memory.TargetIndex = B.Index;
        TestEqual(TEXT("Commitment keeps the current target on a tie"), R.Focus(), B.Index);
    }
    {
        FRig R(EDMSmuggler::Gunman, EDMInvestigator::None, true);
        R.C.Self.bLocalEnemy = true;
        FDMAIActorView& Far = R.Add(FVector(1900, 0, 0), false); Far.Threat = 10;
        TestEqual(TEXT("Leash excludes beyond 1800 from the anchor"), R.Focus(), INDEX_NONE);
        Far.Location = FVector(1700, 0, 0);
        TestEqual(TEXT("Alerted inside the leash is a candidate"), R.Focus(), Far.Index);
    }
    {
        FRig R(EDMSmuggler::Gunman, EDMInvestigator::None, true);
        R.C.Self.bLocalEnemy = true;
        FDMAIActorView& E = R.Add(FVector(700, 0, 0), false);
        TestEqual(TEXT("Unalerted beyond sight range excluded"), R.Focus(), INDEX_NONE);
        E.Location = FVector(300, 0, 0); E.bVisible = false;
        TestEqual(TEXT("Unalerted without sight excluded"), R.Focus(), INDEX_NONE);
        E.bVisible = true;
        TestEqual(TEXT("Visible inside sight range acquired"), R.Focus(), E.Index);
        E.Location = FVector(700, 0, 0); E.bVisible = false; E.bGroupEngaged = true;
        TestEqual(TEXT("Group alert includes a camp-mate's target"), R.Focus(), E.Index);
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        R.C.Self.bCompanionTethered = true;
        FDMAIActorView& L = R.Add(FVector(0, 1000, 0), false); L.bPlayerControlled = true; R.C.LeaderIndex = L.Index;
        FDMAIActorView& E = R.Add(FVector(600, 0, 0), true);
        TestEqual(TEXT("Tether excludes far from leader and far from self"), R.Focus(), INDEX_NONE);
        E.Location = FVector(400, 0, 0);
        TestEqual(TEXT("Within 500 of self is a candidate"), R.Focus(), E.Index);
        E.Location = FVector(0, 600, 0);
        TestEqual(TEXT("Within 850 of the leader is a candidate"), R.Focus(), E.Index);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMUtilityAIChannelsTest, "DreadMeridian.Foundation.UtilityAI.Channels", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMUtilityAIChannelsTest::RunTest(const FString& Parameters)
{
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        R.C.Self.Health = 10;
        R.Add(FVector(300, 0, 0), true);
        const FDMAIDecision D = R.Step();
        TestTrue(TEXT("Flee and BasicAttack coexist"), D.Chose(EDMAIAction::Flee) && D.Chose(EDMAIAction::BasicAttack));
        TestTrue(TEXT("Both channels taken"), EnumHasAllFlags(D.Taken, EDMAIChannel::Move | EDMAIChannel::Attack) && !D.bAttackHold);
        TestEqual(TEXT("Flee owns Move"), D.ChosenOn(EDMAIChannel::Move)->Action, EDMAIAction::Flee);
    }
    {
        FRig R(EDMSmuggler::Bruiser, EDMInvestigator::None, true);
        R.C.Self.AttackRange = 120;
        FDMAIActorView& E = R.Add(FVector(150, 0, 0), false); E.bDiver = true;
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Rooted signature chosen"), D.Chose(EDMAIAction::Signature) && D.Taken == EDMAIChannel::All);
        TestTrue(TEXT("Rooted signature blocks move and attack"), !D.Chose(EDMAIAction::Engage) && !D.Chose(EDMAIAction::BasicAttack) && D.bAttackHold);
        R.C.Self.bSignatureSight = false;
        D = R.Step();
        TestTrue(TEXT("Vetoed option never chosen"), !D.Chose(EDMAIAction::Signature) && VetoIs(FRig::Find(D, EDMAIAction::Signature), TEXT("no_sight")));
        TestTrue(TEXT("Leftover channels filled"), D.Chose(EDMAIAction::Engage) && D.Chose(EDMAIAction::BasicAttack));
    }
    {
        auto Mk = [](EDMAIAction A, int32 Target, EDMAIRank Rank = EDMAIRank::Routine, const TCHAR* Veto = nullptr)
        { FDMAIOption O; O.Action = A; O.Target = Target; O.Score = .5f; O.Rank = Rank; O.Channels = EDMAIChannel::Move; O.Veto = Veto; return O; };
        TArray<FDMAIOption> Options;
        Options.Add(Mk(EDMAIAction::Anchor, INDEX_NONE));
        Options.Add(Mk(EDMAIAction::Engage, 2));
        Options.Add(Mk(EDMAIAction::Engage, 1));
        Options.Add(Mk(EDMAIAction::Rescue, INDEX_NONE, EDMAIRank::Locked, TEXT("dying")));
        FDMAIDecision D;
        Select(Options, D);
        TestTrue(TEXT("Vetoed sorts first by rank but is skipped"), D.Ranked[0].Action == EDMAIAction::Rescue && !D.Chose(EDMAIAction::Rescue));
        TestTrue(TEXT("Ties break by ordinal then target"), D.Ranked[1].Action == EDMAIAction::Engage && D.Ranked[1].Target == 1
            && D.Ranked[2].Action == EDMAIAction::Engage && D.Ranked[2].Target == 2 && D.Ranked[3].Action == EDMAIAction::Anchor);
        TestTrue(TEXT("One Move option chosen"), D.Chosen.Num() == 1 && D.Chosen[0].Target == 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMUtilityAICascadeTest, "DreadMeridian.Foundation.UtilityAI.Cascade", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMUtilityAICascadeTest::RunTest(const FString& Parameters)
{
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        R.Add(FVector(200, 0, 0), true);
        FDMAIActorView& Ally = R.Add(FVector(500, 0, 0), false); Ally.bDown = true;
        const FDMAIDecision D = R.Step();
        TestTrue(TEXT("Rescue beats everything"), D.Chosen.Num() == 1 && D.Chose(EDMAIAction::Rescue) && D.Chosen[0].Target == Ally.Index);
        TestTrue(TEXT("Rescue clears focus"), D.bClearFocus && D.Focus == INDEX_NONE && D.Taken == EDMAIChannel::All);
    }
    {
        FRig R(EDMSmuggler::Gunman, EDMInvestigator::None, true);
        R.C.Self.bLocalEnemy = true;
        R.C.Self.Location = FVector(1900, 0, 0);
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Return-home latch enters beyond 1800"), D.Memory.bReturningHome && D.Chose(EDMAIAction::ReturnHome) && D.bClearThreat && D.bClearFocus);
        TestTrue(TEXT("Return-home goal is the anchor"), D.ChosenOn(EDMAIChannel::Move)->Point.Equals(R.C.Self.Anchor));
        R.C.Self.Location = FVector(150, 0, 0);
        D = R.Step();
        TestTrue(TEXT("Latch holds inside the leash"), D.Memory.bReturningHome && D.Chose(EDMAIAction::ReturnHome));
        R.C.Self.Location = FVector(50, 0, 0);
        D = R.Step();
        TestTrue(TEXT("Latch exits under 100"), !D.Memory.bReturningHome && !D.Chose(EDMAIAction::ReturnHome));
    }
    {
        FRig R(EDMSmuggler::Gunman, EDMInvestigator::None, true);
        R.C.Self.bLocalEnemy = true; R.C.Self.bPatrolMember = true;
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Patrol member at its point advances the waypoint"), D.bAdvanceWaypoint && D.Chose(EDMAIAction::Patrol) && D.bClearThreat);
        R.C.Self.bPatrolMember = false;
        D = R.Step();
        TestTrue(TEXT("Camp member at its anchor holds and clears threat"), D.Chose(EDMAIAction::Hold) && D.bClearThreat && !D.bAdvanceWaypoint);
        R.C.Self.Location = FVector(200, 0, 0);
        D = R.Step();
        TestTrue(TEXT("Off the anchor walks back"), D.Chose(EDMAIAction::Anchor) && D.ChosenOn(EDMAIChannel::Move)->Point.Equals(R.C.Self.Anchor));
    }
    {
        FRig R(EDMSmuggler::Bomber, EDMInvestigator::None, true);
        R.C.Self.bLocalEnemy = true; R.C.Self.bCasting = true;
        R.Add(FVector(620, 0, 0), false).Threat = 10;
        const FDMAIDecision D = R.Step();
        TestTrue(TEXT("Casting enemy holds only HoldCast"), D.Chosen.Num() == 1 && D.Chose(EDMAIAction::HoldCast) && D.Focus == INDEX_NONE && D.bClearFocus && D.bAttackHold);
        TestFalse(TEXT("Casting enemy keeps its threat memory"), D.bClearThreat);
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        FDMAIActorView& Ally = R.Add(FVector(500, 0, 0), false); Ally.bDown = true;
        FDMAIHazard H; H.Center = FVector::ZeroVector; H.Radius = 100; R.C.Hazards.Add(H);
        const FDMAIDecision D = R.Step();
        TestTrue(TEXT("Rescue yields to EvadeHazard inside a hostile circle"), D.Chose(EDMAIAction::EvadeHazard) && !D.Chose(EDMAIAction::Rescue));
        R.C.Hazards.Reset();
        TestTrue(TEXT("Rescue resumes outside the circle"), R.Step().Chose(EDMAIAction::Rescue));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMUtilityAIConservationTest, "DreadMeridian.Foundation.UtilityAI.Conservation", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMUtilityAIConservationTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Threshold at 3 charges"), Near(Threshold(30, 3, 3, 8, 2, 1), 32.4f));
    TestTrue(TEXT("Threshold at 2 charges"), Near(Threshold(30, 2, 3, 8, 2, 1), 54.f));
    TestTrue(TEXT("Threshold at 1 charge"), Near(Threshold(30, 1, 3, 8, 2, 1), 75.6f));
    TestTrue(TEXT("Score below threshold is zero"), ConservedScore(1, 50, 54) == 0 && ConservedScore(1, 110, 54) == 1);
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        R.Add(FVector(300, 0, 0), true);
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Satchel placed at 3 charges"), D.Chose(EDMAIAction::PlaceSatchel));
        R.C.Self.Charges = 2; D = R.Step();
        TestTrue(TEXT("Satchel placed at 2 charges"), D.Chose(EDMAIAction::PlaceSatchel));
        R.C.Self.Charges = 1; D = R.Step();
        const FDMAIOption* Held = FRig::Find(D, EDMAIAction::PlaceSatchel);
        // KStock 2.7864 / KCooldown 0.7768 follow the baked Sapper defaults (Saved/AITuning/final): threshold 69.8 against a value of 55.
        TestTrue(TEXT("Last charge held for one enemy"), !D.Chose(EDMAIAction::PlaceSatchel) && Held && Held->Veto == nullptr && Held->Score == 0 && Near(Held->Threshold, Threshold(23, 1, 3, 8, 2.7864f, 0.7768f)) && Held->Threshold > Held->Value);
        R.Add(FVector(400, 0, 0), true); D = R.Step();
        TestTrue(TEXT("Two enemies in radius spend the last charge"), D.Chose(EDMAIAction::PlaceSatchel));
    }
    {
        FRig R(EDMSmuggler::Lookout, EDMInvestigator::None, true);
        R.C.Self.AttackRange = 900;
        R.Add(FVector(500, 0, 0), false);
        FDMAIDecision D = R.Step();
        const FDMAIOption* Mark = FRig::Find(D, EDMAIAction::Signature);
        TestTrue(TEXT("Lone lookout does not mark"), !D.Chose(EDMAIAction::Signature) && Mark && Mark->Veto == nullptr && Near(Mark->Value, 5));
        R.Add(FVector(100, 0, 0), true).bRanged = true;
        R.Add(FVector(-100, 0, 0), true).bRanged = true;
        D = R.Step();
        TestTrue(TEXT("Lookout with two ranged allies marks"), D.Chose(EDMAIAction::Signature));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        FDMAIActorView& F = R.Add(FVector(300, 0, 0), true, 10);
        FDMAIActorView& Ally = R.Add(FVector(100, 0, 0), false); Ally.AttackTargetIndex = F.Index; Ally.AttackDamage = 20;
        const FDMAIDecision D = R.Step();
        TestTrue(TEXT("Committed incoming counted"), Near(ExpectedHealthAtResolve(R.C, F.Index, 5), -10));
        TestTrue(TEXT("Dying veto"), VetoIs(FRig::Find(D, EDMAIAction::PlaceSatchel), TEXT("dying")));
    }
    {
        FRig R(EDMSmuggler::Lookout, EDMInvestigator::None, true);
        R.C.Self.AttackRange = 900;
        R.Add(FVector(500, 0, 0), false).bMarked = true;
        const FDMAIDecision D = R.Step();
        TestTrue(TEXT("Redundant veto on an already marked target"), VetoIs(FRig::Find(D, EDMAIAction::Signature), TEXT("redundant")));
    }
    // ---- Sapper named kit. One Cast channel is shared, so these also have to out-rank each other sensibly.
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        FDMAIActorView& F = R.Add(FVector(300, 0, 0), true);
        FDMAIDecision D = R.Step();
        const FDMAIOption* Zone = FRig::Find(D, EDMAIAction::SuppressingFire);
        TestTrue(TEXT("Suppression is worth casting on one enemy"), Zone && Zone->Veto == nullptr && Zone->Score > 0);
        // A cone already covering the focus adds nothing; the option must not re-fire every tick it is off cooldown.
        FDMAIMarkerView& Cone = R.Marker(FDMAIMarkerView::Zone, FVector::ZeroVector);
        Cone.Direction = FVector(1, 0, 0); Cone.HalfAngle = 25; Cone.Length = 600;
        D = R.Step();
        TestTrue(TEXT("Suppression vetoed while a live cone covers the focus"), VetoIs(FRig::Find(D, EDMAIAction::SuppressingFire), TEXT("redundant")));
        R.C.Markers.Reset();
        F.Location = FVector(900, 0, 0);
        D = R.Step();
        TestTrue(TEXT("Suppression vetoed beyond its range"), VetoIs(FRig::Find(D, EDMAIAction::SuppressingFire), TEXT("out_of_range")));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        FDMAIActorView& F = R.Add(FVector(400, 0, 0), true);
        F.Velocity2D = FVector(-300, 0, 0);   // closing on the Sapper
        FDMAIDecision D = R.Step();
        const FDMAIOption* Wire = FRig::Find(D, EDMAIAction::Tripwire);
        TestTrue(TEXT("Wire laid across an approach"), Wire && Wire->Veto == nullptr && Wire->Score > 0);
        TestTrue(TEXT("Wire carries both ends"), Wire && !Wire->Point.Equals(Wire->Point2) && Near(FVector::Dist2D(Wire->Point, Wire->Point2), 300, 1.f));
        F.Velocity2D = FVector(300, 0, 0);    // walking away
        D = R.Step();
        Wire = FRig::Find(D, EDMAIAction::Tripwire);
        TestTrue(TEXT("No wire behind a retreating enemy"), Wire && Wire->Veto == nullptr && Wire->Score == 0);
        F.Velocity2D = FVector(-300, 0, 0);
        R.C.Self.Wires = 2; D = R.Step();
        TestTrue(TEXT("Wire vetoed at the wire cap"), VetoIs(FRig::Find(D, EDMAIAction::Tripwire), TEXT("redundant")));
        R.C.Self.Wires = 0; R.C.Self.bWirePending = true; D = R.Step();
        TestTrue(TEXT("Wire vetoed while one is half placed"), VetoIs(FRig::Find(D, EDMAIAction::Tripwire), TEXT("redundant")));
        R.C.Self.bWirePending = false; F.Location = FVector(150, 0, 0); D = R.Step();
        TestTrue(TEXT("No wire in front of an enemy already past it"), VetoIs(FRig::Find(D, EDMAIAction::Tripwire), TEXT("redundant")));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        R.Add(FVector(500, 0, 0), true);
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Dead Ground needs a prepared trap"), VetoIs(FRig::Find(D, EDMAIAction::DeadGround), TEXT("redundant")));
        R.Marker(FDMAIMarkerView::Satchel, FVector(500, 0, 0));
        // Out of charges, so the satchel does not take the shared Cast channel and the ultimate is judged on its own.
        R.C.Self.Charges = 0;
        D = R.Step();
        const FDMAIOption* Ultimate = FRig::Find(D, EDMAIAction::DeadGround);
        // One common by a satchel is not worth a 60-second ultimate; the threshold has to be reachable all the same.
        TestTrue(TEXT("Dead Ground held for one enemy"), Ultimate && Ultimate->Veto == nullptr && Ultimate->Score == 0);
        TestTrue(TEXT("Ultimate threshold uses the capped cooldown"), Ultimate && Near(Ultimate->Threshold, Threshold(32, -1, 0, 150, 2.7864f, 0.7768f)));
        // Without the cap the same ultimate would need roughly two and a half times the value to fire at all.
        TestTrue(TEXT("The cap is what brings the ultimate into reach"), Threshold(32, -1, 0, 600, 2.7864f, 0.7768f) > 2.5f * Threshold(32, -1, 0, 150, 2.7864f, 0.7768f));
        R.Add(FVector(520, 0, 0), true);
        D = R.Step();
        TestTrue(TEXT("Two enemies by a trap are worth deferring"), D.Chose(EDMAIAction::DeadGround));
    }
    // ---- Photographer named kit.
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Photographer, false);
        FDMAIActorView& F = R.Add(FVector(300, 0, 0), true);
        FDMAIDecision D = R.Step();
        const FDMAIOption* Flash = FRig::Find(D, EDMAIAction::Flashbulb);
        TestTrue(TEXT("One enemy in the cone is worth a flash"), Flash && Flash->Veto == nullptr && Flash->Score > 0);
        const float Plain = Flash->Value;
        F.bCommitted = true; D = R.Step();
        Flash = FRig::Find(D, EDMAIAction::Flashbulb);
        TestTrue(TEXT("A committed subject is worth more to the flash"), Flash && Flash->Value > Plain);
        F.bCommitted = false;
        // The cone is aimed at the focus, so what limits it is reach rather than aim.
        F.Location = FVector(700, 0, 0); D = R.Step();
        TestTrue(TEXT("Nothing within reach of the flash"), VetoIs(FRig::Find(D, EDMAIAction::Flashbulb), TEXT("out_of_range")));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Photographer, false);
        FDMAIActorView& F = R.Add(FVector(300, 0, 0), true);
        F.Exposure = 20;
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Develop waits on a lightly exposed subject"), VetoIs(FRig::Find(D, EDMAIAction::Develop), TEXT("redundant")));
        F.Exposure = 60; D = R.Step();
        TestTrue(TEXT("A well exposed subject is developed"), D.Chose(EDMAIAction::Develop));
        // Inside the ultimate the stored Exposure is not spent, so a smaller reading is still worth developing.
        F.Exposure = 25; R.C.Self.bRActive = true; D = R.Step();
        const FDMAIOption* Develop = FRig::Find(D, EDMAIAction::Develop);
        TestTrue(TEXT("The ultimate lowers what Develop waits for"), Develop && Develop->Veto == nullptr);
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Photographer, false);
        R.Add(FVector(400, 0, 0), true);
        FDMAIDecision D = R.Step();
        const FDMAIOption* Photo = FRig::Find(D, EDMAIAction::ImpossiblePhotograph);
        TestTrue(TEXT("One subject is not worth the photograph"), Photo && Photo->Veto == nullptr && Photo->Score == 0);
        R.Add(FVector(450, 0, 0), true); D = R.Step();
        TestTrue(TEXT("Two visible subjects are"), D.Chose(EDMAIAction::ImpossiblePhotograph));
    }
    // ---- Medium named kit.
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        R.Add(FVector(600, 0, 0), true);
        FDMAIActorView& Ally = R.Add(FVector(-300, 0, 0), false, 20);
        R.Add(FVector(-320, 0, 0), false);
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Beckon needs spirits to call"), VetoIs(FRig::Find(D, EDMAIAction::Beckon), TEXT("redundant")));
        // A spirit out by the enemy, while a hurt ally needs it back here.
        R.Marker(FDMAIMarkerView::Spirit, FVector(600, 0, 0)).Attention = 30;
        D = R.Step();
        const FDMAIOption* Call = FRig::Find(D, EDMAIAction::Beckon);
        TestTrue(TEXT("Spirits are called to the threatened ally"), Call && Call->Veto == nullptr && Call->Score > 0);
        TestTrue(TEXT("Called to the ally, not the enemy"), Call && FVector::Dist2D(Call->Point, Ally.Location) < 1.f);
        // Already gathered where they are wanted: nothing to gain by calling again.
        R.C.Markers[0].Location = Ally.Location;
        D = R.Step();
        TestTrue(TEXT("No call when the spirits are already there"), VetoIs(FRig::Find(D, EDMAIAction::Beckon), TEXT("redundant")));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        FDMAIActorView& F = R.Add(FVector(300, 0, 0), true);
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Intercession needs a listening spirit"), VetoIs(FRig::Find(D, EDMAIAction::Intercession), TEXT("redundant")));
        // A spirit riding the enemy, with enough Attention to be worth spending.
        FDMAIMarkerView& Haunt = R.Marker(FDMAIMarkerView::Spirit, F.Location);
        Haunt.Attention = 80; Haunt.BoundIndex = F.Index;
        R.C.Self.MaxAttention = 80;
        D = R.Step();
        TestTrue(TEXT("A well attended hostile binding is spent"), D.Chose(EDMAIAction::Intercession));
        // Barely listening: worth keeping for a real intervention instead.
        Haunt.Attention = 20; R.C.Self.MaxAttention = 20;
        D = R.Step();
        const FDMAIOption* Call = FRig::Find(D, EDMAIAction::Intercession);
        TestTrue(TEXT("A barely attended spirit is kept"), Call && Call->Veto == nullptr && Call->Score == 0);
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        R.Add(FVector(300, 0, 0), true);
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Open Seance needs spirits"), VetoIs(FRig::Find(D, EDMAIAction::OpenSeance), TEXT("redundant")));
        R.Marker(FDMAIMarkerView::Spirit, FVector(300, 0, 0)).Attention = 50;
        D = R.Step();
        const FDMAIOption* Seance = FRig::Find(D, EDMAIAction::OpenSeance);
        TestTrue(TEXT("One spirit on one enemy is not a seance"), Seance && Seance->Veto == nullptr && Seance->Score == 0);
        for (int32 I = 0; I < 3; ++I) { R.Add(FVector(320 + I * 20, 40, 0), true); }
        R.Marker(FDMAIMarkerView::Spirit, FVector(320, 0, 0)).Attention = 50;
        D = R.Step();
        TestTrue(TEXT("Spirits standing in a crowd are worth manifesting"), D.Chose(EDMAIAction::OpenSeance));
    }
    // ---- Smuggler named kit.
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Smuggler, false);
        R.C.Self.AttackRange = 160;
        FDMAIActorView& F = R.Add(FVector(300, 0, 0), true);
        FDMAIDecision D = R.Step();
        const FDMAIOption* Charge = FRig::Find(D, EDMAIAction::ShoulderThrough);
        TestTrue(TEXT("A charge is worth closing one body"), Charge && Charge->Veto == nullptr && Charge->Score > 0);
        const float Alone = Charge->Value;
        // A second enemy standing on the same line is run through as well.
        R.Add(FVector(200, 0, 0), true); D = R.Step();
        Charge = FRig::Find(D, EDMAIAction::ShoulderThrough);
        TestTrue(TEXT("A charge through a line is worth more"), Charge && Charge->Value > Alone);
        F.Location = FVector(80, 0, 0); D = R.Step();
        TestTrue(TEXT("No charge at a body already in reach"), VetoIs(FRig::Find(D, EDMAIAction::ShoulderThrough), TEXT("redundant")));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Smuggler, false);
        FDMAIActorView& Shooter = R.Add(FVector(400, 0, 0), true);
        // Real Gunman numbers: damage 7 on a 1.6s cadence. One at range is not worth bracing for.
        Shooter.AttackDamage = 7; Shooter.AttackInterval = 16; Shooter.AttackTargetIndex = R.C.Self.Index;
        FDMAIDecision D = R.Step();
        const FDMAIOption* Brace = FRig::Find(D, EDMAIAction::DigIn);
        TestTrue(TEXT("One distant shooter is not worth bracing"), Brace && Brace->Veto == nullptr && Brace->Score == 0);
        FDMAIActorView& Second = R.Add(FVector(150, 0, 0), true);
        Second.AttackDamage = 7; Second.AttackInterval = 16; Second.AttackTargetIndex = R.C.Self.Index;
        D = R.Step();
        TestTrue(TEXT("Two shooters on the Smuggler are"), D.Chose(EDMAIAction::DigIn));
        R.C.Self.bBraced = true; D = R.Step();
        TestTrue(TEXT("No second brace while already braced"), VetoIs(FRig::Find(D, EDMAIAction::DigIn), TEXT("redundant")));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Smuggler, false);
        R.Add(FVector(200, 0, 0), true);
        FDMAIDecision D = R.Step();
        const FDMAIOption* Drowned = FRig::Find(D, EDMAIAction::DrownedMan);
        TestTrue(TEXT("One common is not worth the ultimate"), Drowned && Drowned->Veto == nullptr && Drowned->Score == 0);
        // An elite counts double: exactly the fight the altered state is for.
        R.C.Actors[1].bCommon = false; D = R.Step();
        TestTrue(TEXT("An elite is"), D.Chose(EDMAIAction::DrownedMan));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Smuggler, false);
        R.C.Self.AttackRange = 600;
        FDMAIActorView& F = R.Add(FVector(300, 0, 0), true);
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Clinch vetoed out of range"), VetoIs(FRig::Find(D, EDMAIAction::Clinch), TEXT("out_of_range")));
        const FDMAIOption* Engage = D.ChosenOn(EDMAIChannel::Move);
        TestTrue(TEXT("Engage approaches to Clinch range with the bonus"), Engage && Engage->Action == EDMAIAction::Engage && Near(Engage->Score, 1.2f));
        F.Location = FVector(150, 0, 0);
        D = R.Step();
        TestTrue(TEXT("Inside 160 Engage stops and Clinch fires"), !D.Chose(EDMAIAction::Engage) && D.Chose(EDMAIAction::Clinch));
    }
    {
        FRig R(EDMSmuggler::Bomber, EDMInvestigator::None, true);
        R.C.Self.AttackRange = 650;
        FDMAIActorView& F = R.Add(FVector(400, 0, 0), false);
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Firebomb on a standing target"), D.Chose(EDMAIAction::Signature));
        F.Speed2D = 420;
        D = R.Step();
        const FDMAIOption* Walk = FRig::Find(D, EDMAIAction::Signature);
        const float WalkPHit = Walk ? Walk->PHit : 1.f;
        TestTrue(TEXT("Firebomb still fires on a walking investigator"), D.Chose(EDMAIAction::Signature) && Walk && WalkPHit < 1 && WalkPHit > 0);
        F.Speed2D = 700;
        D = R.Step();
        const FDMAIOption* Bomb = FRig::Find(D, EDMAIAction::Signature);
        TestTrue(TEXT("PHit drops the firebomb on a sprinting target"), !D.Chose(EDMAIAction::Signature) && Bomb && Bomb->Veto == nullptr && Bomb->PHit < WalkPHit);
    }
    {
        FRig R(EDMSmuggler::Bruiser, EDMInvestigator::None, true);
        R.C.Self.AttackRange = 155;
        R.Add(FVector(150, 0, 0), false);
        const FDMAIDecision D = R.Step();
        const FDMAIOption* Shove = FRig::Find(D, EDMAIAction::Signature);
        TestTrue(TEXT("Bruiser shoves a plain target in range"), D.Chose(EDMAIAction::Signature) && Shove && Shove->Value > Shove->Threshold);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMUtilityAICompanionsTest, "DreadMeridian.Foundation.UtilityAI.Companions", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMUtilityAICompanionsTest::RunTest(const FString& Parameters)
{
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        R.C.Self.Charges = 0;
        R.C.Pickups.Add(FVector(400, 0, 0));
        R.Add(FVector(500, 0, 0), true);
        const FDMAIDecision D = R.Step();
        const FDMAIOption* Move = D.ChosenOn(EDMAIChannel::Move);
        TestTrue(TEXT("Empty Sapper seeks resupply over engaging"), Move && Move->Action == EDMAIAction::SeekPickup && Move->Rank == EDMAIRank::Tactical && !D.Chose(EDMAIAction::Engage));
        TestTrue(TEXT("Satchel vetoed without stock"), VetoIs(FRig::Find(D, EDMAIAction::PlaceSatchel), TEXT("no_stock")));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        FDMAIActorView& Ally = R.Add(FVector(100, 0, 0), false, 30);
        R.Add(FVector(300, 0, 0), true);
        const FDMAIDecision D = R.Step();
        const FDMAIOption* Cast = D.ChosenOn(EDMAIChannel::Cast);
        TestTrue(TEXT("Medium binds the threatened ally over the focus"), Cast && Cast->Action == EDMAIAction::BindSpirit && Cast->Target == Ally.Index);
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Photographer, false);
        FDMAIActorView& F = R.Add(FVector(300, 0, 0), true);
        F.ExposureMultiplier = 1.5f;
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Frame vetoed at exposure 1.5"), VetoIs(FRig::Find(D, EDMAIAction::Frame), TEXT("redundant")));
        F.ExposureMultiplier = 1; F.Health = 40;
        D = R.Step();
        TestTrue(TEXT("Frame vetoed under 45 health"), VetoIs(FRig::Find(D, EDMAIAction::Frame), TEXT("redundant")));
        F.Health = 100;
        D = R.Step();
        // The baked Photographer defaults (Frame Base 28.6499, KCooldown 1.6185) put the threshold at 37.9: a fresh common
        // target is worth 30 and is held without a veto, while the elite worth (x1.5724) clears it.
        const FDMAIOption* Held = FRig::Find(D, EDMAIAction::Frame);
        TestTrue(TEXT("Fresh common target is held below the Frame threshold"), !D.Chose(EDMAIAction::Frame) && Held && Held->Veto == nullptr && Held->Score == 0 && Held->Threshold > Held->Value);
        F.bCommon = false;
        D = R.Step();
        TestTrue(TEXT("Frame fires on a fresh target"), D.Chose(EDMAIAction::Frame));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        R.C.Self.bProfileRange = true; R.C.Self.Health = 10; R.C.Self.Charges = 0;
        R.C.Pickups.Add(FVector(400, 0, 0));
        R.Add(FVector(300, 0, 0), true);
        const FDMAIDecision D = R.Step();
        TestTrue(TEXT("Profile vetoes Q"), !D.Chose(EDMAIAction::PlaceSatchel) && VetoIs(FRig::Find(D, EDMAIAction::PlaceSatchel), TEXT("profile")));
        TestTrue(TEXT("Profile disables Flee"), !D.Chose(EDMAIAction::Flee) && !D.Memory.bFleeing);
        TestTrue(TEXT("Profile vetoes SeekPickup"), !D.Chose(EDMAIAction::SeekPickup) && VetoIs(FRig::Find(D, EDMAIAction::SeekPickup), TEXT("profile")));
        TestTrue(TEXT("Profile bot still attacks"), D.Chose(EDMAIAction::BasicAttack));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMUtilityAIHysteresisTest, "DreadMeridian.Foundation.UtilityAI.Hysteresis", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMUtilityAIHysteresisTest::RunTest(const FString& Parameters)
{
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        R.Add(FVector(300, 0, 0), true);
        int32 Entries = 0, Exits = 0; bool bWas = false, bFledAtTen = false;
        auto Sample = [&](float Health)
        {
            R.C.Self.Health = Health;
            const FDMAIDecision D = R.Step();
            if (D.Memory.bFleeing && !bWas) { ++Entries; }
            if (!D.Memory.bFleeing && bWas) { ++Exits; }
            bWas = D.Memory.bFleeing;
            if (Health == 10 && D.Chose(EDMAIAction::Flee)) { bFledAtTen = true; }
        };
        for (int32 H = 100; H >= 0; --H) { Sample(static_cast<float>(H)); }
        for (int32 H = 0; H <= 100; ++H) { Sample(static_cast<float>(H)); }
        TestTrue(TEXT("Health ramp produces exactly one Flee entry and exit"), Entries == 1 && Exits == 1 && bFledAtTen && !bWas);
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Sapper, false);
        FDMAIHazard H; H.Center = FVector::ZeroVector; H.Radius = 100; R.C.Hazards.Add(H);
        FDMAIActionSpec* Spec = R.W.FindAction(EDMAIAction::EvadeHazard);
        TestTrue(TEXT("EvadeHazard has no runtime or cooldown cap by default"), Spec && Spec->MaxRuntimeTicks == 0 && Spec->DecisionCooldownTicks == 0);
        bool bHeld = true;
        for (int32 I = 0; I < 40; ++I) { bHeld = bHeld && R.Step().Chose(EDMAIAction::EvadeHazard); }
        TestTrue(TEXT("Uncapped evade holds while inside the hazard"), bHeld);
        // The runtime cap and decision cooldown mechanics, on the same action with explicit caps.
        Spec->MaxRuntimeTicks = 30; Spec->DecisionCooldownTicks = 20;
        R.C.Memory = FDMAIMemory();
        bool bRan = true, bStopped = true, bCooled = true;
        for (int32 I = 0; I < 30; ++I) { bRan = bRan && R.Step().Chose(EDMAIAction::EvadeHazard); }
        FDMAIDecision D = R.Step();
        bStopped = !D.Chose(EDMAIAction::EvadeHazard) && VetoIs(FRig::Find(D, EDMAIAction::EvadeHazard), TEXT("runtime"));
        for (int32 I = 31; I < 50; ++I) { D = R.Step(); bCooled = bCooled && !D.Chose(EDMAIAction::EvadeHazard) && VetoIs(FRig::Find(D, EDMAIAction::EvadeHazard), TEXT("cooldown")); }
        D = R.Step();
        TestTrue(TEXT("Evade runs for MaxRuntimeTicks"), bRan);
        TestTrue(TEXT("Evade stops at the runtime cap"), bStopped);
        TestTrue(TEXT("Evade cannot re-fire inside the decision cooldown"), bCooled);
        TestTrue(TEXT("Evade re-fires after the cooldown"), D.Chose(EDMAIAction::EvadeHazard));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMUtilityAIPingsTest, "DreadMeridian.Foundation.UtilityAI.Pings", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMUtilityAIPingsTest::RunTest(const FString& Parameters)
{
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        FDMAIActorView& A = R.Add(FVector(300, 0, 0), true);
        FDMAIActorView& B = R.Add(FVector(-600, 0, 0), true);
        FDMAIDecision D = R.Step();
        TestEqual(TEXT("Nearest enemy first"), D.Focus, A.Index);
        const FDMAIPingView& P = R.Ping(EDMPingKind::Focus, B.Location, B.Index);
        D = R.Step();
        TestTrue(TEXT("Focus ping flips the focus in one decide"), D.Focus == B.Index && D.FocusRank == EDMAIRank::Reflex);
        TestTrue(TEXT("Focus ping answered on_it"), D.PingsOnIt.Contains(P.Id));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        FDMAIActorView& E = R.Add(FVector(200, 0, 0), true);
        R.Ping(EDMPingKind::Ignore, E.Location, E.Index);
        const FDMAIDecision D = R.Step();
        const FDMAIOption* Attack = D.ChosenOn(EDMAIChannel::Attack);
        TestTrue(TEXT("Ignored sole target still attacked"), D.Focus == E.Index && Attack && Attack->Action == EDMAIAction::BasicAttack);
        TestTrue(TEXT("Ignore lowers the act score without zeroing it"), Attack && Attack->Score > 0 && Attack->Score < .5f);
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        R.C.Self.bCompanionTethered = true;
        FDMAIActorView& L = R.Add(FVector(0, 800, 0), false); L.bPlayerControlled = true; R.C.LeaderIndex = L.Index;
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Tethered companion follows the leader"), D.Chose(EDMAIAction::FollowLeader));
        const FDMAIPingView& P = R.Ping(EDMPingKind::GoHere, FVector(500, 0, 0));
        D = R.Step();
        TestTrue(TEXT("Rally beats FollowLeader"), D.Chose(EDMAIAction::RallyToPing) && !D.Chose(EDMAIAction::FollowLeader) && D.PingsOnIt.Contains(P.Id));
        R.C.Self.Location = FVector(450, 0, 0);
        D = R.Step();
        const FDMAIOption* Hold = D.ChosenOn(EDMAIChannel::Move);
        TestTrue(TEXT("Inside the rally ring the bot holds instead of following the leader"), Hold && Hold->Action == EDMAIAction::RallyToPing && Hold->Point.Equals(R.C.Self.Location) && !D.Chose(EDMAIAction::FollowLeader) && D.PingsOnIt.Contains(P.Id));
    }
    {
        FRig R(EDMSmuggler::Gunman, EDMInvestigator::None, true);
        FDMAIActorView& E = R.Add(FVector(300, 0, 0), false);
        R.Ping(EDMPingKind::GoHere, FVector(500, 0, 0));
        R.Ping(EDMPingKind::Enemy, E.Location, E.Index);
        const FDMAIDecision D = R.Step();
        TestTrue(TEXT("Enemies neither answer nor author pings"), D.Focus == E.Index && D.PingsOnIt.Num() == 0 && D.PingRequests.Num() == 0 && !D.Chose(EDMAIAction::RallyToPing));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Photographer, false);
        // Elite target: the baked Photographer Frame threshold (37.9) holds a common target worth 30.
        R.Add(FVector(300, 0, 0), true).bCommon = false;
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("Frame fires when nothing vetoes it"), D.Chose(EDMAIAction::Frame));
        R.C.Memory.CooldownUntil.Add(EDMAIAction::Frame, R.C.Tick + 10);
        D = R.Step();
        TestTrue(TEXT("A world rejection cooldown vetoes the cast"), !D.Chose(EDMAIAction::Frame) && VetoIs(FRig::Find(D, EDMAIAction::Frame), TEXT("cooldown")));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        R.C.Self.Health = 10;
        R.Add(FVector(300, 0, 0), true);
        R.Ping(EDMPingKind::Retreat, FVector(-800, 0, 0));
        const FDMAIDecision D = R.Step();
        const FDMAIOption* Move = D.ChosenOn(EDMAIChannel::Move);
        TestTrue(TEXT("Retreat ping redirects the flee"), D.Memory.bFleeing && !D.Chose(EDMAIAction::Flee) && Move && Move->Action == EDMAIAction::RetreatToPing && Move->Point.Equals(FVector(-800, 0, 0)));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        R.Ping(EDMPingKind::Perceive, FVector(500, 0, 0));
        const FDMAIDecision D = R.Step();
        TestTrue(TEXT("Perceive investigates without a focus"), D.Chose(EDMAIAction::InvestigatePing) && D.Focus == INDEX_NONE);
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        FDMAIActorView& E = R.Add(FVector(200, 0, 0), true);
        R.Ping(EDMPingKind::Enemy, E.Location, E.Index, false);
        // The baked Medium default PingCompliance (0.8623) scales the human and bot weights alike; the bot ping stays half a human one.
        const float HumanWeight = R.W.PingCompliance, BotWeight = R.W.BotPingWeight * R.W.PingCompliance;
        TestTrue(TEXT("Bot ping weighs half"), Near(PingWeightOnTarget(R.C, E.Index, EDMPingKind::Enemy), BotWeight) && Near(BotWeight, .5f * HumanWeight));
        FDMAIDecision D = R.Step();
        const float BotScore = D.ChosenOn(EDMAIChannel::Attack)->Score;
        R.C.Pings.Reset();
        R.Ping(EDMPingKind::Enemy, E.Location, E.Index, true);
        D = R.Step();
        const float HumanScore = D.ChosenOn(EDMAIChannel::Attack)->Score;
        TestTrue(TEXT("Human ping boosts twice as much"), Near(BotScore, .5f * (1 + R.W.PingActBoost * BotWeight)) && Near(HumanScore, .5f * (1 + R.W.PingActBoost * HumanWeight)));
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        FDMAIActorView& A = R.Add(FVector(300, 0, 0), true);
        FDMAIActorView& B = R.Add(FVector(500, 0, 0), true);
        FDMAIDecision D = R.Step();
        TestTrue(TEXT("New hostile focus requests an Enemy ping"), D.PingRequests.Num() == 1 && D.PingRequests[0].Kind == EDMPingKind::Enemy && D.PingRequests[0].TargetIndex == A.Index);
        A.bDown = true;
        D = R.Step();
        TestTrue(TEXT("No second request inside the bot ping cooldown"), D.Focus == B.Index && D.PingRequests.Num() == 0);
        R.C.Tick = 60; A.bDown = false; B.bDown = true;
        D = R.Step();
        TestTrue(TEXT("Request again after the cooldown"), D.Focus == A.Index && D.PingRequests.Num() == 1);
    }
    {
        FRig R(EDMSmuggler::None, EDMInvestigator::Medium, false);
        R.C.Tick = 100; R.C.Self.Health = 10;
        R.Add(FVector(300, 0, 0), true);
        const FDMAIDecision D = R.Step();
        const bool bHelp = D.PingRequests.ContainsByPredicate([](const FDMAIPingRequest& Q) { return Q.Kind == EDMPingKind::Help && Q.TargetIndex == 0; });
        TestTrue(TEXT("Entering Flee requests Help"), D.Memory.bFleeing && bHelp);
    }
    return true;
}
#endif
