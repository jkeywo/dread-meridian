#include "DMKitRules.h"

bool FDMBreakMeter::Add(float Amount, int32 Tick)
{
    if (!FMath::IsFinite(Amount) || Amount <= 0 || IsBroken(Tick)) { return false; }
    Value += Amount * (IsResisting(Tick) ? ResistFactor : 1.f);
    if (Value < Threshold) { return false; }
    Value = Threshold; BrokenUntil = Tick + BrokenTicks;
    return true;
}

bool FDMBreakMeter::Step(int32 Tick)
{
    if (BrokenUntil <= 0 || Tick < BrokenUntil) { return false; }
    BrokenUntil = 0; Value = 0; ResistUntil = Tick + ResistTicks;
    return true;
}

bool FDMDeadGroundLedger::HasTag(uint8 Trap, int32 Serial) const
{ return Tags.ContainsByPredicate([&](const FTag& T) { return T.Trap == Trap && T.Serial == Serial; }); }

bool FDMDeadGroundLedger::Tag(int32 Enemy, uint8 Trap, int32 Serial, int32 Tick)
{
    if (Tags.ContainsByPredicate([&](const FTag& T) { return T.Enemy == Enemy && T.Trap == Trap && T.Serial == Serial; })) { return false; }
    if (Tags.IsEmpty()) { ResolveTick = Tick + DelayTicks; }
    Tags.Add({ Enemy, Trap, Serial });
    return true;
}

int32 FDMDeadGroundLedger::DropTrap(uint8 Trap, int32 Serial)
{
    const int32 Removed = Tags.RemoveAll([&](const FTag& T) { return T.Trap == Trap && T.Serial == Serial; });
    if (Tags.IsEmpty()) { ResolveTick = 0; }
    return Removed;
}

namespace DMKitRules
{
    float ShieldGain(float Current, float Cap, float Amount)
    {
        if (!FMath::IsFinite(Amount) || Amount <= 0 || !FMath::IsFinite(Current) || !FMath::IsFinite(Cap)) { return 0.f; }
        return FMath::Clamp(Cap - Current, 0.f, Amount);
    }

    void AddSlow(TArray<FDMSlow>& Slows, float Fraction, int32 UntilTick, int32 Tick, int32 MaxEntries)
    {
        Slows.RemoveAll([&](const FDMSlow& S) { return S.UntilTick <= Tick; });
        if (!FMath::IsFinite(Fraction) || Fraction <= 0 || UntilTick <= Tick) { return; }
        Fraction = FMath::Min(Fraction, 1.f);
        for (FDMSlow& S : Slows)
        { if (FMath::IsNearlyEqual(S.Fraction, Fraction)) { S.UntilTick = FMath::Max(S.UntilTick, UntilTick); return; } }
        Slows.Add({ Fraction, UntilTick });
        while (Slows.Num() > FMath::Max(1, MaxEntries))
        {
            int32 Weakest = 0;
            for (int32 I = 1; I < Slows.Num(); ++I)
            { if (Slows[I].Fraction < Slows[Weakest].Fraction || (Slows[I].Fraction == Slows[Weakest].Fraction && Slows[I].UntilTick < Slows[Weakest].UntilTick)) { Weakest = I; } }
            Slows.RemoveAt(Weakest);
        }
    }

    float EffectiveSlow(const TArray<FDMSlow>& Slows, int32 Tick)
    {
        float Best = 0;
        for (const FDMSlow& S : Slows) { if (S.UntilTick > Tick) { Best = FMath::Max(Best, S.Fraction); } }
        return Best;
    }

    bool PointInCone(const FVector& Origin, const FVector& Dir, float HalfAngleDeg, float Length, const FVector& Point)
    {
        if (Origin.ContainsNaN() || Dir.ContainsNaN() || Point.ContainsNaN() || !FMath::IsFinite(Length)) { return false; }
        const FVector To = (Point - Origin) * FVector(1, 1, 0);
        const float Dist = To.Size();
        if (Dist > Length) { return false; }
        if (Dist <= KINDA_SMALL_NUMBER) { return true; }
        const FVector D = Dir.GetSafeNormal2D();
        if (D.IsNearlyZero()) { return false; }
        return FVector::DotProduct(To / Dist, D) >= FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));
    }

    float DistanceToSegment2D(const FVector& A, const FVector& B, const FVector& P)
    {
        const FVector2D A2(A), B2(B), P2(P);
        const FVector2D AB = B2 - A2;
        const float L2 = AB.SizeSquared();
        const float T = L2 <= KINDA_SMALL_NUMBER ? 0.f : FMath::Clamp(FVector2D::DotProduct(P2 - A2, AB) / L2, 0.f, 1.f);
        return FVector2D::Distance(P2, A2 + AB * T);
    }

    static float Orient(const FVector2D& A, const FVector2D& B, const FVector2D& C)
    { return (B.X - A.X) * (C.Y - A.Y) - (B.Y - A.Y) * (C.X - A.X); }

    static bool OnSegment(const FVector2D& A, const FVector2D& B, const FVector2D& P)
    {
        return P.X <= FMath::Max(A.X, B.X) + KINDA_SMALL_NUMBER && P.X >= FMath::Min(A.X, B.X) - KINDA_SMALL_NUMBER
            && P.Y <= FMath::Max(A.Y, B.Y) + KINDA_SMALL_NUMBER && P.Y >= FMath::Min(A.Y, B.Y) - KINDA_SMALL_NUMBER;
    }

    bool SegmentsCross2D(const FVector& A, const FVector& B, const FVector& C, const FVector& D)
    {
        const FVector2D A2(A), B2(B), C2(C), D2(D);
        const float O1 = Orient(A2, B2, C2), O2 = Orient(A2, B2, D2), O3 = Orient(C2, D2, A2), O4 = Orient(C2, D2, B2);
        if (((O1 > 0) != (O2 > 0)) && ((O3 > 0) != (O4 > 0)) && O1 != 0 && O2 != 0 && O3 != 0 && O4 != 0) { return true; }
        if (FMath::IsNearlyZero(O1) && OnSegment(A2, B2, C2)) { return true; }
        if (FMath::IsNearlyZero(O2) && OnSegment(A2, B2, D2)) { return true; }
        if (FMath::IsNearlyZero(O3) && OnSegment(C2, D2, A2)) { return true; }
        if (FMath::IsNearlyZero(O4) && OnSegment(C2, D2, B2)) { return true; }
        return false;
    }

    bool CrossesWire(const FVector& A, const FVector& B, const FVector& P0, const FVector& P1, float Slack)
    {
        if (A.ContainsNaN() || B.ContainsNaN() || P0.ContainsNaN() || P1.ContainsNaN()) { return false; }
        if (DistanceToSegment2D(A, B, P1) <= Slack) { return true; }
        return !P0.Equals(P1) && SegmentsCross2D(A, B, P0, P1);
    }

    FDMControl ResolveControl(const FDMControl& Requested, bool bCommon, bool bBroken)
    {
        FDMControl R = Requested;
        if (bCommon) { R.BreakPressure = 0; return R; }
        if (bBroken) { R.BreakPressure = 0; return R; }
        R.Slow *= .5f; R.Displacement = FVector::ZeroVector; R.StaggerTicks = 0; R.bInterrupt = false;
        return R;
    }
}
