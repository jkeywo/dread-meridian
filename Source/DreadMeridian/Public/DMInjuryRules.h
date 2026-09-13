#pragma once
#include "CoreMinimal.h"
#include "DMInjuryRules.generated.h"

/** Stable catalogue IDs: append, never reorder. Magnitudes are provisional, GDD M.1 themes are preserved. */
UENUM(BlueprintType)
enum class EDMInjury : uint8 { BrokenRibs, Concussion, WoundedArm, TwistedKnee, DeepCut, Burns, Count };

USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMInjurySettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) int32 WindowTicks = 30;
    UPROPERTY(EditAnywhere) float BurstFraction = .35f;
    UPROPERTY(EditAnywhere) float HeavyFraction = .15f;
    UPROPERTY(EditAnywhere) int32 RepeatHeavyTicks = 30;
    UPROPERTY(EditAnywhere) float RibsMultiplier = 1.25f;
    UPROPERTY(EditAnywhere) int32 ChainCastTicks = 15;
    UPROPERTY(EditAnywhere) int32 ConcussionTicks = 10;
    UPROPERTY(EditAnywhere) int32 AttackPauseTicks = 15;
    UPROPERTY(EditAnywhere) int32 AttackChainLength = 3;
    UPROPERTY(EditAnywhere) int32 ArmRecoveryTicks = 10;
    UPROPERTY(EditAnywhere) int32 MovementWindowTicks = 10;
    UPROPERTY(EditAnywhere) float MovementDistance = 300;
    UPROPERTY(EditAnywhere) int32 LimpTicks = 10;
    UPROPERTY(EditAnywhere) float LimpFraction = .4f;
    UPROPERTY(EditAnywhere) int32 RecoveryTicks = 30;
    UPROPERTY(EditAnywhere) float DeepCutHealingFactor = .5f;
    UPROPERTY(EditAnywhere) int32 HazardRepeatTicks = 20;
    UPROPERTY(EditAnywhere) float BurnsMultiplier = 1.5f;
    UPROPERTY(EditAnywhere) float FoodPulseHealth = 6;
    UPROPERTY(EditAnywhere) int32 FoodPulses = 5;
    UPROPERTY(EditAnywhere) int32 FoodIntervalTicks = 10;
    void Sanitize();
};

struct FDMHealthLoss { int32 Tick = 0; float Amount = 0; };

/** Server-only history. Copyable for rules restoration tests; not a full gameplay migration save. */
struct DREADMERIDIAN_API FDMInjuryState
{
    static constexpr int32 Capacity = 2;
    TArray<EDMInjury> Specific;
    int32 Grievous = 0;
    TArray<FDMHealthLoss> Losses;
    int32 LastHeavyTick = -1000000, LastHazardTick = -1000000;
    int32 LastCastTick = -1000000, LastAttackTick = -1000000;
    int32 CastUntil = 0, AttackUntil = 0, LimpUntil = 0, RecoveryUntil = 0, AttackChain = 0;
    int32 MovementSince = 0;
    float DistanceMoved = 0;
    bool Has(EDMInjury Kind) const { return Specific.Contains(Kind); }
    float RecentLoss(int32 Tick, int32 WindowTicks);
    /** Consumes a burst when it triggers. Down takes priority, so a lethal burst grants one event. */
    bool RecordLoss(float Loss, float MaxHealth, int32 Tick, bool bDown, bool bHazard, const FDMInjurySettings& S);
    /** Uses one draw only while specific capacity remains. Returns Count for a Grievous stack. */
    EDMInjury Gain(uint32 Draw);
    bool Treat();
    float Incoming(float Unshielded, float MaxHealth, int32 Tick, bool bHazard, const FDMInjurySettings& S) const;
    void Cast(int32 Tick, const FDMInjurySettings& S);
    void Attack(int32 Tick, const FDMInjurySettings& S);
    void Move(float Distance, int32 Tick, const FDMInjurySettings& S);
};

namespace DMInjuryRules
{
    DREADMERIDIAN_API const TCHAR* Name(EDMInjury Kind);
    DREADMERIDIAN_API const TCHAR* Effect(EDMInjury Kind);
    DREADMERIDIAN_API const TCHAR* Id(EDMInjury Kind);
}
