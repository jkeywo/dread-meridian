#include "DMUtilityAI.h"
#include "DMKitRules.h"
#include "Algo/StableSort.h"

// ------------------------------------------------------------------------------------------ curves

float FDMAICurveSpec::Evaluate(float Raw) const
{
    // Min > Max is read as the reversed range (Max..Min, then flipped); only Min == Max degrades to a step at Max.
    const bool bReversed = Min > Max;
    const float Lo = bReversed ? Max : Min, Hi = bReversed ? Min : Max;
    const float Span = Hi - Lo;
    const float X = Span > UE_KINDA_SMALL_NUMBER ? FMath::Clamp((Raw - Lo) / Span, 0.f, 1.f) : (Raw >= Hi ? 1.f : 0.f);
    float Y = X;
    switch (Curve)
    {
    case EDMAICurve::Linear: Y = X; break;
    case EDMAICurve::Quadratic: Y = FMath::Pow(X, Exponent); break;
    case EDMAICurve::InverseQuadratic: Y = 1.f - FMath::Pow(X, Exponent); break;
    case EDMAICurve::Logistic:
    {
        auto L = [&](float V) { return 1.f / (1.f + FMath::Exp(-Exponent * (V - Midpoint))); };
        const float L0 = L(0.f), L1 = L(1.f);
        Y = L1 - L0 > UE_KINDA_SMALL_NUMBER ? (L(X) - L0) / (L1 - L0) : X;
        break;
    }
    case EDMAICurve::Step: Y = X >= Midpoint ? 1.f : 0.f; break;
    case EDMAICurve::Bell: Y = FMath::Exp(-FMath::Square((X - Midpoint) * Exponent)); break;
    default: break;
    }
    if (bInvert != bReversed) { Y = 1.f - Y; }
    return FMath::Clamp(Y, 0.f, 1.f);
}

// ----------------------------------------------------------------------------------------- weights

const FDMAIActionSpec* FDMAIWeights::FindAction(EDMAIAction Action) const
{ return Actions.FindByPredicate([Action](const FDMAIActionSpec& S) { return S.Action == Action; }); }
FDMAIActionSpec* FDMAIWeights::FindAction(EDMAIAction Action)
{ return Actions.FindByPredicate([Action](const FDMAIActionSpec& S) { return S.Action == Action; }); }
const FDMAIAbilityTemplate* FDMAIWeights::FindAbility(EDMAIAction Action) const { return Abilities.Find(Action); }

// ---------------------------------------------------------------------------------------- decision

const FDMAIOption* FDMAIDecision::ChosenOn(EDMAIChannel Channel) const
{
    for (const FDMAIOption& O : Chosen) { if (EnumHasAnyFlags(O.Channels, Channel)) { return &O; } }
    return nullptr;
}
bool FDMAIDecision::Chose(EDMAIAction Action) const
{ return Chosen.ContainsByPredicate([Action](const FDMAIOption& O) { return O.Action == Action; }); }

// ----------------------------------------------------------------------------------------- helpers

namespace
{
    const TCHAR* const VetoDying = TEXT("dying");
    const TCHAR* const VetoOutOfRange = TEXT("out_of_range");
    const TCHAR* const VetoRedundant = TEXT("redundant");
    const TCHAR* const VetoNoSight = TEXT("no_sight");
    const TCHAR* const VetoCooldown = TEXT("cooldown");
    const TCHAR* const VetoNoStock = TEXT("no_stock");
    const TCHAR* const VetoProfile = TEXT("profile");
    const TCHAR* const VetoRuntime = TEXT("runtime");
    /** A Sapper on its last charge grabs a pickup this close even with a focus (Routine, so it never abandons a fight). */
    constexpr float SeekPickupNearRadius = 300;
    /** Engage keeps closing to this distance while the focus is occluded, instead of parking at attack range behind a wall. */
    constexpr float OccludedApproachStop = 120;

    const TCHAR* const ActionNames[] = {
        TEXT("None"), TEXT("HoldCast"), TEXT("Rescue"), TEXT("ReturnHome"), TEXT("EvadeHazard"), TEXT("Flee"), TEXT("RetreatToPing"),
        TEXT("KeepDistance"), TEXT("SeekPickup"), TEXT("SeekPingedPickup"), TEXT("RallyToPing"), TEXT("DefendPing"), TEXT("HelpPing"),
        TEXT("Strafe"), TEXT("Engage"), TEXT("InvestigatePing"), TEXT("Anchor"), TEXT("Patrol"), TEXT("FollowLeader"), TEXT("Hold"),
        TEXT("BasicAttack"), TEXT("Signature"), TEXT("Throw"), TEXT("HoldFrame"), TEXT("Frame"), TEXT("PlaceSatchel"), TEXT("BindSpirit"), TEXT("Clinch"),
        TEXT("SuppressingFire"), TEXT("Tripwire"), TEXT("DeadGround"), TEXT("Flashbulb"), TEXT("Develop"), TEXT("ImpossiblePhotograph"),
        TEXT("Beckon"), TEXT("Intercession"), TEXT("OpenSeance"), TEXT("ShoulderThrough"), TEXT("DigIn"), TEXT("DrownedMan") };
    static_assert(static_cast<int32>(UE_ARRAY_COUNT(ActionNames)) == static_cast<int32>(EDMAIAction::Count), "ActionNames out of sync with EDMAIAction");

    const TCHAR* const InputNames[] = {
        TEXT("Distance"), TEXT("TargetHealthFrac"), TEXT("SelfHealthFrac"), TEXT("Threat"), TEXT("StockFrac"), TEXT("CooldownFrac"),
        TEXT("EnemiesInRadius"), TEXT("AlliesInRadius"), TEXT("TargetSpeedOverRadius"), TEXT("AnchorDistance"), TEXT("LeaderDistance"),
        TEXT("HazardDepth"), TEXT("PingFocusOnTarget"), TEXT("PingIgnoreOnTarget"), TEXT("PingAge"), TEXT("PingDistance"),
        TEXT("NotCasting"), TEXT("NotRooted"), TEXT("NotFraming"), TEXT("HasSight") };
    static_assert(static_cast<int32>(UE_ARRAY_COUNT(InputNames)) == static_cast<int32>(EDMAIInput::Count), "InputNames out of sync with EDMAIInput");

    bool IsAbility(EDMAIAction A)
    {
        switch (A)
        {
        case EDMAIAction::Signature: case EDMAIAction::PlaceSatchel: case EDMAIAction::BindSpirit:
        case EDMAIAction::Frame: case EDMAIAction::Clinch:
        case EDMAIAction::SuppressingFire: case EDMAIAction::Tripwire: case EDMAIAction::DeadGround:
        case EDMAIAction::Flashbulb: case EDMAIAction::Develop: case EDMAIAction::ImpossiblePhotograph:
        case EDMAIAction::Beckon: case EDMAIAction::Intercession: case EDMAIAction::OpenSeance:
        case EDMAIAction::ShoulderThrough: case EDMAIAction::DigIn: case EDMAIAction::DrownedMan:
            return true;
        default: return false;
        }
    }

    /**
     * Only these pull Engage toward a cast that is out of range. A self-cast has no range to close, and walking a
     * Photographer from rifle range into a 350-unit Flashbulb, or a Smuggler across the map for a charge, is worse
     * than not casting at all.
     */
    bool DrivesApproach(EDMAIAction A)
    {
        switch (A)
        {
        case EDMAIAction::Signature: case EDMAIAction::PlaceSatchel: case EDMAIAction::BindSpirit:
        case EDMAIAction::Frame: case EDMAIAction::Clinch: case EDMAIAction::Develop: case EDMAIAction::SuppressingFire:
            return true;
        default: return false;
        }
    }

    /** Strongest live ping of a kind (weight, then youngest, then lowest id). */
    const FDMAIPingView* BestPing(const FDMAIContext& C, EDMPingKind Kind)
    {
        const FDMAIPingView* Best = nullptr;
        for (const FDMAIPingView& P : C.Pings)
        {
            if (P.Kind != Kind) { continue; }
            if (!Best || P.Weight > Best->Weight || (P.Weight == Best->Weight && (P.AgeTicks < Best->AgeTicks || (P.AgeTicks == Best->AgeTicks && P.Id < Best->Id)))) { Best = &P; }
        }
        return Best;
    }

    float ExactPingWeight(const FDMAIContext& C, int32 Target, EDMPingKind Kind)
    {
        float Best = 0;
        for (const FDMAIPingView& P : C.Pings) { if (P.Kind == Kind && P.TargetIndex == Target && P.TargetIndex != INDEX_NONE) { Best = FMath::Max(Best, P.Weight); } }
        return Best;
    }

    /** True when A is attacking a living ally that a live Help ping names. */
    bool AttacksHelpedAlly(const FDMAIContext& C, const FDMAIActorView& A)
    {
        if (A.AttackTargetIndex == INDEX_NONE) { return false; }
        for (const FDMAIPingView& P : C.Pings)
        {
            if (P.Kind != EDMPingKind::Help || P.TargetIndex != A.AttackTargetIndex || !C.Actors.IsValidIndex(P.TargetIndex)) { continue; }
            const FDMAIActorView& Ally = C.Actors[P.TargetIndex];
            if (Ally.bEnemy == C.Self.bEnemy && !Ally.bDown) { return true; }
        }
        return false;
    }

    struct FEmit
    {
        EDMAIAction Action = EDMAIAction::None;
        const FDMAIActorView* Target = nullptr;
        FVector Point = FVector::ZeroVector;
        FVector Point2 = FVector::ZeroVector;
        const FDMAIAbilityTemplate* Tmpl = nullptr;
        const FDMAIPingView* Ping = nullptr;
        const TCHAR* Veto = nullptr;
        float Multiplier = 1;
        bool bConserved = false, bPingAct = false, bClearsFocus = false;
        float Value = 0, PHit = 1;
        int32 Stock = -1, Capacity = 0;
        EDMAIChannel Extra = EDMAIChannel::None;
    };

    struct FBuilder
    {
        const FDMAIContext& C;
        const FDMAIWeights& W;
        const FDMAISelfView& S;
        const FDMAIMemory& M;
        int32 Focus;
        TArray<FDMAIOption>& Out;

