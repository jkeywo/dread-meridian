#include "DMInjuryRules.h"

void FDMInjurySettings::Sanitize()
{
    for (int32* V : { &WindowTicks, &RepeatHeavyTicks, &ChainCastTicks, &ConcussionTicks, &AttackPauseTicks,
        &AttackChainLength, &ArmRecoveryTicks, &MovementWindowTicks, &LimpTicks, &RecoveryTicks, &HazardRepeatTicks, &FoodPulses, &FoodIntervalTicks })
    { *V = FMath::Clamp(*V, 1, 10000); }
    auto Clamp = [](float& V, float Default, float Min, float Max) { V = FMath::IsFinite(V) ? FMath::Clamp(V, Min, Max) : Default; };
    Clamp(BurstFraction, .35f, .01f, 1.f); Clamp(HeavyFraction, .15f, .01f, 1.f);
    Clamp(RibsMultiplier, 1.25f, 1.f, 5.f); Clamp(BurnsMultiplier, 1.5f, 1.f, 5.f);
    Clamp(DeepCutHealingFactor, .5f, 0.f, 1.f); Clamp(LimpFraction, .4f, 0.f, .9f);
    Clamp(MovementDistance, 300, 1, 100000);
    Clamp(FoodPulseHealth, 6, 0, 100000);
}
float FDMInjuryState::RecentLoss(int32 Tick, int32 WindowTicks)
{
    Losses.RemoveAll([&](const FDMHealthLoss& L) { return Tick - L.Tick >= WindowTicks; });
    float Sum = 0; for (const auto& L : Losses) { Sum += L.Amount; } return Sum;
}
bool FDMInjuryState::RecordLoss(float Loss, float MaxHealth, int32 Tick, bool bDown, bool bHazard, const FDMInjurySettings& S)
{
    RecentLoss(Tick, S.WindowTicks);
    if (!FMath::IsFinite(Loss) || Loss <= 0 || !FMath::IsFinite(MaxHealth) || MaxHealth <= 0) { return false; }
    // Coalesce a tick so high-frequency hits cannot grow the rolling history without bound.
    if (Losses.Num() && Losses.Last().Tick == Tick) { Losses.Last().Amount += Loss; }
    else { Losses.Add({ Tick, Loss }); }
    if (Loss >= MaxHealth * S.HeavyFraction) { LastHeavyTick = Tick; }
    if (bHazard) { LastHazardTick = Tick; }
    const bool bBurst = RecentLoss(Tick, S.WindowTicks) >= MaxHealth * S.BurstFraction;
    if (bBurst) { RecoveryUntil = Tick + S.RecoveryTicks; }
    if (!bDown && !bBurst) { return false; }
    Losses.Reset(); return true;
}
EDMInjury FDMInjuryState::Gain(uint32 Draw)
{
    if (Specific.Num() >= Capacity) { Grievous = FMath::Min(Grievous + 1, 1000000); return EDMInjury::Count; }
    TArray<EDMInjury> Available;
    for (uint8 I = 0; I < static_cast<uint8>(EDMInjury::Count); ++I)
    { if (!Has(static_cast<EDMInjury>(I))) { Available.Add(static_cast<EDMInjury>(I)); } }
    const EDMInjury Kind = Available[Draw % Available.Num()]; Specific.Add(Kind); return Kind;
}
bool FDMInjuryState::Treat()
{
    if (Grievous > 0) { --Grievous; return true; }
    if (Specific.IsEmpty()) { return false; }
    Specific.RemoveAt(0); return true;
}
float FDMInjuryState::Incoming(float Unshielded, float MaxHealth, int32 Tick, bool bHazard, const FDMInjurySettings& S) const
{
    float Multiplier = 1;
    if (Has(EDMInjury::BrokenRibs) && Unshielded >= MaxHealth * S.HeavyFraction && Tick - LastHeavyTick < S.RepeatHeavyTicks)
    { Multiplier *= S.RibsMultiplier; }
    if (Has(EDMInjury::Burns) && bHazard && Tick - LastHazardTick < S.HazardRepeatTicks) { Multiplier *= S.BurnsMultiplier; }
    return Multiplier;
}
void FDMInjuryState::Cast(int32 Tick, const FDMInjurySettings& S)
{
    if (Has(EDMInjury::Concussion) && Tick - LastCastTick < S.ChainCastTicks) { CastUntil = Tick + S.ConcussionTicks; }
    LastCastTick = Tick;
}
void FDMInjuryState::Attack(int32 Tick, const FDMInjurySettings& S)
{
    AttackChain = Tick - LastAttackTick >= S.AttackPauseTicks ? 1 : AttackChain + 1;
    if (Has(EDMInjury::WoundedArm) && AttackChain >= S.AttackChainLength) { AttackUntil = Tick + S.ArmRecoveryTicks; AttackChain = 0; }
    LastAttackTick = Tick;
}
void FDMInjuryState::Move(float Distance, int32 Tick, const FDMInjurySettings& S)
{
    if (!FMath::IsFinite(Distance) || Distance < 0) { return; }
    if (Tick - MovementSince >= S.MovementWindowTicks) { MovementSince = Tick; DistanceMoved = 0; }
    DistanceMoved += Distance;
    if (Has(EDMInjury::TwistedKnee) && DistanceMoved >= S.MovementDistance && Tick >= LimpUntil)
    { LimpUntil = Tick + S.LimpTicks; DistanceMoved = 0; MovementSince = Tick; }
}
namespace DMInjuryRules
{
    const TCHAR* Name(EDMInjury K)
    { const TCHAR* Names[] = { TEXT("Broken Ribs"), TEXT("Concussion"), TEXT("Wounded Arm"), TEXT("Twisted Knee"), TEXT("Deep Cut"), TEXT("Burns"), TEXT("Grievous") }; return Names[FMath::Min(static_cast<int32>(K), 6)]; }
    const TCHAR* Effect(EDMInjury K)
    { const TCHAR* Effects[] = { TEXT("Repeated heavy hits hurt more"), TEXT("Rapid casts force a casting pause"), TEXT("Repeated attacks force an attack pause"), TEXT("Fast travel or shoves cause a limp"), TEXT("Burst trauma weakens healing briefly"), TEXT("Repeated hazard hits hurt more"), TEXT("Longer revive; treat this first") }; return Effects[FMath::Min(static_cast<int32>(K), 6)]; }
    const TCHAR* Id(EDMInjury K)
    { const TCHAR* Ids[] = { TEXT("injury.broken_ribs"), TEXT("injury.concussion"), TEXT("injury.wounded_arm"), TEXT("injury.twisted_knee"), TEXT("injury.deep_cut"), TEXT("injury.burns"), TEXT("injury.grievous") }; return Ids[FMath::Min(static_cast<int32>(K), 6)]; }
}
