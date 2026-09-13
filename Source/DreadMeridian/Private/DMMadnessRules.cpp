#include "DMMadnessRules.h"
void FDMMadnessSettings::Sanitize()
{
    Early = FMath::IsFinite(Early) ? FMath::Clamp(Early, 1.f, 97.f) : 25;
    Mid = FMath::IsFinite(Mid) ? FMath::Clamp(Mid, Early + 1, 98.f) : FMath::Max(50.f, Early + 1);
    High = FMath::IsFinite(High) ? FMath::Clamp(High, Mid + 1, 99.f) : FMath::Max(75.f, Mid + 1);
    CrisisTicks = FMath::Clamp(CrisisTicks, 1, 10000);
    CrisisRestTicks = FMath::Clamp(CrisisRestTicks, 1, 10000);
    GroundTicks = FMath::Clamp(GroundTicks, 1, 10000);
    CrisisRecovery = FMath::IsFinite(CrisisRecovery) ? FMath::Clamp(CrisisRecovery, 1.f, 100.f) : 50;
    GroundRecovery = FMath::IsFinite(GroundRecovery) ? FMath::Clamp(GroundRecovery, 1.f, 100.f) : 10;
}
int32 FDMMadnessState::Band(const FDMMadnessSettings& S) const
{ return Current >= S.High ? 3 : Current >= S.Mid ? 2 : Current >= S.Early ? 1 : 0; }
void FDMMadnessState::Enter(int32 Tick, const FDMMadnessSettings& S)
{ if (!CrisisUntil && Tick >= RestUntil && Current >= 100) { CrisisUntil = Tick + S.CrisisTicks; } }
bool FDMMadnessState::Add(float Amount, int32 Tick, const FDMMadnessSettings& S)
{
    if (!FMath::IsFinite(Amount) || Amount <= 0) { return false; }
    const float Before = Current; const int32 BeforeCrisis = CrisisUntil;
    Current = FMath::Clamp(Current + Amount, Floor, 100.f); Enter(Tick, S);
    return Before != Current || BeforeCrisis != CrisisUntil;
}
bool FDMMadnessState::Recover(float Amount)
{
    if (!FMath::IsFinite(Amount) || Amount <= 0) { return false; }
    const float Before = Current; Current = FMath::Max(Floor, Current - Amount); return Before != Current;
}
bool FDMMadnessState::RaiseFloor(float Value, int32 Tick, const FDMMadnessSettings& S)
{
    if (!FMath::IsFinite(Value) || Value <= Floor) { return false; }
    const float Before = Floor; Floor = FMath::Clamp(Value, Floor, 100.f);
    Current = FMath::Max(Current, Floor); Enter(Tick, S); return Before != Floor;
}
void FDMMadnessState::Step(int32 Tick, const FDMMadnessSettings& S)
{
    if (CrisisUntil && Tick >= CrisisUntil)
    { CrisisUntil = 0; Recover(S.CrisisRecovery); RestUntil = Tick + S.CrisisRestTicks; }
    else { Enter(Tick, S); }
}
FString FDMMadnessView::Summary() const
{
    return FString::Printf(TEXT("%s|%.2f|%.2f|%d|%d|%d|%s|%s|%d"), *EntityId, Current, Floor, Band,
        CrisisUntil, GroundUntil, *SymptomId, *SymptomText, SymptomUntil);
}