        const FDMAIActorView* Actor(int32 I) const { return C.Actors.IsValidIndex(I) ? &C.Actors[I] : nullptr; }
        bool Hostile(const FDMAIActorView& A) const { return A.bEnemy != S.bEnemy && !A.bDown; }
        bool Friendly(const FDMAIActorView& A) const { return A.bEnemy == S.bEnemy && !A.bDown && A.Index != S.Index; }
        float SelfHealthFrac() const { return S.MaxHealth > 0 ? S.Health / S.MaxHealth : 0.f; }
        FVector Clamp(FVector P, float Inset = 0) const
        {
            P.X = FMath::Clamp(P.X, -C.PlayableExtent.X + Inset, C.PlayableExtent.X - Inset);
            P.Y = FMath::Clamp(P.Y, -C.PlayableExtent.Y + Inset, C.PlayableExtent.Y - Inset);
            return P;
        }
        FVector Away(const FVector& From) const
        {
            const FVector D = (S.Location - From).GetSafeNormal2D();
            return D.IsNearlyZero() ? FVector(1, 0, 0) : D;
        }
        float Worth(const FDMAIActorView& A) const
        { return 1.f + W.EliteWorth * (A.bCommon ? 0.f : 1.f) + W.MarkedWorth * ((A.bMarked || A.bForced) ? 1.f : 0.f); }
        float HostileWorthWithin(const FVector& Point, float Radius) const
        {
            float Sum = 0;
            if (Radius <= 0) { return 0; }
            for (const FDMAIActorView& A : C.Actors) { if (Hostile(A) && FVector::Dist2D(Point, A.Location) <= Radius) { Sum += Worth(A); } }
            return Sum;
        }
        /** Any armed trap of the bot's own: Dead Ground has nothing to defer without one. */
        bool HasArmedTrap() const
        {
            for (const FDMAIMarkerView& Mk : C.Markers)
            { if (Mk.bArmed && (Mk.Kind == FDMAIMarkerView::Satchel || Mk.Kind == FDMAIMarkerView::Wire)) { return true; } }
            return false;
        }
        /** Worth of hostiles standing near any armed trap, counting each hostile once however many traps reach it. */
        float WorthNearArmedTraps(float Radius) const
        {
            float Sum = 0;
            for (const FDMAIActorView& A : C.Actors)
            {
                if (!Hostile(A)) { continue; }
                for (const FDMAIMarkerView& Mk : C.Markers)
                {
                    if (!Mk.bArmed || (Mk.Kind != FDMAIMarkerView::Satchel && Mk.Kind != FDMAIMarkerView::Wire)) { continue; }
                    const float Distance = Mk.Kind == FDMAIMarkerView::Wire
                        ? DMKitRules::DistanceToSegment2D(Mk.Location, Mk.WireEnd, A.Location)
                        : static_cast<float>(FVector::Dist2D(Mk.Location, A.Location));
                    if (Distance <= Radius) { Sum += Worth(A); break; }
                }
            }
            return Sum;
        }
        bool ArmedTrapWithin(const FVector& Point, float Radius) const
        {
            for (const FDMAIMarkerView& Mk : C.Markers)
            {
                if (!Mk.bArmed || (Mk.Kind != FDMAIMarkerView::Satchel && Mk.Kind != FDMAIMarkerView::Wire)) { continue; }
                const float Distance = Mk.Kind == FDMAIMarkerView::Wire
                    ? DMKitRules::DistanceToSegment2D(Mk.Location, Mk.WireEnd, Point)
                    : static_cast<float>(FVector::Dist2D(Mk.Location, Point));
                if (Distance <= Radius) { return true; }
            }
            return false;
        }
        /** A live suppression cone already covering the point: casting a second one there adds nothing. */
        bool ZoneCovers(const FVector& Point) const
        {
            for (const FDMAIMarkerView& Mk : C.Markers)
            { if (Mk.Kind == FDMAIMarkerView::Zone && DMKitRules::PointInCone(Mk.Location, Mk.Direction, Mk.HalfAngle, Mk.Length, Point)) { return true; } }
            return false;
        }
        /** True when the actor is closing on self rather than retreating; a wire in front only pays against an approach. */
        bool Approaching(const FDMAIActorView& A) const
        {
            const FVector Velocity = A.Velocity2D.GetSafeNormal2D();
            if (Velocity.IsNearlyZero()) { return false; }
            return FVector::DotProduct(Velocity, (S.Location - A.Location).GetSafeNormal2D()) > 0;
        }
        int32 AlliesWithin(float Radius, bool bRangedOnly) const
        {
            int32 N = 0;
            for (const FDMAIActorView& A : C.Actors) { if (Friendly(A) && (!bRangedOnly || A.bRanged) && A.Distance2D <= Radius) { ++N; } }
            return N;
        }
        const FDMAIActorView* NearestHostile() const
        {
            const FDMAIActorView* Best = nullptr;
            for (const FDMAIActorView& A : C.Actors) { if (Hostile(A) && (!Best || A.Distance2D < Best->Distance2D)) { Best = &A; } }
            return Best;
        }
        float PHitFor(const FDMAIAbilityTemplate* T, const FDMAIActorView* Target) const
        {
            if (!T || !Target || T->CastDelayTicks <= 0 || T->Radius <= 0) { return 1.f; }
            return W.PHitCurve.Evaluate(Target->Speed2D * T->CastDelayTicks * .1f / T->Radius);
        }
        float DyingCheck(const FDMAIActorView* Target, const FDMAIAbilityTemplate* T) const
        { return Target && T ? DMUtilityAI::ExpectedHealthAtResolve(C, Target->Index, T->CastDelayTicks) : 1.f; }

        float Raw(EDMAIInput In, const FDMAIOption& O, const FDMAIActorView* T, const FDMAIAbilityTemplate* Tmpl, const FDMAIPingView* Ping) const
        {
            switch (In)
            {
            case EDMAIInput::Distance: return T ? T->Distance2D : FVector::Dist2D(S.Location, O.Point);
            case EDMAIInput::TargetHealthFrac: return T && T->MaxHealth > 0 ? T->Health / T->MaxHealth : 0.f;
            case EDMAIInput::SelfHealthFrac: return SelfHealthFrac();
            case EDMAIInput::Threat: return T ? T->Threat : 0.f;
            case EDMAIInput::StockFrac: return S.ChargeCapacity > 0 ? static_cast<float>(S.Charges) / S.ChargeCapacity : 0.f;
            case EDMAIInput::CooldownFrac:
                if (Tmpl && Tmpl->CooldownTicks > 0) { return FMath::Clamp(static_cast<float>(S.QCooldownRemaining) / Tmpl->CooldownTicks, 0.f, 1.f); }
                return S.bQReady ? 0.f : 1.f;
            case EDMAIInput::EnemiesInRadius:
            {
                if (!Tmpl || Tmpl->Radius <= 0) { return T && Hostile(*T) ? 1.f : 0.f; }
                int32 N = 0;
                for (const FDMAIActorView& A : C.Actors) { if (Hostile(A) && FVector::Dist2D(O.Point, A.Location) <= Tmpl->Radius) { ++N; } }
                return static_cast<float>(N);
            }
            case EDMAIInput::AlliesInRadius: return static_cast<float>(AlliesWithin(W.AllyRadius, false));
            case EDMAIInput::TargetSpeedOverRadius:
                return T && Tmpl && Tmpl->CastDelayTicks > 0 && Tmpl->Radius > 0 ? T->Speed2D * Tmpl->CastDelayTicks * .1f / Tmpl->Radius : 0.f;
            case EDMAIInput::AnchorDistance: return FVector::Dist2D(S.Location, S.Anchor);
            case EDMAIInput::LeaderDistance: return C.Actors.IsValidIndex(C.LeaderIndex) ? C.Actors[C.LeaderIndex].Distance2D : 0.f;
            case EDMAIInput::HazardDepth:
            {
                float Depth = 0;
                for (const FDMAIHazard& H : C.Hazards) { Depth = FMath::Max(Depth, H.Radius + W.EvadeMargin - static_cast<float>(FVector::Dist2D(S.Location, H.Center))); }
                return Depth;
            }
            case EDMAIInput::PingFocusOnTarget: return T ? DMUtilityAI::PingWeightOnTarget(C, T->Index, EDMPingKind::Focus) : 0.f;
            case EDMAIInput::PingIgnoreOnTarget: return T ? DMUtilityAI::PingWeightOnTarget(C, T->Index, EDMPingKind::Ignore) : 0.f;
            case EDMAIInput::PingAge: return Ping ? Ping->AgeFrac : 0.f;
            case EDMAIInput::PingDistance: return Ping ? FVector::Dist2D(S.Location, Ping->Location) : 0.f;
            case EDMAIInput::NotCasting: return S.bCasting ? 0.f : 1.f;
            case EDMAIInput::NotRooted: return S.bRestrained || S.FrameTarget != INDEX_NONE ? 0.f : 1.f;
            case EDMAIInput::NotFraming: return S.FrameTarget != INDEX_NONE ? 0.f : 1.f;
            case EDMAIInput::HasSight: return !T || T->bVisible ? 1.f : 0.f;
            default: return 0.f;
            }
        }

        FDMAIOption* Emit(const FDMAIActionSpec* Spec, const FEmit& E)
        {
            if (!Spec || !Spec->bEnabled) { return nullptr; }
            FDMAIOption& O = Out.AddDefaulted_GetRef();
            O.Action = E.Action;
            O.Target = E.Target ? E.Target->Index : INDEX_NONE;
            O.Point = E.Point.IsNearlyZero() && E.Target ? E.Target->Location : E.Point;
            O.Point2 = E.Point2;
            O.Channels = Spec->Channels() | E.Extra;
            O.Rank = Spec->Rank;
            O.PingId = E.Ping ? E.Ping->Id : INDEX_NONE;
            O.bClearsFocus = E.bClearsFocus;
            O.Value = E.Value; O.PHit = E.PHit;
            if (E.bConserved && E.Tmpl) { O.Threshold = DMUtilityAI::Threshold(E.Tmpl->Base, E.Stock, E.Capacity, E.Tmpl->ThresholdCooldown(), W.KStock, W.KCooldown); }
            if (E.Veto) { O.Veto = E.Veto; O.Score = 0; return &O; }

            TArray<float, TInlineAllocator<8>> Scores;
            O.Breakdown.Reserve(Spec->Considerations.Num() + 2);
            for (const FDMAIConsideration& Con : Spec->Considerations)
            {
                const float R = Raw(Con.Input, O, E.Target, E.Tmpl, E.Ping);
                const float Sc = Con.Curve.Evaluate(R);
                O.Breakdown.Add({ Con.Input, R, Sc });
                Scores.Add(Sc);
                if (Sc <= 0) { break; }
            }
            const float Base = DMUtilityAI::Combine(TArrayView<const float>(Scores.GetData(), Scores.Num()));
            float Score = E.bConserved ? DMUtilityAI::ConservedScore(Spec->Weight, E.Value * E.PHit, O.Threshold) * Base : Spec->Weight * Base;
            Score *= E.Multiplier;
            if (E.bPingAct && E.Target)
            {
                const float Fw = DMUtilityAI::PingWeightOnTarget(C, E.Target->Index, EDMPingKind::Focus);
                const float Iw = DMUtilityAI::PingWeightOnTarget(C, E.Target->Index, EDMPingKind::Ignore);
                if (Fw > 0) { const float Mul = 1.f + W.PingActBoost * Fw; Score *= Mul; O.Breakdown.Add({ EDMAIInput::PingFocusOnTarget, Fw, Mul }); }
                if (Iw > 0) { const float Mul = FMath::Max(0.f, 1.f - W.PingIgnoreActPenalty * Iw); Score *= Mul; O.Breakdown.Add({ EDMAIInput::PingIgnoreOnTarget, Iw, Mul }); }
            }
            if (Spec->MaxRuntimeTicks > 0)
            {
                const float X = static_cast<float>(M.RuntimeTicks.FindRef(E.Action)) / Spec->MaxRuntimeTicks;
                const float Mul = 1.f - FMath::Pow(FMath::Clamp(X, 0.f, 1.f), 6.f);
                if (Mul <= 0) { O.Veto = VetoRuntime; O.Score = 0; return &O; }
                Score *= Mul;
            }
            // Decision cooldowns are a hard veto: rank ordering would let a suppressed-but-positive score re-fire at once.
            if (C.Tick < M.CooldownUntil.FindRef(E.Action)) { O.Veto = VetoCooldown; O.Score = 0; return &O; }
            if (EnumHasAnyFlags(O.Channels, EDMAIChannel::Move) && E.Action == M.LastMove && C.Tick - M.MoveSinceTick < W.CommitTicks) { Score *= W.CommitFactor; }
            O.Score = FMath::Max(0.f, Score);
            return &O;
        }

