#pragma once
#include "CoreMinimal.h"

/**
 * Pure rules shared by the W/E/R kit abilities: no UObjects, no world, no RNG. The kit component and the
 * combatant call these; Foundation tests exercise them directly. Numbers are provisional sandbox tuning.
 */

/** A control request before the Break/Resolve layer (GDD 4.4, O.6) decides how much of it lands. */
struct DREADMERIDIAN_API FDMControl
{
    float Damage = 0;
    /** Fraction of move speed removed for SlowTicks. */
    float Slow = 0;
    int32 SlowTicks = 0;
    FVector Displacement = FVector::ZeroVector;
    /** Basic attack delay; also cancels an active telegraph. */
    int32 StaggerTicks = 0;
    /** Cancels an enemy signature in progress and clears its orders. */
    bool bInterrupt = false;
    /** Resolve damage banked by elites that are not yet broken. */
    float BreakPressure = 0;
};

/** Elite Break/Resolve meter (common enemies never carry one). */
struct DREADMERIDIAN_API FDMBreakMeter
{
    static constexpr float Threshold = 100;
    static constexpr int32 BrokenTicks = 40;
    static constexpr int32 ResistTicks = 100;
    static constexpr float ResistFactor = .5f;

    float Value = 0;
    /** 0 when not broken. */
    int32 BrokenUntil = 0;
    int32 ResistUntil = 0;

    bool IsBroken(int32 Tick) const { return BrokenUntil > Tick; }
    bool IsResisting(int32 Tick) const { return ResistUntil > Tick; }
    /** Adds pressure (halved while resisting, ignored while broken). Returns true on the call that breaks. */
    bool Add(float Amount, int32 Tick);
    /** Returns true on the tick the broken window ends; recovery zeroes the meter and starts resistance. */
    bool Step(int32 Tick);
};

/**
 * Dead Ground: triggers are recorded instead of resolved, then every tag resolves at once after a shared delay.
 * Traps are identified by a stable serial (never an array index): a trap destroyed by any other path (manual
 * detonation, eviction, owner leaving) drops its tags with DropTrap so nothing resolves twice or against the
 * wrong trap. A wire tags only its first crosser (the caller checks HasTag); satchels tag everyone in radius.
 */
struct DREADMERIDIAN_API FDMDeadGroundLedger
{
    static constexpr int32 DelayTicks = 15;
    enum ETrap : uint8 { Satchel = 0, Wire = 1 };
    struct FTag { int32 Enemy = INDEX_NONE; uint8 Trap = Satchel; int32 Serial = INDEX_NONE; };

    TArray<FTag> Tags;
    /** 0 while no batch is open. */
    int32 ResolveTick = 0;

    /** Opens a batch on the first tag. Returns false when that trap already tagged that enemy. */
    bool Tag(int32 Enemy, uint8 Trap, int32 Serial, int32 Tick);
    bool Due(int32 Tick) const { return !Tags.IsEmpty() && Tick >= ResolveTick; }
    bool HasTag(uint8 Trap, int32 Serial) const;
    /** Removes every tag of one trap; closes the batch when nothing is left. Returns the number removed. */
    int32 DropTrap(uint8 Trap, int32 Serial);
    void Clear() { Tags.Reset(); ResolveTick = 0; }
};

/** One active slow. Slows stack by strength: the effective slow is the strongest live entry, never the newest. */
struct DREADMERIDIAN_API FDMSlow
{
    float Fraction = 0;
    int32 UntilTick = 0;
};

namespace DMKitRules
{
    /**
     * Shield actually granted: spirit pulses land repeatedly over a long fight, so the total is capped rather than
     * each gift. Returns 0 for a non-positive or finite-overflowing amount, or when the cap is already met.
     */
    DREADMERIDIAN_API float ShieldGain(float Current, float Cap, float Amount);
    /** Drops expired entries, extends an equal-strength entry, otherwise appends; over MaxEntries the weakest goes. */
    DREADMERIDIAN_API void AddSlow(TArray<FDMSlow>& Slows, float Fraction, int32 UntilTick, int32 Tick, int32 MaxEntries = 4);
    /** Strongest fraction among entries still live at Tick (0 when none). */
    DREADMERIDIAN_API float EffectiveSlow(const TArray<FDMSlow>& Slows, int32 Tick);
    /** 2D cone test: within Length of Origin and within HalfAngleDeg of Dir (Dir is normalised in 2D). A point at the origin counts. */
    DREADMERIDIAN_API bool PointInCone(const FVector& Origin, const FVector& Dir, float HalfAngleDeg, float Length, const FVector& Point);
    DREADMERIDIAN_API float DistanceToSegment2D(const FVector& A, const FVector& B, const FVector& P);
    /** Proper or touching intersection of AB and CD in the XY plane. */
    DREADMERIDIAN_API bool SegmentsCross2D(const FVector& A, const FVector& B, const FVector& C, const FVector& D);
    /**
     * An actor that moved P0 -> P1 this tick touched the wire AB if its path crossed the wire or it now stands
     * within Slack of it (slack covers the capsule radius and logical-tick sampling).
     */
    DREADMERIDIAN_API bool CrossesWire(const FVector& A, const FVector& B, const FVector& P0, const FVector& P1, float Slack);
    /**
     * O.6: common enemies take the full request (no Break tracking); unbroken elites take half the slow and the damage,
     * no displacement, stagger or interrupt, and bank the pressure; broken elites take everything (no further pressure).
     */
    DREADMERIDIAN_API FDMControl ResolveControl(const FDMControl& Requested, bool bCommon, bool bBroken);
}