        FDMAIOption* Simple(EDMAIAction A, const FDMAIActorView* T, FVector Point, bool bClearsFocus = false, const FDMAIPingView* Ping = nullptr, float Multiplier = 1)
        {
            FEmit E; E.Action = A; E.Target = T; E.Point = Point; E.bClearsFocus = bClearsFocus; E.Ping = Ping; E.Multiplier = Multiplier;
            return Emit(W.FindAction(A), E);
        }

        void Ability(EDMAIAction A, const FDMAIActorView* T, FVector Point, const TCHAR* Veto, float Value, int32 Stock, int32 Capacity, FVector Point2 = FVector::ZeroVector)
        {
            const FDMAIAbilityTemplate* Tmpl = W.FindAbility(A);
            if (!Tmpl) { return; }
            FEmit E; E.Action = A; E.Target = T; E.Point = Point; E.Point2 = Point2; E.Tmpl = Tmpl; E.Veto = Veto; E.bConserved = true; E.bPingAct = T && Hostile(*T);
            E.Value = Value; E.PHit = PHitFor(Tmpl, T); E.Stock = Stock; E.Capacity = Capacity;
            E.Extra = Tmpl->bRoots ? (EDMAIChannel::Move | EDMAIChannel::Attack) : EDMAIChannel::None;
            Emit(W.FindAction(A), E);
        }

        void Run()
        {
            // A charge owns every channel until it ends: the world refuses other casts anyway, and a rejected one
            // would pick up a decision cooldown that outlives the charge.
            if (S.bCasting || S.bCharging)
            {
                static const FDMAIActionSpec Fallback = [] { FDMAIActionSpec Sp; Sp.Action = EDMAIAction::HoldCast; Sp.Rank = EDMAIRank::Locked; Sp.ChannelMask = static_cast<uint8>(EDMAIChannel::All); return Sp; }();
                const FDMAIActionSpec* Spec = W.FindAction(EDMAIAction::HoldCast);
                FEmit E; E.Action = EDMAIAction::HoldCast; E.Point = S.Location; E.bClearsFocus = true;
                Emit(Spec && Spec->bEnabled ? Spec : &Fallback, E);
                return;
            }
            const FDMAIActorView* F = Actor(Focus);
            const bool bCompanion = !S.bEnemy;
            const FDMAIPingView* Retreat = bCompanion ? BestPing(C, EDMPingKind::Retreat) : nullptr;

            // Rescue: first downed ally in roster order.
            if (bCompanion)
            {
                for (const FDMAIActorView& A : C.Actors)
                { if (A.bEnemy == S.bEnemy && A.Index != S.Index && A.bDown) { Simple(EDMAIAction::Rescue, &A, A.Location, true); break; } }
            }
            if (S.bLocalEnemy && M.bReturningHome) { Simple(EDMAIAction::ReturnHome, nullptr, S.Anchor, true); }

            // EvadeHazard: step out of the nearest hostile circle we stand in.
            {
                const FDMAIHazard* In = nullptr; float BestDepth = 0;
                for (const FDMAIHazard& H : C.Hazards)
                {
                    const float Depth = H.Radius + W.EvadeMargin - FVector::Dist2D(S.Location, H.Center);
                    if (Depth > 0 && (!In || Depth > BestDepth)) { In = &H; BestDepth = Depth; }
                }
                if (In)
                {
                    const FVector Dir = Away(In->Center);
                    Simple(EDMAIAction::EvadeHazard, nullptr, Clamp(In->Center + Dir * (In->Radius + W.EvadeMargin + 20)));
                }
            }

            // Flee: latched in Decide; a live Retreat ping replaces its goal.
            if (bCompanion && !S.bProfileRange && M.bFleeing && !Retreat)
            {
                const FDMAIActorView* Enemy = NearestHostile();
                const FVector Dir = Enemy ? Away(Enemy->Location) : FVector(1, 0, 0);
                const FDMAIActorView* Ally = C.Actors.IsValidIndex(C.LeaderIndex) && Friendly(C.Actors[C.LeaderIndex]) ? &C.Actors[C.LeaderIndex] : nullptr;
                if (!Ally) { for (const FDMAIActorView& A : C.Actors) { if (Friendly(A)) { Ally = &A; break; } } }
                Simple(EDMAIAction::Flee, nullptr, Clamp(Ally ? Ally->Location + Dir * 150 : S.Location + Dir * W.FleeDistance));
            }
            if (bCompanion)
            {
                for (const FDMAIPingView& P : C.Pings)
                { if (P.Kind == EDMPingKind::Retreat && FVector::Dist2D(S.Location, P.Location) > W.RallyArriveRadius) { Simple(EDMAIAction::RetreatToPing, nullptr, P.Location, false, &P); } }
            }
            if (const FDMAIActorView* Held = Actor(S.HeldTarget))
            { Simple(EDMAIAction::Throw, Held, Held->Location + (Held->Location - S.Location).GetSafeNormal2D() * 300); }
            if (const FDMAIActorView* Framed = Actor(S.FrameTarget))
            { if (!Framed->bDown && Framed->ExposureMultiplier < 2) { Simple(EDMAIAction::HoldFrame, Framed, S.Location); } }

            // KeepDistance: ranged enemies back off from the nearest investigator while latched.
            if (S.bEnemy && M.bKeepingDistance)
            {
                if (const FDMAIActorView* Near = NearestHostile())
                {
                    const float Mul = S.Role == EDMSmuggler::Gunman && S.bSetPosition ? 1.f - W.SetPositionReluctance : 1.f;
                    Simple(EDMAIAction::KeepDistance, Near, Clamp(S.Location + Away(Near->Location) * W.KeepDistanceStep), false, nullptr, Mul);
                }
            }
            if (S.Role == EDMSmuggler::GangBoss && F && F->Distance2D < W.StrafeRadius)
            {
                const float Side = (C.Tick / 30) % 2 ? -1.f : 1.f;
                const FVector Offset = (S.Location - F->Location).GetSafeNormal2D().RotateAngleAxis(Side * W.StrafeAngle, FVector::UpVector) * W.StrafeOffset;
                Simple(EDMAIAction::Strafe, F, Clamp(F->Location + Offset, 80));
            }

            // Signature (enemies) through the conservation template.
            if (S.bEnemy && F)
            {
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::Signature))
                {
                    // Precedence: out_of_range first so Engage can approach to template range even while on cooldown.
                    const TCHAR* Veto = nullptr;
                    if (F->Distance2D > T->Range) { Veto = VetoOutOfRange; }
                    else if (!S.bSignatureSight) { Veto = VetoNoSight; }
                    else if ((S.Role == EDMSmuggler::Lookout || S.Role == EDMSmuggler::GangBoss) && (F->bMarked || F->bForced)) { Veto = VetoRedundant; }
                    else if (DyingCheck(F, T) <= 0) { Veto = VetoDying; }
                    else if (!S.bSignatureReady) { Veto = VetoCooldown; }
                    float Value = 0;
                    switch (S.Role)
                    {
                    case EDMSmuggler::Bruiser: Value = (T->Magnitude + (F->bDiver ? T->SecondaryMagnitude : 0.f)) * Worth(*F); break;
                    case EDMSmuggler::Bomber: Value = T->Magnitude * HostileWorthWithin(F->Location, T->Radius); break;
                    case EDMSmuggler::Lookout: Value = (T->Magnitude + T->SecondaryMagnitude * AlliesWithin(W.AllyRadius, true)) * Worth(*F); break;
                    case EDMSmuggler::GangBoss: Value = (T->Magnitude + T->SecondaryMagnitude * AlliesWithin(W.AllyRadius, false)) * Worth(*F); break;
                    default: Value = T->Magnitude * Worth(*F); break;
                    }
                    Ability(EDMAIAction::Signature, F, F->Location, Veto, Value, -1, 0);
                }
            }

            // Base Q (companions).
            if (bCompanion && S.FrameTarget == INDEX_NONE && S.HeldTarget == INDEX_NONE)
            {
                // Precedence: profile, no_stock, out_of_range, redundant, dying, cooldown (out_of_range ahead of cooldown so Engage approaches).
                auto Finish = [&](const TCHAR* Veto, const FDMAIActorView* T, const FDMAIAbilityTemplate* Tmpl, bool bRedundant, bool bDying) -> const TCHAR*
                {
                    if (!Veto && T->Distance2D > Tmpl->Range) { Veto = VetoOutOfRange; }
                    if (!Veto && bRedundant) { Veto = VetoRedundant; }
                    if (!Veto && bDying && DyingCheck(T, Tmpl) <= 0) { Veto = VetoDying; }
                    if (!Veto && !S.bQReady) { Veto = VetoCooldown; }
                    return Veto;
                };
                const TCHAR* const Profile = S.bProfileRange ? VetoProfile : nullptr;
                if (S.Kind == EDMInvestigator::Sapper && F)
                {
                    if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::PlaceSatchel))
                    {
                        const TCHAR* Veto = Finish(Profile ? Profile : S.Charges < 1 ? VetoNoStock : nullptr, F, T, S.Satchels > 0, true);
                        Ability(EDMAIAction::PlaceSatchel, F, F->Location, Veto, T->Magnitude * HostileWorthWithin(F->Location, T->Radius), S.Charges, S.ChargeCapacity);
                    }
                }
                else if (S.Kind == EDMInvestigator::Medium)
                {
                    if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::BindSpirit))
                    {
                        for (const FDMAIActorView& A : C.Actors)
                        {
                            if (A.bEnemy != S.bEnemy || A.bDown || A.Health >= W.ThreatenedAllyHealth) { continue; }
                            Ability(EDMAIAction::BindSpirit, &A, A.Location, Finish(Profile, &A, T, A.bBoundByMe, false), T->Magnitude * Worth(A), -1, 0);
                        }
                        if (F)
                        { Ability(EDMAIAction::BindSpirit, F, F->Location, Finish(Profile, F, T, F->bBoundByMe, true), T->SecondaryMagnitude * Worth(*F), -1, 0); }
                    }
                }
                else if (S.Kind == EDMInvestigator::Photographer && F)
                {
                    if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::Frame))
                    {
                        const TCHAR* Veto = Finish(Profile, F, T, F->ExposureMultiplier >= 1.5f || F->Health < 45, true);
                        const float Value = (1.5f - F->ExposureMultiplier) / .5f * T->Magnitude * FMath::Min(1.f, F->Health / 45.f) * Worth(*F);
                        Ability(EDMAIAction::Frame, F, F->Location, Veto, Value, -1, 0);
                    }
                }
                else if (S.Kind == EDMInvestigator::Smuggler && F)
                {
                    if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::Clinch))
                    {
                        const TCHAR* Veto = Finish(Profile, F, T, F->bRestrained || (!F->bCommon && !F->bBreakVulnerable), true);
                        // Worth applies here too: an elite's break window is exactly where the multiplier matters.
                        Ability(EDMAIAction::Clinch, F, F->Location, Veto, (T->Magnitude + F->AttackDamage * 15.f / FMath::Max(1, F->AttackInterval)) * Worth(*F), -1, 0);
                    }
                }
            }

            // Named kits (companions). Separate from the base-Q block: these do not share Q's cooldown or its
            // framing/holding gate, and each ability carries its own readiness flag.
            if (bCompanion && !S.bProfileRange && S.Kind == EDMInvestigator::Sapper)
            {
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::SuppressingFire); T && F)
                {
                    const TCHAR* Veto = F->Distance2D > T->Range ? VetoOutOfRange
                        : ZoneCovers(F->Location) ? VetoRedundant
                        : DyingCheck(F, T) <= 0 ? VetoDying
                        : !S.bWReady ? VetoCooldown : nullptr;
                    // Worth more where the Sapper has already prepared ground: suppression exists to hold enemies in it.
                    const float Value = T->Magnitude * HostileWorthWithin(F->Location, T->Radius) * (ArmedTrapWithin(F->Location, 300) ? 1.5f : 1.f);
                    Ability(EDMAIAction::SuppressingFire, F, F->Location, Veto, Value, -1, 0);
                }
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::Tripwire); T && F)
                {
                    // Laid across the approach line, far enough ahead that an advancing enemy still has to cross it.
                    const FVector Dir = (F->Location - S.Location).GetSafeNormal2D();
                    const FVector Perp(-Dir.Y, Dir.X, 0);
                    const FVector Mid = S.Location + Dir * 250;
                    const FVector A = Clamp(Mid - Perp * T->Radius, 60), B = Clamp(Mid + Perp * T->Radius, 60);
                    const TCHAR* Veto = Dir.IsNearlyZero() ? VetoRedundant
                        : S.Wires >= 2 || S.bWirePending ? VetoRedundant
                        : F->Distance2D < 200 ? VetoRedundant          // already past where the wire would go
                        : F->Distance2D > T->Range ? VetoOutOfRange
                        : !S.bEReady ? VetoCooldown : nullptr;
                    Ability(EDMAIAction::Tripwire, F, A, Veto, T->Magnitude * Worth(*F) * (Approaching(*F) ? 1.f : .4f), -1, 0, B);
                }
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::DeadGround))
                {
                    const TCHAR* Veto = !HasArmedTrap() ? VetoRedundant : !S.bRReady ? VetoCooldown : nullptr;
                    Ability(EDMAIAction::DeadGround, nullptr, S.Location, Veto, T->Magnitude * WorthNearArmedTraps(T->Radius), -1, 0);
                }
            }
            // The Photographer's kit is emitted outside the base-Q gate on purpose: Flashbulb and Develop are both
            // castable while framing, and the frame is exactly when a bot has Exposure worth spending.
            if (bCompanion && !S.bProfileRange && S.Kind == EDMInvestigator::Photographer)
            {
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::Flashbulb); T && F)
                {
                    // Worth what it catches: a subject mid-telegraph gives up more Exposure to the flash.
                    float Value = 0;
                    const FVector Direction = (F->Location - S.Location).GetSafeNormal2D();
                    for (const FDMAIActorView& A : C.Actors)
                    {
                        if (!Hostile(A) || !DMKitRules::PointInCone(S.Location, Direction, T->Radius, T->Range, A.Location)) { continue; }
                        Value += T->Magnitude * Worth(A) + (A.bCommitted ? T->SecondaryMagnitude : 0.f);
                    }
                    // The cone is aimed at the focus, so the focus is always inside it; range is what decides
                    // whether anything is caught at all, and the value speaks for how much.
                    const TCHAR* Veto = F->Distance2D > T->Range ? VetoOutOfRange : !S.bWReady ? VetoCooldown : nullptr;
                    Ability(EDMAIAction::Flashbulb, F, F->Location, Veto, Value, -1, 0);
                }
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::Develop); T && F)
                {
                    // Exposure is an investment in a subject: spend it once it is worth more than another frame.
                    const float Ready = S.bRActive ? T->SecondaryMagnitude : T->Radius;
                    const TCHAR* Veto = F->Exposure < Ready ? VetoRedundant
                        : F->Distance2D > T->Range ? VetoOutOfRange
                        : DyingCheck(F, T) <= 0 ? VetoDying
                        : !S.bEReady ? VetoCooldown : nullptr;
                    Ability(EDMAIAction::Develop, F, F->Location, Veto, T->Magnitude * F->Exposure * Worth(*F), -1, 0);
                }
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::ImpossiblePhotograph))
                {
                    float Value = 0;
                    for (const FDMAIActorView& A : C.Actors)
                    { if (Hostile(A) && A.bVisible && A.Distance2D <= T->Range) { Value += T->Magnitude * Worth(A); } }
                    const TCHAR* Veto = Value <= 0 ? VetoRedundant : !S.bRReady ? VetoCooldown : nullptr;
                    Ability(EDMAIAction::ImpossiblePhotograph, nullptr, S.Location, Veto, Value, -1, 0);
                }
            }
            if (bCompanion && !S.bProfileRange && S.Kind == EDMInvestigator::Medium)
            {
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::Beckon))
                {
                    // Call the spirits to whoever needs them: a threatened ally first, otherwise the focus.
                    const FDMAIActorView* Threatened = nullptr;
                    for (const FDMAIActorView& A : C.Actors)
                    { if (Friendly(A) && A.Health < W.ThreatenedAllyHealth && (!Threatened || A.Health < Threatened->Health)) { Threatened = &A; } }
                    const FDMAIActorView* Destination = Threatened ? Threatened : F;
                    if (Destination)
                    {
                        int32 Spirits = 0, Away = 0;
                        for (const FDMAIMarkerView& Mk : C.Markers)
                        {
                            if (Mk.Kind != FDMAIMarkerView::Spirit) { continue; }
                            ++Spirits;
                            if (FVector::Dist2D(Mk.Location, Destination->Location) > T->Radius) { ++Away; }
                        }
                        // Units the arrival would actually reach, friend or foe: the pulse does both.
                        int32 Reached = 0;
                        for (const FDMAIActorView& A : C.Actors)
                        { if (!A.bDown && A.Index != S.Index && FVector::Dist2D(A.Location, Destination->Location) <= T->Radius) { ++Reached; } }
                        const TCHAR* Veto = Spirits == 0 ? VetoRedundant
                            : Away == 0 ? VetoRedundant           // already gathered where they are wanted
                            : Destination->Distance2D > T->Range ? VetoOutOfRange
                            : !S.bWReady ? VetoCooldown : nullptr;
                        Ability(EDMAIAction::Beckon, nullptr, Destination->Location, Veto, T->Magnitude * Spirits * Reached, -1, 0);
                    }
                }
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::Intercession))
                {
                    // Intercession spends the spirit, so it is worth what that spirit can do where it stands.
                    float Value = 0;
                    for (const FDMAIMarkerView& Mk : C.Markers)
                    {
                        if (Mk.Kind != FDMAIMarkerView::Spirit || Mk.bTravelling || Mk.Attention < S.MaxAttention) { continue; }
                        const FDMAIActorView* Bound = Actor(Mk.BoundIndex);
                        const float Scale = Mk.Attention / 100.f + .5f;
                        if (Bound && Hostile(*Bound)) { Value = (T->Magnitude + Mk.Attention * .3f) * Worth(*Bound) * Scale; }
                        else if (Bound && Friendly(*Bound)) { Value = (Bound->Health < W.ThreatenedAllyHealth ? T->SecondaryMagnitude : T->SecondaryMagnitude * .3f) * Scale; }
                        else
                        {
                            int32 Reached = 0;
                            for (const FDMAIActorView& A : C.Actors)
                            { if (!A.bDown && A.Index != S.Index && FVector::Dist2D(A.Location, Mk.Location) <= T->Radius) { ++Reached; } }
                            Value = 10.f * Reached * Scale;
                        }
                        break;
                    }
                    const TCHAR* Veto = S.MaxAttention < 20 ? VetoRedundant : !S.bEReady ? VetoCooldown : nullptr;
                    Ability(EDMAIAction::Intercession, nullptr, S.Location, Veto, Value, -1, 0);
                }
                if (const FDMAIAbilityTemplate* T = W.FindAbility(EDMAIAction::OpenSeance))
                {
                    int32 Spirits = 0;
                    float Value = 0;
                    for (const FDMAIMarkerView& Mk : C.Markers)
                    {
                        if (Mk.Kind != FDMAIMarkerView::Spirit) { continue; }
                        ++Spirits;
                        for (const FDMAIActorView& A : C.Actors)
                        { if (Hostile(A) && FVector::Dist2D(A.Location, Mk.Location) <= T->Radius) { Value += T->Magnitude * Worth(A); } }
                    }
                    const TCHAR* Veto = Spirits == 0 ? VetoRedundant : !S.bRReady ? VetoCooldown : nullptr;
                    Ability(EDMAIAction::OpenSeance, nullptr, S.Location, Veto, Value, -1, 0);
                }
            }

            // Pickups (Sapper).
            if (bCompanion && S.Kind == EDMInvestigator::Sapper && S.Charges < S.ChargeCapacity)
            {
                const FVector* Nearest = nullptr; float BestD = W.PickupRadius;
                for (const FVector& P : C.Pickups) { const float D = FVector::Dist2D(S.Location, P); if (D <= BestD && (!Nearest || D < BestD)) { Nearest = &P; BestD = D; } }
                if (Nearest && (S.Charges == 0 || Focus == INDEX_NONE || (S.Charges <= 1 && BestD <= SeekPickupNearRadius)))
                {
                    FEmit E; E.Action = EDMAIAction::SeekPickup; E.Point = *Nearest; E.Veto = S.bProfileRange ? VetoProfile : nullptr;
                    if (FDMAIOption* O = Emit(W.FindAction(EDMAIAction::SeekPickup), E)) { if (S.Charges > 0) { O->Rank = EDMAIRank::Routine; } }
                }
                if (!S.bProfileRange)
                {
                    for (const FDMAIPingView& P : C.Pings)
                    {
                        if (P.Kind != EDMPingKind::Pickup) { continue; }
                        const FVector* Near = nullptr; float ND = MAX_flt;
                        for (const FVector& Pk : C.Pickups) { const float D = FVector::Dist2D(P.Location, Pk); if (D < ND) { Near = &Pk; ND = D; } }
                        Simple(EDMAIAction::SeekPingedPickup, nullptr, Near ? *Near : P.Location, false, &P);
                    }
                }
            }

            // Command pings (companions): walk to the point, then hold there (Point = self, executed as StopGoal) so
            // FollowLeader or Engage cannot drag the bot back across the arrive ring every other tick.
            if (bCompanion)
            {
                for (const FDMAIPingView& P : C.Pings)
                {
                    switch (P.Kind)
                    {
                    case EDMPingKind::GoHere:
                        Simple(EDMAIAction::RallyToPing, nullptr, FVector::Dist2D(S.Location, P.Location) > W.RallyArriveRadius ? P.Location : S.Location, false, &P);
                        break;
                    case EDMPingKind::Defend:
                        Simple(EDMAIAction::DefendPing, nullptr, FVector::Dist2D(S.Location, P.Location) > W.DefendHoldRadius ? P.Location : S.Location, false, &P);
                        break;
                    case EDMPingKind::Help:
                        if (const FDMAIActorView* Ally = Actor(P.TargetIndex))
                        {
                            if (!Friendly(*Ally)) { break; }
                            if (Ally->Distance2D > W.HelpArriveRadius) { Simple(EDMAIAction::HelpPing, Ally, Ally->Location, false, &P); }
                            else { Simple(EDMAIAction::HelpPing, nullptr, S.Location, false, &P); }
                        }
                        break;
                    default: break;
                    }
                }
            }

            // Engage: approach the focus to attack range, or to the range of a cast vetoed out_of_range.
            if (F)
            {
                float Stop = S.AttackRange - W.ApproachBand; bool bBonus = false;
                for (const FDMAIOption& O : Out)
                {
                    if (!DrivesApproach(O.Action) || O.Target != Focus || O.Veto != VetoOutOfRange) { continue; }
                    // Only close in for a cast that would actually be worth making once in range.
                    if (O.Threshold > 0 && O.Value * O.PHit <= O.Threshold) { continue; }
                    const FDMAIAbilityTemplate* T = W.FindAbility(O.Action);
                    if (T && T->Range - 20 < Stop) { Stop = T->Range - 20; bBonus = true; }
                }
                if (!F->bVisible) { Stop = FMath::Min(Stop, OccludedApproachStop); }
                if (F->Distance2D > Stop)
                {
                    FEmit E; E.Action = EDMAIAction::Engage; E.Target = F; E.Point = F->Location; E.bPingAct = true; E.Multiplier = bBonus ? 1.2f : 1.f;
                    Emit(W.FindAction(EDMAIAction::Engage), E);
                }
            }
            if (bCompanion && !F)
            {
                for (const FDMAIPingView& P : C.Pings)
                {
                    if (P.Kind != EDMPingKind::Enemy && P.Kind != EDMPingKind::Perceive) { continue; }
                    Simple(EDMAIAction::InvestigatePing, nullptr, FVector::Dist2D(S.Location, P.Location) > W.RallyArriveRadius ? P.Location : S.Location, false, &P);
                }
            }
            if (S.bLocalEnemy && !F)
            {
                if (FVector::Dist2D(S.Location, S.Anchor) > W.AnchorRadius) { Simple(EDMAIAction::Anchor, nullptr, S.Anchor); }
                else if (S.bPatrolMember) { Simple(EDMAIAction::Patrol, nullptr, S.Anchor); }
                else { Simple(EDMAIAction::Hold, nullptr, S.Location); }
            }
            if (bCompanion && S.bCompanionTethered && !F)
            {
                const FDMAIActorView* Leader = Actor(C.LeaderIndex);
                if (Leader && !Leader->bDown && Leader->Distance2D > W.FollowDistance) { Simple(EDMAIAction::FollowLeader, Leader, Leader->Location); }
            }
            if (F && S.FrameTarget == INDEX_NONE)
            {
                FEmit E; E.Action = EDMAIAction::BasicAttack; E.Target = F; E.Point = F->Location; E.bPingAct = true;
                Emit(W.FindAction(EDMAIAction::BasicAttack), E);
            }
        }
    };

    void BuildOptionsWith(const FDMAIContext& C, int32 Focus, const FDMAIMemory& M, TArray<FDMAIOption>& Out)
    {
        static const FDMAIWeights Fallback = DMUtilityAI::DefaultWeights(EDMSmuggler::None, EDMInvestigator::None);
        FBuilder B{ C, C.W ? *C.W : Fallback, C.Self, M, Focus, Out };
        B.Run();
    }

    // ---------------------------------------------------------------------------------- defaults

    struct FDefaults
    {
        FDMAIWeights& W;
        static FDMAICurveSpec Switch() { return FDMAICurveSpec(EDMAICurve::Linear, 0, 1); }
        static TArray<FDMAIConsideration> Std()
        { return { FDMAIConsideration(EDMAIInput::NotCasting, Switch()), FDMAIConsideration(EDMAIInput::NotRooted, Switch()) }; }
        FDMAIActionSpec& Add(EDMAIAction A, EDMAIRank R, float Weight, EDMAIChannel Ch, TArray<FDMAIConsideration> Cons, int32 Runtime = 0, int32 Cooldown = 0)
        {
            FDMAIActionSpec& S = W.Actions.AddDefaulted_GetRef();
            S.Action = A; S.Rank = R; S.Weight = Weight; S.ChannelMask = static_cast<uint8>(Ch); S.Considerations = MoveTemp(Cons);
            S.MaxRuntimeTicks = Runtime; S.DecisionCooldownTicks = Cooldown; S.bEnabled = Weight > 0;
            return S;
        }
        void Template(EDMAIAction A, float Range, float Radius, float Magnitude, float Secondary, float Base, int32 Cooldown, int32 Delay, bool bRoots, int32 ThresholdCap = 0)
        {
            FDMAIAbilityTemplate T; T.Range = Range; T.Radius = Radius; T.Magnitude = Magnitude; T.SecondaryMagnitude = Secondary; T.Base = Base;
            T.CooldownTicks = Cooldown; T.CastDelayTicks = Delay; T.bRoots = bRoots; T.ThresholdCooldownCap = ThresholdCap;
            W.Abilities.Add(A, T);
        }
    };

    float RoleAttackRange(EDMSmuggler Role)
    {
        // Mirrors UDMSmugglerComponent::Range so the KeepDistance curve reaches zero where the latch exits.
        switch (Role)
        {
        case EDMSmuggler::Bruiser: return 155;
        case EDMSmuggler::Lookout: return 600;
        case EDMSmuggler::Bomber: return 550;
        default: return 700;
        }
    }
}

// --------------------------------------------------------------------------------------- tuned defaults

namespace
{
    /**
     * Values baked from the Saved/AITuning/final campaign (campaign3 investigators + campaign4 enemies, 11 Sep 2026;
     * thirty-seed hold-out 4/30 wins vs 2/30 for the hand defaults, see final/report.md). Applied on top of the hand
     * defaults so DefaultWeights(Role, Kind) equals those defaults plus final/best.json exactly: the literals are doubles
     * narrowed once, the same path a -DMAIWeights JSON number takes. Curve bookends that DefaultWeights derives from a
     * scalar (Flee, KeepDistance, SeekPickup, FollowLeader) are re-derived afterwards, as UDMAIProfile does for a JSON
     * override, so a tuned FleeExit also moves the Flee curve. Sandbox tuning only: nothing here is approved balance.
     */
    struct FTuned
    {
        FDMAIWeights& W;
        void Weight(EDMAIAction A, double V) { if (FDMAIActionSpec* S = W.FindAction(A)) { S->Weight = static_cast<float>(V); S->bEnabled = S->Weight > 0; } }
        void Runtime(EDMAIAction A, int32 Ticks) { if (FDMAIActionSpec* S = W.FindAction(A)) { S->MaxRuntimeTicks = Ticks; } }
        void Base(EDMAIAction A, double V) { if (FDMAIAbilityTemplate* T = W.Abilities.Find(A)) { T->Base = static_cast<float>(V); } }
        void Bookend(EDMAIAction A, EDMAIInput In, float Min, float Max)
        {
            FDMAIActionSpec* S = W.FindAction(A);
            if (!S) { return; }
            for (FDMAIConsideration& Con : S->Considerations) { if (Con.Input == In) { Con.Curve.Min = Min; Con.Curve.Max = Max; } }
        }
    };

    void ApplyTunedDefaults(FDMAIWeights& W, EDMSmuggler Role, EDMInvestigator Kind)
    {
        FTuned T{ W };
        const auto F = [](double V) { return static_cast<float>(V); };
        switch (Role)
        {
        case EDMSmuggler::Gunman: W.SightRange = F(612.3128); T.Runtime(EDMAIAction::KeepDistance, 27); break;
        case EDMSmuggler::Bruiser: W.LeashRange = F(1492.3444); break;
        case EDMSmuggler::Lookout: W.AllyRadius = F(1011.3852); W.EliteWorth = F(0.9737); W.TargetCommitment = F(13.7513); break;
        case EDMSmuggler::Bomber: W.TargetCommitment = F(52.5924); break;
        default: break;
        }
        switch (Kind)
        {
        case EDMInvestigator::Sapper:
            W.EliteWorth = F(0.5328); W.FleeDistance = F(185.6959); W.FleeEnter = F(0.1843); W.KCooldown = F(0.7768); W.KStock = F(2.7864);
            W.MarkedWorth = F(0.36); W.PingEnemyScore = F(291.5309); W.TargetCommitment = F(46.0947);
            T.Weight(EDMAIAction::EvadeHazard, 1.0801); T.Weight(EDMAIAction::SeekPickup, 0.9233);
            break;
        case EDMInvestigator::Photographer:
            W.EliteWorth = F(0.5724); W.EvadeMargin = F(63.5246); W.FleeEnemyRadius = F(478.3227); W.FleeEnter = F(0.2556); W.FleeExit = F(0.3522);
            W.KCooldown = F(1.6185); W.TargetCommitment = F(4.8771);
            T.Weight(EDMAIAction::EvadeHazard, 1.6028); T.Weight(EDMAIAction::Flee, 1.5853); T.Base(EDMAIAction::Frame, 28.6499);
            break;
        case EDMInvestigator::Medium:
            W.EvadeMargin = F(131.5508); W.FleeEnter = F(0.1375); W.KCooldown = F(1.1811); W.MarkedWorth = F(0.2811); W.PingCompliance = F(0.8623);
            W.ThreatenedAllyHealth = F(51.7847);
            T.Weight(EDMAIAction::EvadeHazard, 0.6048); T.Weight(EDMAIAction::Flee, 0.8656); T.Base(EDMAIAction::BindSpirit, 14.5827);
            break;
        case EDMInvestigator::Smuggler:
            W.EliteWorth = F(0.6516); W.FleeExit = F(0.3853); W.KCooldown = F(0.8498); W.MarkedWorth = F(0.4809); W.PingCompliance = F(1);
            W.PingEnemyScore = F(347.1017);
            T.Weight(EDMAIAction::Flee, 0.835); T.Base(EDMAIAction::Clinch, 17.307);
            break;
        default: break;
        }
        // Derived bookends, same formulas as the D.Add calls in DefaultWeights (a no-op for a scalar that was not tuned).
        T.Bookend(EDMAIAction::Flee, EDMAIInput::SelfHealthFrac, 0, W.FleeExit * 1.5f);
        T.Bookend(EDMAIAction::KeepDistance, EDMAIInput::Distance, 0, RoleAttackRange(Role) * W.KeepDistanceExit);
        T.Bookend(EDMAIAction::SeekPickup, EDMAIInput::Distance, 0, W.PickupRadius);
        T.Bookend(EDMAIAction::FollowLeader, EDMAIInput::LeaderDistance, W.FollowDistance, W.FollowDistance + 600);
    }
}

// --------------------------------------------------------------------------------------------- API

namespace DMUtilityAI
{
    float Combine(TArrayView<const float> Scores)
    {
        const int32 N = Scores.Num();
        if (N == 0) { return 1.f; }
        const float Mod = 1.f - 1.f / N;
        float Product = 1.f;
        for (float S : Scores)
        {
            if (S <= 0) { return 0.f; }
            Product *= S + (1.f - S) * Mod * S;
        }
        return FMath::Clamp(Product, 0.f, 1.f);
    }

    FDMAIWeights DefaultWeights(EDMSmuggler Role, EDMInvestigator Kind)
    {
        FDMAIWeights W;
        // Delayed area casts: a walking investigator (420 u/s) still gives a bomber a usable hit chance (x = 0.7 -> 0.51).
        W.PHitCurve = FDMAICurveSpec(EDMAICurve::InverseQuadratic, 0, 4, 2);
        // Wide enough that the step-out point is clearly outside a firebomb; the bot parks just outside the margin and keeps shooting.
        W.EvadeMargin = 100;
        FDefaults D{ W };
        const auto Inv = [](EDMAIInput In, float Min, float Max) { return FDMAIConsideration(In, FDMAICurveSpec(EDMAICurve::Linear, Min, Max, 2, .5f, true)); };
        const auto Lin = [](EDMAIInput In, float Min, float Max) { return FDMAIConsideration(In, FDMAICurveSpec(EDMAICurve::Linear, Min, Max)); };
        const auto InvQ = [](EDMAIInput In, float Min, float Max) { return FDMAIConsideration(In, FDMAICurveSpec(EDMAICurve::InverseQuadratic, Min, Max, 2)); };
        auto StdPlus = [](std::initializer_list<FDMAIConsideration> Extra) { TArray<FDMAIConsideration> C = FDefaults::Std(); for (const auto& E : Extra) { C.Add(E); } return C; };
        const EDMAIChannel ChMove = EDMAIChannel::Move, ChAttack = EDMAIChannel::Attack, ChCast = EDMAIChannel::Cast, ChAll = EDMAIChannel::All;

        D.Add(EDMAIAction::HoldCast, EDMAIRank::Locked, 1, ChAll, {});
        const bool bEnemyRole = Role != EDMSmuggler::None;
        const bool bCompanionKind = Kind != EDMInvestigator::None;
        const bool bRaider = !bEnemyRole && !bCompanionKind;

        // Rescue yields inside a hostile circle (switch on HazardDepth) so EvadeHazard wins there instead of a revive that restarts on every burn tick.
        if (bCompanionKind || bRaider) { D.Add(EDMAIAction::Rescue, EDMAIRank::Locked, 1, ChAll, { Inv(EDMAIInput::HazardDepth, 0, 1) }); }
        if (bEnemyRole) { D.Add(EDMAIAction::ReturnHome, EDMAIRank::Reflex, 1, ChAll, {}); }
        if (bCompanionKind)
        {
            // No runtime or decision cooldown on the hazard reflex: a cooldown would leave the bot standing in fire.
            D.Add(EDMAIAction::EvadeHazard, EDMAIRank::Reflex, 1, ChMove, StdPlus({ InvQ(EDMAIInput::SelfHealthFrac, 0, 1.25f) }));
            // Bookend 1.5 x FleeExit (not FleeEnter): the curve must stay above zero across the whole latch window, or the latch goes inert.
            D.Add(EDMAIAction::Flee, EDMAIRank::Reflex, 1, ChMove, StdPlus({ InvQ(EDMAIInput::SelfHealthFrac, 0, W.FleeExit * 1.5f) }));
            D.Add(EDMAIAction::RetreatToPing, EDMAIRank::Reflex, 1, ChMove, StdPlus({ Inv(EDMAIInput::PingAge, 0, 1) }));
        }
        else { D.Add(EDMAIAction::Flee, EDMAIRank::Reflex, 0, ChMove, FDefaults::Std()); }
        if (Kind == EDMInvestigator::Smuggler) { D.Add(EDMAIAction::Throw, EDMAIRank::Reflex, 1, ChCast, { FDMAIConsideration(EDMAIInput::NotCasting, FDefaults::Switch()) }); }
        if (Kind == EDMInvestigator::Photographer) { D.Add(EDMAIAction::HoldFrame, EDMAIRank::Reflex, 1, ChMove | ChAttack, {}); }
        if (Role == EDMSmuggler::Gunman || Role == EDMSmuggler::Bomber || Role == EDMSmuggler::Lookout)
        { D.Add(EDMAIAction::KeepDistance, EDMAIRank::Tactical, 1, ChMove, StdPlus({ InvQ(EDMAIInput::Distance, 0, RoleAttackRange(Role) * W.KeepDistanceExit) }), 40, 30); }
        if (Role == EDMSmuggler::GangBoss) { D.Add(EDMAIAction::Strafe, EDMAIRank::Tactical, .7f, ChMove, FDefaults::Std()); }
        if (Kind == EDMInvestigator::Sapper)
        {
            D.Add(EDMAIAction::SeekPickup, EDMAIRank::Tactical, .9f, ChMove, StdPlus({ InvQ(EDMAIInput::StockFrac, 0, 1), Inv(EDMAIInput::Distance, 0, W.PickupRadius) }));
            D.Add(EDMAIAction::SeekPingedPickup, EDMAIRank::Tactical, 1, ChMove, StdPlus({ Inv(EDMAIInput::PingAge, 0, 1) }));
        }
        if (bCompanionKind)
        {
            D.Add(EDMAIAction::RallyToPing, EDMAIRank::Tactical, 1, ChMove, StdPlus({ Inv(EDMAIInput::PingAge, 0, 1), Inv(EDMAIInput::PingDistance, 0, 4000) }));
            D.Add(EDMAIAction::DefendPing, EDMAIRank::Tactical, 1, ChMove, StdPlus({ Inv(EDMAIInput::PingAge, 0, 1) }));
            D.Add(EDMAIAction::HelpPing, EDMAIRank::Tactical, 1, ChMove, StdPlus({ Inv(EDMAIInput::PingAge, 0, 1) }));
        }
        D.Add(EDMAIAction::Engage, EDMAIRank::Routine, 1, ChMove, FDefaults::Std());
        if (bCompanionKind) { D.Add(EDMAIAction::InvestigatePing, EDMAIRank::Routine, .8f, ChMove, StdPlus({ Inv(EDMAIInput::PingAge, 0, 1) })); }
        if (bEnemyRole || bRaider)
        {
            D.Add(EDMAIAction::Anchor, EDMAIRank::Routine, .3f, ChMove, FDefaults::Std());
            if (bEnemyRole) { D.Add(EDMAIAction::Patrol, EDMAIRank::Routine, .3f, ChMove, FDefaults::Std()); }
        }
        if (bCompanionKind || bRaider)
        { D.Add(EDMAIAction::FollowLeader, EDMAIRank::Routine, .4f, ChMove, StdPlus({ Lin(EDMAIInput::LeaderDistance, W.FollowDistance, W.FollowDistance + 600) })); }
        if (bEnemyRole || bRaider) { D.Add(EDMAIAction::Hold, EDMAIRank::Routine, .1f, ChMove, {}); }
        // HasSight last: an occluded focus holds the attack channel (no telegraph, no wasted activation) while Engage closes in.
        D.Add(EDMAIAction::BasicAttack, EDMAIRank::Routine, .5f, ChAttack, { FDMAIConsideration(EDMAIInput::NotCasting, FDefaults::Switch()), FDMAIConsideration(EDMAIInput::NotFraming, FDefaults::Switch()), FDMAIConsideration(EDMAIInput::HasSight, FDefaults::Switch()) });

        // Abilities: one decision template per signature or base Q. Numbers are provisional sandbox tuning.
        switch (Role)
        {
        // Bruiser: Magnitude 14 clears the 8.25 threshold on a plain target (score 0.7); the diver bonus still dominates.
        case EDMSmuggler::Bruiser: D.Template(EDMAIAction::Signature, 200, 0, 14, 20, 5, 65, 6, true); break;
        // Bomber: Base 5 (threshold 9.5) so a walking target at PHit 0.51 is still worth a firebomb.
        case EDMSmuggler::Bomber: D.Template(EDMAIAction::Signature, 650, 180, 26, 0, 5, 90, 12, true); break;
        case EDMSmuggler::Lookout: D.Template(EDMAIAction::Signature, 900, 0, 5, 5, 5, 85, 0, false); break;
        case EDMSmuggler::GangBoss: D.Template(EDMAIAction::Signature, 900, 0, 5, 6, 5, 95, 0, false); break;
        default: break;
        }
        if (W.Abilities.Contains(EDMAIAction::Signature)) { D.Add(EDMAIAction::Signature, EDMAIRank::Tactical, 1, ChCast, FDefaults::Std()); }
        const auto QCons = [&] { return StdPlus({ FDMAIConsideration(EDMAIInput::NotFraming, FDefaults::Switch()), FDMAIConsideration(EDMAIInput::HasSight, FDefaults::Switch()) }); };
        switch (Kind)
        {
        case EDMInvestigator::Sapper:
            // Range matches UDMPrimaryComponent::Range (650); Base 23 gives thresholds 24.8 / 41.4 / 58 so two charges
            // spend on a single mover and the last one is held for a pair, an elite or a marked target.
            D.Template(EDMAIAction::PlaceSatchel, 650, 220, 55, 0, 23, 8, 5, false);
            D.Add(EDMAIAction::PlaceSatchel, EDMAIRank::Tactical, 1, ChCast, QCons());
            // Base 10 puts the threshold at 19.3, so one common near the focus (24) is already worth suppressing.
            // Routine, not Tactical: only one cast is chosen per tick, and suppression exists to hold enemies in
            // prepared ground rather than to replace preparing it. Ranking it below the traps says that outright,
            // where balancing weights against the satchel's conservation curve would only hold for one charge count.
            D.Template(EDMAIAction::SuppressingFire, 550, 250, 24, 0, 10, 120, 0, false);
            D.Add(EDMAIAction::SuppressingFire, EDMAIRank::Routine, 1, ChCast, QCons());
            // Base 16 -> threshold 25.9: an approaching common (40) clears it, one walking away (16) never does.
            D.Template(EDMAIAction::Tripwire, 650, 150, 40, 0, 16, 80, 0, false);
            D.Add(EDMAIAction::Tripwire, EDMAIRank::Tactical, 1, ChCast, QCons());
            // The threshold cap keeps a 600-tick ultimate reachable: uncapped it would need a value no fight produces.
            // Base 32 with the cap -> threshold 69.3, so two commons or one elite near a trap is worth deferring for.
            D.Template(EDMAIAction::DeadGround, 0, 300, 55, 0, 32, 600, 0, false, 150);
            D.Add(EDMAIAction::DeadGround, EDMAIRank::Tactical, 1, ChCast, QCons());
            break;
        case EDMInvestigator::Medium:
            D.Template(EDMAIAction::BindSpirit, 650, 0, 40, 20, 10, 25, 0, false);
            D.Add(EDMAIAction::BindSpirit, EDMAIRank::Tactical, 1, ChCast, QCons());
            // Radius is the arrival pulse. Routine: repositioning spirits should not pre-empt binding a new one or
            // spending one that is ready. Base 8 -> threshold 21.2, so one spirit reaching two units (24) is worth it.
            D.Template(EDMAIAction::Beckon, 650, 180, 12, 0, 8, 80, 0, false);
            D.Add(EDMAIAction::Beckon, EDMAIRank::Routine, 1, ChCast, QCons());
            // Magnitude/Secondary are the hostile and protective interventions; Radius is the ground pulse.
            // Base 10 -> threshold 21.8: a haunt at the minimum Attention is worth 18.2 and is kept, per K.4's
            // "do not expend Intercession simply because it is available". A threatened ally is worth it at once.
            D.Template(EDMAIAction::Intercession, 0, 180, 20, 45, 10, 100, 0, false);
            D.Add(EDMAIAction::Intercession, EDMAIRank::Tactical, 1, ChCast, QCons());
            // Base 27 with the cap -> threshold 141: two spirits with two hostiles each (100) still holds; the
            // ultimate wants the spirits already placed where the fight is.
            D.Template(EDMAIAction::OpenSeance, 0, 300, 25, 0, 27, 600, 0, false, 150);
            D.Add(EDMAIAction::OpenSeance, EDMAIRank::Tactical, 1, ChCast, QCons());
            break;
        case EDMInvestigator::Photographer:
            D.Template(EDMAIAction::Frame, 850, 0, 30, 0, 20, 20, 30, false);
            D.Add(EDMAIAction::Frame, EDMAIRank::Tactical, 1, ChAll, QCons());
            // Radius carries the flash half-angle and SecondaryMagnitude the bonus for catching a committed enemy.
            // Routine for the same reason as the Sapper's suppression: disruption should not pre-empt the frame or
            // the Develop it is feeding. Base 6 -> threshold 15.7, so one common in the cone (20) is worth a flash.
            D.Template(EDMAIAction::Flashbulb, 350, 35, 20, 15, 6, 100, 0, false);
            D.Add(EDMAIAction::Flashbulb, EDMAIRank::Routine, 1, ChCast, QCons());
            // Radius is the Exposure a bot waits for and SecondaryMagnitude the lower bar inside the ultimate.
            // Base 10 -> threshold 29.7: developing 40 Exposure (28) still waits, 50 (35) spends.
            D.Template(EDMAIAction::Develop, 850, 40, .7f, 20, 10, 60, 0, false);
            D.Add(EDMAIAction::Develop, EDMAIRank::Tactical, 1, ChCast, QCons());
            // Base 11.7 with the cap -> threshold 55.4: two visible commons (60) are worth the photograph, one is not.
            D.Template(EDMAIAction::ImpossiblePhotograph, 1200, 0, 30, 0, 11.7f, 600, 0, false, 150);
            D.Add(EDMAIAction::ImpossiblePhotograph, EDMAIRank::Tactical, 1, ChCast, QCons());
            break;
        case EDMInvestigator::Smuggler:
            D.Template(EDMAIAction::Clinch, 180, 0, 10, 0, 10, 40, 0, false);
            D.Add(EDMAIAction::Clinch, EDMAIRank::Tactical, 1, ChMove | ChCast, QCons());
            break;
        default: break;
        }
        ApplyTunedDefaults(W, Role, Kind);
        return W;
    }

    int32 ChooseFocus(const FDMAIContext& C, EDMAIRank* OutRank, float* OutScore)
    {
        static const FDMAIWeights Fallback = DefaultWeights(EDMSmuggler::None, EDMInvestigator::None);
        const FDMAIWeights& W = C.W ? *C.W : Fallback;
        const FDMAISelfView& S = C.Self;
        const FDMAIPingView* Defend = !S.bEnemy ? BestPing(C, EDMPingKind::Defend) : nullptr;
        int32 Best = INDEX_NONE; EDMAIRank BestRank = EDMAIRank::Routine; float BestScore = -MAX_flt;
        for (const FDMAIActorView& A : C.Actors)
        {
            if (A.bDown || A.bEnemy == S.bEnemy) { continue; }
            if (S.bLocalEnemy)
            {
                const bool bAlerted = A.bForced || A.bMarked || A.bDiver || A.Threat > 0 || A.Index == C.Memory.TargetIndex || A.bGroupEngaged;
                // The current target keeps 15% of slack on the candidate leash so a target pacing the 1800 line does not flicker focus.
                const float Leash = A.Index == C.Memory.TargetIndex ? W.LeashRange * 1.15f : W.LeashRange;
                if (A.AnchorDistance2D > Leash || (!bAlerted && (A.Distance > W.SightRange || !A.bVisible))) { continue; }
            }
            else if (!S.bEnemy && S.bCompanionTethered && A.LeaderDistance2D > W.TetherLeaderRadius && A.Distance > W.TetherSelfRadius) { continue; }
            if (Defend && FVector::Dist2D(Defend->Location, A.Location) > W.DefendEngageRadius && A.AttackTargetIndex != S.Index) { continue; }
            EDMAIRank Rank = EDMAIRank::Routine;
            if (A.bForced) { Rank = EDMAIRank::Locked; }
            else if (S.bEnemy) { Rank = A.bDiver ? EDMAIRank::Reflex : A.bMarked ? EDMAIRank::Tactical : EDMAIRank::Routine; }
            else { Rank = ExactPingWeight(C, A.Index, EDMPingKind::Focus) > 0 ? EDMAIRank::Reflex : AttacksHelpedAlly(C, A) ? EDMAIRank::Tactical : EDMAIRank::Routine; }
            const float Score = (S.bEnemy ? A.Threat * 1000.f : 0.f) - A.Distance + (A.Index == C.Memory.TargetIndex ? W.TargetCommitment : 0.f)
                + W.PingEnemyScore * PingWeightOnTarget(C, A.Index, EDMPingKind::Enemy) - W.PingIgnorePenalty * PingWeightOnTarget(C, A.Index, EDMPingKind::Ignore);
            if (Best == INDEX_NONE || Rank > BestRank || (Rank == BestRank && Score > BestScore)) { Best = A.Index; BestRank = Rank; BestScore = Score; }
        }
        if (OutRank) { *OutRank = Best == INDEX_NONE ? EDMAIRank::Routine : BestRank; }
        if (OutScore) { *OutScore = Best == INDEX_NONE ? 0.f : BestScore; }
        return Best;
    }

    void BuildOptions(const FDMAIContext& C, int32 Focus, TArray<FDMAIOption>& Out) { BuildOptionsWith(C, Focus, C.Memory, Out); }

    void Select(TArray<FDMAIOption>& Options, FDMAIDecision& Out)
    {
        Algo::StableSort(Options, [](const FDMAIOption& A, const FDMAIOption& B)
        {
            if (A.Rank != B.Rank) { return A.Rank > B.Rank; }
            if (A.Score != B.Score) { return A.Score > B.Score; }
            if (A.Action != B.Action) { return A.Action < B.Action; }
            if (A.Target != B.Target) { return A.Target < B.Target; }
            if (A.Point.X != B.Point.X) { return A.Point.X < B.Point.X; }
            return A.Point.Y < B.Point.Y;
        });
        Out.Ranked = MoveTemp(Options);
        Out.Chosen.Reset();
        Out.Taken = EDMAIChannel::None;
        for (const FDMAIOption& O : Out.Ranked)
        {
            if (O.Veto || O.Score <= 0 || EnumHasAnyFlags(Out.Taken, O.Channels)) { continue; }
            Out.Chosen.Add(O);
            Out.Taken |= O.Channels;
            if (Out.Taken == EDMAIChannel::All) { break; }
        }
    }

    FDMAIDecision Decide(const FDMAIContext& C)
    {
        static const FDMAIWeights Fallback = DefaultWeights(EDMSmuggler::None, EDMInvestigator::None);
        const FDMAIWeights& W = C.W ? *C.W : Fallback;
        const FDMAISelfView& S = C.Self;
        const int32 Tick = C.Tick;
        FDMAIDecision D;
        D.Memory = C.Memory;
        FDMAIMemory& M = D.Memory;
        auto RequestPing = [&](EDMPingKind Kind, int32 Target, FVector Location)
        {
            if (Tick - M.LastPingTick < W.BotPingCooldownTicks) { return; }
            FDMAIPingRequest& R = D.PingRequests.AddDefaulted_GetRef(); R.Kind = Kind; R.TargetIndex = Target; R.Location = Location;
            M.LastPingTick = Tick;
        };

        // Latches (enter/exit hysteresis).
        float NearestHostile = MAX_flt;
        for (const FDMAIActorView& A : C.Actors) { if (!A.bDown && A.bEnemy != S.bEnemy) { NearestHostile = FMath::Min(NearestHostile, A.Distance2D); } }
        if (S.bLocalEnemy)
        {
            const float AnchorDistance = FVector::Dist2D(S.Location, S.Anchor);
            if (AnchorDistance > W.LeashRange) { M.bReturningHome = true; }
            if (M.bReturningHome && AnchorDistance < W.LeashReturnRadius) { M.bReturningHome = false; }
        }
        else { M.bReturningHome = false; }
        const FDMAIActionSpec* FleeSpec = W.FindAction(EDMAIAction::Flee);
        if (!S.bEnemy && !S.bProfileRange && FleeSpec && FleeSpec->bEnabled && FleeSpec->Weight > 0)
        {
            const float HealthFrac = S.MaxHealth > 0 ? S.Health / S.MaxHealth : 0.f;
            if (!M.bFleeing && HealthFrac < W.FleeEnter && NearestHostile < W.FleeEnemyRadius)
            { M.bFleeing = true; RequestPing(EDMPingKind::Help, S.Index, S.Location); }
            else if (M.bFleeing && (HealthFrac > W.FleeExit || NearestHostile > W.FleeSafeRadius)) { M.bFleeing = false; }
        }
        else { M.bFleeing = false; }
        const FDMAIActionSpec* KeepSpec = W.FindAction(EDMAIAction::KeepDistance);
        if (S.bEnemy && S.bRanged && KeepSpec && KeepSpec->bEnabled)
        {
            if (!M.bKeepingDistance && NearestHostile < S.AttackRange * W.KeepDistanceEnter) { M.bKeepingDistance = true; }
            else if (M.bKeepingDistance && NearestHostile > S.AttackRange * W.KeepDistanceExit) { M.bKeepingDistance = false; }
        }
        else { M.bKeepingDistance = false; }

        // Focus, options, selection.
        if (S.bCasting || M.bReturningHome) { D.Focus = INDEX_NONE; }
        else { D.Focus = ChooseFocus(C, &D.FocusRank, &D.FocusScore); }
        TArray<FDMAIOption> Options;
        Options.Reserve(16);
        BuildOptionsWith(C, D.Focus, M, Options);
        Select(Options, D);

        for (const FDMAIOption& O : D.Chosen) { if (O.bClearsFocus) { D.bClearFocus = true; } }
        if (D.bClearFocus) { D.Focus = INDEX_NONE; D.FocusRank = EDMAIRank::Routine; D.FocusScore = 0; }
        D.bAttackHold = !D.Chose(EDMAIAction::BasicAttack);
        D.bAdvanceWaypoint = D.Chose(EDMAIAction::Patrol);
        // Parity: Threat.Empty() belongs to the leash and no-focus branches only; a casting enemy keeps its threat memory.
        D.bClearThreat = S.bLocalEnemy && !S.bCasting && D.Focus == INDEX_NONE;

        // Pings are a companion concern: enemies neither author nor answer them.
        if (!S.bEnemy)
        {
            // Bot-authored pings: call out a newly acquired hostile nobody has pinged.
            if (D.Focus != INDEX_NONE && D.Focus != C.Memory.TargetIndex)
            {
                bool bPinged = false;
                for (const FDMAIPingView& P : C.Pings) { if ((P.Kind == EDMPingKind::Enemy || P.Kind == EDMPingKind::Focus) && P.TargetIndex == D.Focus) { bPinged = true; break; } }
                if (!bPinged) { RequestPing(EDMPingKind::Enemy, D.Focus, C.Actors[D.Focus].Location); }
            }
            for (const FDMAIOption& O : D.Chosen) { if (O.PingId != INDEX_NONE) { D.PingsOnIt.AddUnique(O.PingId); } }
            // Compliance counts as on_it: attacking a pinged enemy, leaving an ignored one alone, or already standing inside a command ring.
            for (const FDMAIPingView& P : C.Pings)
            {
                if (P.bAuthoredBySelf) { continue; }
                bool bOnIt = false;
                switch (P.Kind)
                {
                case EDMPingKind::Enemy: case EDMPingKind::Focus: bOnIt = D.Focus != INDEX_NONE && P.TargetIndex == D.Focus; break;
                case EDMPingKind::Ignore: bOnIt = P.TargetIndex != INDEX_NONE && P.TargetIndex != D.Focus; break;
                case EDMPingKind::GoHere: case EDMPingKind::Retreat: bOnIt = FVector::Dist2D(S.Location, P.Location) <= W.RallyArriveRadius; break;
                case EDMPingKind::Defend: bOnIt = FVector::Dist2D(S.Location, P.Location) <= W.DefendHoldRadius; break;
                case EDMPingKind::Help: bOnIt = P.TargetIndex != S.Index && C.Actors.IsValidIndex(P.TargetIndex) && C.Actors[P.TargetIndex].Distance2D <= W.HelpArriveRadius; break;
                default: break;
                }
                if (bOnIt) { D.PingsOnIt.AddUnique(P.Id); }
            }
        }

        // Memory: target, last move/act, runtime counters and decision cooldowns.
        if (D.Focus != M.TargetIndex) { M.TargetIndex = D.Focus; M.TargetSinceTick = Tick; }
        const FDMAIOption* Move = D.ChosenOn(EDMAIChannel::Move);
        const EDMAIAction NewMove = Move ? Move->Action : EDMAIAction::None;
        if (NewMove != M.LastMove) { M.LastMove = NewMove; M.MoveSinceTick = Tick; }
        const FDMAIOption* Act = D.ChosenOn(EDMAIChannel::Cast | EDMAIChannel::Attack);
        M.LastAct = Act ? Act->Action : EDMAIAction::None;
        bool bChosen[static_cast<int32>(EDMAIAction::Count)] = {};
        for (const FDMAIOption& O : D.Chosen) { bChosen[static_cast<int32>(O.Action)] = true; }
        for (int32 I = 1; I < static_cast<int32>(EDMAIAction::Count); ++I)
        {
            const EDMAIAction A = static_cast<EDMAIAction>(I);
            if (bChosen[I]) { ++M.RuntimeTicks.FindOrAdd(A); }
            else if (const int32* Runtime = M.RuntimeTicks.Find(A))
            {
                const FDMAIActionSpec* Spec = *Runtime > 0 ? W.FindAction(A) : nullptr;
                if (Spec && Spec->DecisionCooldownTicks > 0) { M.CooldownUntil.Add(A, Tick + Spec->DecisionCooldownTicks); }
                M.RuntimeTicks.Remove(A);
            }
            if (const int32* Until = M.CooldownUntil.Find(A)) { if (*Until <= Tick) { M.CooldownUntil.Remove(A); } }
        }
        return D;
    }

    float Threshold(float Base, int32 Stock, int32 Capacity, int32 CooldownTicks, float KStock, float KCooldown)
    {
        const float StockFactor = Stock >= 0 && Capacity > 0 ? 1.f + KStock * (1.f - static_cast<float>(Stock) / Capacity) : 1.f;
        return Base * StockFactor * (1.f + KCooldown * CooldownTicks / 100.f);
    }

    float ConservedScore(float Weight, float Value, float Threshold)
    {
        if (Threshold <= 0) { return Value > 0 ? Weight : 0.f; }
        return Weight * FMath::Clamp((Value - Threshold) / Threshold, 0.f, 1.f);
    }

    float ExpectedHealthAtResolve(const FDMAIContext& C, int32 TargetIndex, int32 CastDelayTicks)
    {
        if (!C.Actors.IsValidIndex(TargetIndex)) { return 0.f; }
        const FDMAIActorView& T = C.Actors[TargetIndex];
        float Incoming = 0;
        for (const FDMAIActorView& A : C.Actors)
        {
            if (A.Index == C.Self.Index || A.bEnemy != C.Self.bEnemy || A.bDown || A.AttackTargetIndex != TargetIndex || A.NextAttackIn > CastDelayTicks) { continue; }
            const int32 Hits = 1 + (CastDelayTicks - A.NextAttackIn) / FMath::Max(1, A.AttackInterval);
            Incoming += Hits * A.AttackDamage;
        }
        return T.Health + T.Shield - Incoming;
    }

    float PingWeightOnTarget(const FDMAIContext& C, int32 TargetIndex, EDMPingKind Kind)
    {
        if (TargetIndex == INDEX_NONE) { return 0.f; }
        if (Kind == EDMPingKind::Focus || Kind == EDMPingKind::Enemy)
        { return FMath::Max(ExactPingWeight(C, TargetIndex, EDMPingKind::Focus), ExactPingWeight(C, TargetIndex, EDMPingKind::Enemy)); }
        return ExactPingWeight(C, TargetIndex, Kind);
    }

    const TCHAR* ActionName(EDMAIAction Action)
    { const int32 I = static_cast<int32>(Action); return I >= 0 && I < static_cast<int32>(UE_ARRAY_COUNT(ActionNames)) ? ActionNames[I] : TEXT("None"); }
    const TCHAR* RankName(EDMAIRank Rank)
    {
        switch (Rank)
        {
        case EDMAIRank::Locked: return TEXT("Locked");
        case EDMAIRank::Reflex: return TEXT("Reflex");
        case EDMAIRank::Tactical: return TEXT("Tactical");
        default: return TEXT("Routine");
        }
    }
    const TCHAR* InputName(EDMAIInput Input)
    { const int32 I = static_cast<int32>(Input); return I >= 0 && I < static_cast<int32>(UE_ARRAY_COUNT(InputNames)) ? InputNames[I] : TEXT("Distance"); }
    FString ChannelString(EDMAIChannel Channels)
    {
        FString Out;
        auto Append = [&](EDMAIChannel Bit, const TCHAR* Label) { if (EnumHasAnyFlags(Channels, Bit)) { if (!Out.IsEmpty()) { Out += TEXT("|"); } Out += Label; } };
        Append(EDMAIChannel::Move, TEXT("M")); Append(EDMAIChannel::Attack, TEXT("A")); Append(EDMAIChannel::Cast, TEXT("C"));
        return Out;
    }
    EDMAIAction ActionFromName(const FString& Name)
    {
        for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(ActionNames)); ++I) { if (Name.Equals(ActionNames[I], ESearchCase::IgnoreCase)) { return static_cast<EDMAIAction>(I); } }
        return EDMAIAction::None;
    }
}
