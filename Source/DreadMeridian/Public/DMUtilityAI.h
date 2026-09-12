#pragma once
#include "CoreMinimal.h"
#include "DMSmugglerComponent.h"
#include "DMInvestigatorComponent.h"
#include "DMPing.h"
#include "DMSquadBoard.h"
#include "DMUtilityAI.generated.h"

/**
 * Utility AI for bot investigators (companions) and enemies.
 *
 * Everything in this header is pure: no world, no UObject pointers, no RNG. ADMSquadController builds an
 * FDMAIContext from the world once per logical tick, DMUtilityAI::Decide turns it into an FDMAIDecision,
 * and the controller executes and traces the decision. Foundation tests exercise Decide directly.
 *
 * Model (see the plan and docs/architecture.md):
 *  - Every action produces zero or more options. An option has a Rank (dual utility: absolute category)
 *    and a Score in [0,1] (weight x compensated product of curved considerations). Options are walked in
 *    (Rank desc, Score desc, action ordinal, target index, point) order.
 *  - Channels: each option reserves Move, Attack and/or Cast. Lower options still execute on channels that
 *    remain free, so Flee (Move) and BasicAttack (Attack) coexist in one tick.
 *  - Conservation: resource-spending options must beat a threshold that rises with scarcity and cooldown
 *    length, scaled by a hit probability for delayed casts, and are vetoed for wasted casts.
 *  - Hysteresis: enter/exit latches (return home, flee, keep distance), per-action runtime caps and
 *    decision cooldowns, and a commitment multiplier on the previous movement action.
 */

// ---------------------------------------------------------------------------------------------- enums

enum class EDMAIChannel : uint8 { None = 0, Move = 1, Attack = 2, Cast = 4, All = 7 };
ENUM_CLASS_FLAGS(EDMAIChannel)

UENUM(BlueprintType)
enum class EDMAIRank : uint8 { Routine = 0, Tactical = 1, Reflex = 2, Locked = 3 };

/** Ordinal is the tie-break key after rank and score. Append only; never reorder. */
UENUM(BlueprintType)
enum class EDMAIAction : uint8
{
    None, HoldCast, Rescue, ReturnHome, EvadeHazard, Flee, RetreatToPing, KeepDistance, SeekPickup, SeekPingedPickup,
    RallyToPing, DefendPing, HelpPing, Strafe, Engage, InvestigatePing, Anchor, Patrol, FollowLeader, Hold,
    BasicAttack, Signature, Throw, HoldFrame, Frame, PlaceSatchel, BindSpirit, Clinch,
    // Named kits (GDD Appendix K, A nodes), in W/E/R order per investigator.
    SuppressingFire, Tripwire, DeadGround, Flashbulb, Develop, ImpossiblePhotograph,
    Beckon, Intercession, OpenSeance, ShoulderThrough, DigIn, DrownedMan,
    Reposition,
    Count UMETA(Hidden)
};

/** Small preset palette (Lewis, Game AI Pro 3). x is the bookend-normalised input in [0,1]. */
UENUM(BlueprintType)
enum class EDMAICurve : uint8
{
    Linear,           // y = x
    Quadratic,        // y = x^Exponent
    InverseQuadratic, // y = 1 - x^Exponent
    Logistic,         // y = 1 / (1 + e^(-Exponent * (x - Midpoint))), renormalised so y(0)=0 and y(1)=1
    Step,             // y = x >= Midpoint ? 1 : 0
    Bell              // y = e^(-((x - Midpoint) * Exponent)^2)
};

/**
 * Raw inputs a consideration can read. The raw value is clamped to the curve's [Min,Max] bookends and
 * normalised before the curve is applied. "Target" is the option's target actor; "Point" its goal point.
 */
UENUM(BlueprintType)
enum class EDMAIInput : uint8
{
    Distance,            // 2D distance from self to the option's target (or point when untargeted), units
    TargetHealthFrac,    // target Health / MaxHealth
    SelfHealthFrac,      // self Health / MaxHealth
    Threat,              // self's threat table value for the target (raw damage units)
    StockFrac,           // Charges / ChargeCapacity (0 when the kit has no stock)
    CooldownFrac,        // Q cooldown remaining / Q cooldown length (0 = ready)
    EnemiesInRadius,     // living hostiles within the option's ability template Radius of the option point
    AlliesInRadius,      // living friendlies (excluding self) within FDMAIWeights::AllyRadius of self
    TargetSpeedOverRadius, // target Speed2D * CastDelayTicks * 0.1 / Radius (0 for instant abilities)
    AnchorDistance,      // 2D distance from self to its anchor (home or patrol point)
    LeaderDistance,      // 2D distance from self to the human leader (0 when there is none)
    HazardDepth,         // how far inside the nearest hostile hazard self stands: (Radius + EvadeMargin) - dist, min 0
    PingFocusOnTarget,   // weight of the strongest live Focus/Enemy ping on the target (0..1)
    PingIgnoreOnTarget,  // weight of the strongest live Ignore ping on the target (0..1)
    PingAge,             // age fraction of the option's ping (0 new .. 1 expiring)
    PingDistance,        // 2D distance from self to the option's ping location
    NotCasting,          // 1 unless self is mid-signature cast
    NotRooted,           // 1 unless self is restrained or framing
    NotFraming,          // 1 unless self has a FrameTarget
    HasSight,            // 1 when self has line of sight to the target (always evaluated last; lazy)
    // Target-choice inputs. Added for focus scoring; usable by any action's considerations.
    AlliesOnTarget,      // teammates whose live Focus claim names this target (0..3)
    AllyInPeril,         // how far below ThreatenedAllyHealth the ally this target is attacking has fallen (0..1)
    TargetSuppressed,    // 1 when the target is suppressed
    Count UMETA(Hidden)
};

// --------------------------------------------------------------------------------------------- data

USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMAICurveSpec
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) EDMAICurve Curve = EDMAICurve::Linear;
    /** Bookends: raw input at Min maps to x=0, at Max maps to x=1. Min must be < Max. */
    UPROPERTY(EditAnywhere) float Min = 0;
    UPROPERTY(EditAnywhere) float Max = 1;
    UPROPERTY(EditAnywhere) float Exponent = 2;
    UPROPERTY(EditAnywhere) float Midpoint = .5f;
    /** Applied after the curve: y = 1 - y. */
    UPROPERTY(EditAnywhere) bool bInvert = false;

    FDMAICurveSpec() = default;
    FDMAICurveSpec(EDMAICurve InCurve, float InMin, float InMax, float InExponent = 2, float InMidpoint = .5f, bool bInInvert = false)
        : Curve(InCurve), Min(InMin), Max(InMax), Exponent(InExponent), Midpoint(InMidpoint), bInvert(bInInvert) {}
    /** Clamp/normalise Raw with the bookends, apply the curve, invert if asked. Always returns [0,1]. */
    float Evaluate(float Raw) const;
};

/**
 * One additive term of the focus score. Unlike an action's considerations, which multiply into [0,1], focus
 * terms sum into a score measured in the same units as the distance the formula subtracts: a Weight of 250
 * says "worth closing 250 units for". Keeping the scale additive is what lets Distance, TargetCommitment and
 * the ping bonuses stay exactly as tuned while the reasons to prefer one target over another are added.
 */
USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMAIFocusTerm
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) EDMAIInput Input = EDMAIInput::TargetHealthFrac;
    UPROPERTY(EditAnywhere) FDMAICurveSpec Curve;
    /** Score added at curve value 1, in distance units. Negative discourages. */
    UPROPERTY(EditAnywhere) float Weight = 0;
    FDMAIFocusTerm() = default;
    FDMAIFocusTerm(EDMAIInput InInput, const FDMAICurveSpec& InCurve, float InWeight)
        : Input(InInput), Curve(InCurve), Weight(InWeight) {}
};

USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMAIConsideration
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) EDMAIInput Input = EDMAIInput::Distance;
    UPROPERTY(EditAnywhere) FDMAICurveSpec Curve;
    FDMAIConsideration() = default;
    FDMAIConsideration(EDMAIInput InInput, const FDMAICurveSpec& InCurve) : Input(InInput), Curve(InCurve) {}
};

/**
 * One action's data. Code decides when an action has candidates and applies its mandatory switches; the
 * spec shapes the score. Considerations are evaluated in order and early-out on the first zero, so put
 * cheap switches first and HasSight last.
 */
USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMAIActionSpec
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) EDMAIAction Action = EDMAIAction::None;
    UPROPERTY(EditAnywhere) EDMAIRank Rank = EDMAIRank::Routine;
    UPROPERTY(EditAnywhere) float Weight = 1;
    /** EDMAIChannel bits. */
    UPROPERTY(EditAnywhere) uint8 ChannelMask = 1;
    UPROPERTY(EditAnywhere) TArray<FDMAIConsideration> Considerations;
    /** After this many consecutive chosen ticks the score falls to zero (curve 1 - x^6). 0 disables. */
    UPROPERTY(EditAnywhere) int32 MaxRuntimeTicks = 0;
    /** After the action stops being chosen, its score is suppressed (curve x^5) for this many ticks. 0 disables. */
    UPROPERTY(EditAnywhere) int32 DecisionCooldownTicks = 0;
    UPROPERTY(EditAnywhere) bool bEnabled = true;

    EDMAIChannel Channels() const { return static_cast<EDMAIChannel>(ChannelMask); }
};

/** Shared decision template for resource-spending abilities (signatures and base Q). */
USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMAIAbilityTemplate
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) float Range = 0;
    /** Effect radius; 0 for single-target. Used by EnemiesInRadius and PHit. */
    UPROPERTY(EditAnywhere) float Radius = 0;
    /** Primary effect magnitude per affected target (damage-equivalent units). */
    UPROPERTY(EditAnywhere) float Magnitude = 0;
    /** Secondary magnitude; meaning is per ability (see DMUtilityAI.cpp value functions). */
    UPROPERTY(EditAnywhere) float SecondaryMagnitude = 0;
    /** Conservation base threshold before scarcity and cooldown scaling. */
    UPROPERTY(EditAnywhere) float Base = 10;
    UPROPERTY(EditAnywhere) int32 CooldownTicks = 0;
    /** Ticks between activation and effect; drives the "dying" veto and PHit. */
    UPROPERTY(EditAnywhere) int32 CastDelayTicks = 0;
    /** Rooted casts reserve Move and Attack as well as Cast. */
    UPROPERTY(EditAnywhere) bool bRoots = false;
    /**
     * Caps the cooldown term of the conservation threshold (0 = uncapped). An ultimate's 600-tick cooldown would
     * otherwise multiply its threshold by 6-11x through the shared KCooldown, which no reachable fight can clear,
     * and lowering KCooldown to compensate would loosen every Q on the same profile.
     */
    UPROPERTY(EditAnywhere) int32 ThresholdCooldownCap = 0;
    /** Cooldown the threshold actually uses. */
    int32 ThresholdCooldown() const { return ThresholdCooldownCap > 0 ? FMath::Min(CooldownTicks, ThresholdCooldownCap) : CooldownTicks; }
};

/**
 * Everything the brain reads for one role or hero. Defaults come from DMUtilityAI::DefaultWeights; the
 * per-role UDMAIProfile data assets expose the same struct for tuning. All numbers are provisional.
 */
USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMAIWeights
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) TArray<FDMAIActionSpec> Actions;
    UPROPERTY(EditAnywhere) TMap<EDMAIAction, FDMAIAbilityTemplate> Abilities;
    /** Added to every focus candidate's score. Empty leaves ChooseFocus on distance, commitment and pings alone. */
    UPROPERTY(EditAnywhere) TArray<FDMAIFocusTerm> FocusTerms;

    // Conservation
    UPROPERTY(EditAnywhere) float KStock = 2;
    UPROPERTY(EditAnywhere) float KCooldown = 1;
    UPROPERTY(EditAnywhere) FDMAICurveSpec PHitCurve = FDMAICurveSpec(EDMAICurve::InverseQuadratic, 0, 2, 2);
    /** Worth multipliers: 1 + EliteWorth for elites, + MarkedWorth for marked/forced targets. */
    UPROPERTY(EditAnywhere) float EliteWorth = .5f;
    UPROPERTY(EditAnywhere) float MarkedWorth = .25f;

    // Hysteresis
    UPROPERTY(EditAnywhere) float CommitFactor = 1.15f;
    UPROPERTY(EditAnywhere) int32 CommitTicks = 20;
    UPROPERTY(EditAnywhere) float TargetCommitment = 75;
    UPROPERTY(EditAnywhere) float FleeEnter = .25f;
    UPROPERTY(EditAnywhere) float FleeExit = .4f;
    UPROPERTY(EditAnywhere) float FleeEnemyRadius = 400;
    UPROPERTY(EditAnywhere) float FleeSafeRadius = 600;
    UPROPERTY(EditAnywhere) float FleeDistance = 400;
    UPROPERTY(EditAnywhere) float KeepDistanceEnter = .5f;
    UPROPERTY(EditAnywhere) float KeepDistanceExit = .7f;
    UPROPERTY(EditAnywhere) float KeepDistanceStep = 300;
    UPROPERTY(EditAnywhere) float SetPositionReluctance = .6f;
    UPROPERTY(EditAnywhere) float EvadeMargin = 40;

    // Perception and geometry (parity with the previous cascade)
    UPROPERTY(EditAnywhere) float SightRange = 600;
    UPROPERTY(EditAnywhere) float LeashRange = 1800;
    UPROPERTY(EditAnywhere) float LeashReturnRadius = 100;
    UPROPERTY(EditAnywhere) float AnchorRadius = 90;
    UPROPERTY(EditAnywhere) float FollowDistance = 240;
    UPROPERTY(EditAnywhere) float TetherLeaderRadius = 850;
    UPROPERTY(EditAnywhere) float TetherSelfRadius = 500;
    UPROPERTY(EditAnywhere) float ApproachBand = 35;
    UPROPERTY(EditAnywhere) float RescueRadius = 140;
    UPROPERTY(EditAnywhere) float StrafeRadius = 650;
    UPROPERTY(EditAnywhere) float StrafeOffset = 460;
    UPROPERTY(EditAnywhere) float StrafeAngle = 35;
    UPROPERTY(EditAnywhere) float PickupRadius = 700;
    UPROPERTY(EditAnywhere) float AllyRadius = 900;
    UPROPERTY(EditAnywhere) float ThreatenedAllyHealth = 50;

    // Pings
    UPROPERTY(EditAnywhere) float PingCompliance = 1;
    UPROPERTY(EditAnywhere) float BotPingWeight = .5f;
    UPROPERTY(EditAnywhere) float PingEnemyScore = 250;
    UPROPERTY(EditAnywhere) float PingIgnorePenalty = 5000;
    UPROPERTY(EditAnywhere) float PingActBoost = .3f;
    UPROPERTY(EditAnywhere) float PingIgnoreActPenalty = .8f;
    UPROPERTY(EditAnywhere) float RallyArriveRadius = 150;
    UPROPERTY(EditAnywhere) float DefendHoldRadius = 250;
    UPROPERTY(EditAnywhere) float DefendEngageRadius = 500;
    UPROPERTY(EditAnywhere) float HelpArriveRadius = 200;
    UPROPERTY(EditAnywhere) int32 BotPingCooldownTicks = 50;

    // Positional scoring. ChoosePosition samples points and scores them on these; see DMUtilityAI.cpp.
    /** How far out along each whisker a candidate stand-point is sampled. */
    UPROPERTY(EditAnywhere) float PositionStep = 320;
    /** Hostiles beyond this contribute no danger to a candidate point. */
    UPROPERTY(EditAnywhere) float PositionDangerRadius = 750;
    UPROPERTY(EditAnywhere) float PositionDangerWeight = 1.4f;
    /** Standing in a telegraphed circle. Large: this is a switch, not a preference. */
    UPROPERTY(EditAnywhere) float PositionHazardWeight = 4;
    /** How strongly a point is wanted at the distance the current intent asks for. */
    UPROPERTY(EditAnywhere) float PositionRangeWeight = 1.6f;
    UPROPERTY(EditAnywhere) float PositionAllyWeight = .45f;
    /** Closer than this to a teammate is crowding: one bomb, two casualties. */
    UPROPERTY(EditAnywhere) float PositionClumpRadius = 170;
    UPROPERTY(EditAnywhere) float PositionClumpWeight = .8f;
    /** Pull toward armed traps and zones, the team's or this bot's own, when falling back or repositioning. */
    UPROPERTY(EditAnywhere) float PositionGroundRadius = 420;
    UPROPERTY(EditAnywhere) float PositionGroundWeight = .7f;
    /** Cost of walking there at all: the hysteresis that stops a bot skating between equally good points. */
    UPROPERTY(EditAnywhere) float PositionTravelWeight = .55f;

    /** Reserved: weighted random within this fraction of the top score, drawn from EDMRandomStream::AIChoice. 0 = argmax. */
    UPROPERTY(EditAnywhere) float TopBandFraction = 0;

    const FDMAIActionSpec* FindAction(EDMAIAction Action) const;
    FDMAIActionSpec* FindAction(EDMAIAction Action);
    const FDMAIAbilityTemplate* FindAbility(EDMAIAction Action) const;
};

// -------------------------------------------------------------------------------------------- views

/** One roster entry as the deciding bot sees it. Index is the roster index in ADMCombatGameMode::GetCombatants. */
struct DREADMERIDIAN_API FDMAIActorView
{
    int32 Index = INDEX_NONE;
    FString EntityId;
    bool bEnemy = false, bDown = false, bRestrained = false, bPlayerControlled = false, bCommon = true, bBreakVulnerable = false;
    /** Line of sight from self. Only meaningful for hostiles; the context builder traces lazily (see BuildContext). */
    bool bVisible = true;
    /** A living friend in the same encounter group targets or holds threat on this actor. */
    bool bGroupEngaged = false;
    bool bForced = false, bMarked = false, bDiver = false;
    bool bBoundByMe = false, bHeldByMe = false, bRanged = false;
    int32 EncounterGroup = INDEX_NONE;
    /** Roster index this actor is attacking, or INDEX_NONE. */
    int32 AttackTargetIndex = INDEX_NONE;
    float Health = 0, MaxHealth = 0, Shield = 0;
    float Distance = 0;          // 3D, for focus-score parity with FVector::Distance
    float Distance2D = 0;
    float AnchorDistance2D = 0;  // from self's anchor
    float LeaderDistance2D = 0;  // from the leader (0 when none)
    float Threat = 0;            // self->Threat[EntityId]
    /** Damage from friendlies already targeting this actor that lands within the ability's cast delay (filled per option by the pure code from AttackDamage/AttackInterval of attackers). */
    float ExposureMultiplier = 1;
    float AttackDamage = 0;
    int32 AttackInterval = 10;
    int32 NextAttackIn = 0;      // ticks until this actor's next basic attack is ready (0 = ready)
    float Speed2D = 0;
    /** Full 2D velocity; Speed2D stays for PHit. Needed to tell an approaching focus from a retreating one. */
    FVector Velocity2D = FVector::ZeroVector;
    FVector Location = FVector::ZeroVector;
    /** Raw Exposure this Photographer has stored on the actor (0..100). */
    float Exposure = 0;
    bool bSuppressed = false;
    /** Mid-telegraph: Perfect Moment and Flashbulb value these higher. */
    bool bCommitted = false;
    /** Elite Resolve damage banked so far (0..FDMBreakMeter::Threshold). */
    float Break = 0;
};

/** One of the deciding bot's own persistent markers. Enemy hazards stay in FDMAIHazard. */
struct DREADMERIDIAN_API FDMAIMarkerView
{
    enum EKind : uint8 { Satchel, Wire, Zone, Spirit };
    EKind Kind = Satchel;
    FVector Location = FVector::ZeroVector;
    /** Wire: the far end. Zone: unused (see Direction/HalfAngle/Length). */
    FVector WireEnd = FVector::ZeroVector;
    FVector Direction = FVector::ZeroVector;
    float HalfAngle = 0;
    float Length = 0;
    float Radius = 0;
    bool bArmed = false;
    bool bTravelling = false;
    /** Roster index this marker is bound to, or INDEX_NONE for a ground binding. */
    int32 BoundIndex = INDEX_NONE;
    float Attention = 0;
    int32 Serial = 0;
    FString SpiritId;
};

struct DREADMERIDIAN_API FDMAIHazard
{
    FVector Center = FVector::ZeroVector;
    float Radius = 0;
};

struct DREADMERIDIAN_API FDMAIPingView
{
    int32 Id = INDEX_NONE;
    EDMPingKind Kind = EDMPingKind::Enemy;
    bool bHuman = false;
    bool bAuthoredBySelf = false;
    bool bRespondedBySelf = false;
    FVector Location = FVector::ZeroVector;
    int32 TargetIndex = INDEX_NONE;
    float AgeFrac = 0;
    int32 AgeTicks = 0;
    /** (bHuman ? 1 : BotPingWeight) * PingCompliance, clamped to [0,1]. Filled by the context builder. */
    float Weight = 1;
};

/**
 * One teammate's published intent, as this bot hears it. A view rather than the claim itself so the pure
 * layer never sees entity ids: TargetIndex is already resolved to a roster index by the context builder.
 */
struct DREADMERIDIAN_API FDMAIClaimView
{
    EDMClaimKind Kind = EDMClaimKind::Focus;
    /** Roster index of the author, so a reader can weigh who is speaking. */
    int32 AuthorIndex = INDEX_NONE;
    int32 TargetIndex = INDEX_NONE;
    FVector Location = FVector::ZeroVector;
    FVector Location2 = FVector::ZeroVector;
    float Radius = 0, Magnitude = 0;
    /** 0 for a claim made this tick, 1 for last tick's. Stale claims should count for less. */
    int32 AgeTicks = 0;
    /** Cast: ticks from now until it lands; negative once it has. */
    int32 ResolveIn = 0;
    int32 Serial = 0;
};

struct DREADMERIDIAN_API FDMAISelfView
{
    int32 Index = INDEX_NONE;
    FString EntityId;
    bool bEnemy = false, bProfileRange = false, bLocalEnemy = false, bPatrolMember = false, bCompanionTethered = false;
    bool bCasting = false, bRestrained = false, bAttackReady = false, bSetPosition = false, bRanged = false;
    EDMSmuggler Role = EDMSmuggler::None;
    EDMInvestigator Kind = EDMInvestigator::None;
    float Health = 0, MaxHealth = 0, AttackRange = 0, AttackDamage = 0;
    int32 AttackInterval = 10;
    FVector Location = FVector::ZeroVector, Anchor = FVector::ZeroVector;
    int32 EncounterGroup = INDEX_NONE, PatrolWaypoint = 0;
    int32 Charges = 0, ChargeCapacity = 3, Satchels = 0, Bindings = 0;
    float Momentum = 0;
    bool bQReady = false;
    int32 QCooldownRemaining = 0;
    int32 FrameTarget = INDEX_NONE, HeldTarget = INDEX_NONE;
    // Named kits. Ready flags mirror UDMKitComponent::IsReady; the windows mirror its replicated *UntilTick fields.
    bool bWReady = false, bEReady = false, bRReady = false;
    int32 WCooldownRemaining = 0, ECooldownRemaining = 0, RCooldownRemaining = 0;
    bool bRActive = false, bBraced = false, bCharging = false, bWirePending = false;
    int32 Zones = 0, Wires = 0;
    /** Highest Attention across this Medium's bound spirits. */
    float MaxAttention = 0;
    float Madness = 0;
    bool bSignatureReady = false;
    /** Sight to the focus for the signature; the builder evaluates it only when bSignatureReady and a focus exists. */
    bool bSignatureSight = false;
};

/** Persisted on the controller between ticks. Decide reads Context.Memory and returns the updated copy. */
struct DREADMERIDIAN_API FDMAIMemory
{
    int32 TargetIndex = INDEX_NONE;
    int32 TargetSinceTick = 0;
    EDMAIAction LastMove = EDMAIAction::None;
    EDMAIAction LastAct = EDMAIAction::None;
    int32 MoveSinceTick = 0;
    TMap<EDMAIAction, int32> RuntimeTicks;   // consecutive ticks the action was chosen
    TMap<EDMAIAction, int32> CooldownUntil;  // decision cooldown deadline per action
    bool bReturningHome = false, bFleeing = false, bKeepingDistance = false;
    int32 LastPingTick = -1000;
};

struct DREADMERIDIAN_API FDMAIContext
{
    int32 Tick = 0;
    FDMAISelfView Self;
    TArray<FDMAIActorView> Actors;
    int32 LeaderIndex = INDEX_NONE;
    TArray<FVector> Pickups;
    TArray<FDMAIHazard> Hazards;
    TArray<FDMAIPingView> Pings;
    /** The deciding bot's own satchels, wires, zones and spirits, so kit options can reason about their geometry. */
    TArray<FDMAIMarkerView> Markers;
    /**
     * Distance to the first blocker along each of Clearance.Num() rays, evenly spaced from +X counter-clockwise,
     * capped at the longest step the brain will take. The pure layer cannot trace, and the arena has no nav mesh:
     * a bot steers straight at its goal and grinds into whatever is in the way. These whiskers are how a candidate
     * stand-point is known to be reachable, and a short one is also the only usable hint that there is cover there.
     * Empty when unsensed, which every candidate then treats as open ground.
     */
    TArray<float> Clearance;
    /** What teammates have said they are doing. Never contains the deciding bot's own claims. */
    TArray<FDMAIClaimView> Claims;
    FVector PlayableExtent = FVector(2900, 2400, 0);
    const FDMAIWeights* W = nullptr;
    FDMAIMemory Memory;
};

// ------------------------------------------------------------------------------------------ outputs

struct DREADMERIDIAN_API FDMAIScoredConsideration
{
    EDMAIInput Input = EDMAIInput::Distance;
    float Raw = 0;
    float Score = 0;
};

struct DREADMERIDIAN_API FDMAIOption
{
    EDMAIAction Action = EDMAIAction::None;
    int32 Target = INDEX_NONE;
    FVector Point = FVector::ZeroVector;
    /** Second point for two-point casts (Tripwire); zero otherwise. */
    FVector Point2 = FVector::ZeroVector;
    EDMAIChannel Channels = EDMAIChannel::None;
    EDMAIRank Rank = EDMAIRank::Routine;
    /** Nominally [0,1]; commitment can push slightly above 1. 0 means not viable. */
    float Score = 0;
    float Value = 0, Threshold = 0, PHit = 1;
    int32 PingId = INDEX_NONE;
    TArray<FDMAIScoredConsideration> Breakdown;
    /** Set when vetoed: dying, out_of_range, redundant, no_sight, cooldown, no_stock, casting, profile, runtime, disabled. */
    const TCHAR* Veto = nullptr;
    /** Executing this option also clears the focus (Rescue, ReturnHome, HoldCast). */
    bool bClearsFocus = false;
};

struct DREADMERIDIAN_API FDMAIPingRequest
{
    EDMPingKind Kind = EDMPingKind::Enemy;
    int32 TargetIndex = INDEX_NONE;
    FVector Location = FVector::ZeroVector;
};

struct DREADMERIDIAN_API FDMAIDecision
{
    int32 Focus = INDEX_NONE;
    EDMAIRank FocusRank = EDMAIRank::Routine;
    float FocusScore = 0;
    /** Every option, vetoed ones included, in final sort order. */
    TArray<FDMAIOption> Ranked;
    TArray<FDMAIOption> Chosen;
    EDMAIChannel Taken = EDMAIChannel::None;
    FDMAIMemory Memory;
    bool bAdvanceWaypoint = false;
    bool bClearThreat = false;
    /** True when BasicAttack was not chosen: the controller sets ADMCombatant::bAttackHold accordingly. */
    bool bAttackHold = true;
    /** True when a chosen option clears the focus (the controller passes nullptr to SetAttackTarget). */
    bool bClearFocus = false;
    TArray<FDMAIPingRequest> PingRequests;
    /** Intent this decision commits to, for the controller to publish on the squad board. */
    TArray<FDMSquadClaim> Claims;
    /** Ping ids that shaped a chosen option this tick (the controller answers them with on_it). */
    TArray<int32> PingsOnIt;

    const FDMAIOption* ChosenOn(EDMAIChannel Channel) const;
    bool Chose(EDMAIAction Action) const;
};

// ---------------------------------------------------------------------------------------------- API

/** What a movement option is trying to achieve, which is what decides whether a stand-point is any good. */
enum class EDMAIIntent : uint8
{
    KeepDistance, // stand outside the focus's reach
    Flee,         // get away from the nearest hostile, toward help and prepared ground
    Evade,        // out of the fire, and nothing else matters much
    Reposition,   // already in range: stand somewhere better while shooting
};

namespace DMUtilityAI
{
    /** Mark's compensation: each score s becomes s + (1 - s) * (1 - 1/n) * s; then the product. Any zero gives zero. */
    DREADMERIDIAN_API float Combine(TArrayView<const float> Scores);

    /**
     * Baseline weights per enemy role or investigator kind (a Role of None with a Kind of None is the generic
     * "Human Raider" used by smoke fixtures). Every action/ability default in the plan's tables lives here.
     */
    DREADMERIDIAN_API FDMAIWeights DefaultWeights(EDMSmuggler Role, EDMInvestigator Kind);

    /**
     * Focus (attack target) selection, a behavioural port of the previous cascade:
     *  - candidates are living actors on the other team;
     *  - local enemies (bLocalEnemy) skip candidates farther than LeashRange from the anchor, and unalerted
     *    candidates beyond SightRange or without sight. Alerted = forced/marked/diver, Threat > 0,
     *    current target, or bGroupEngaged;
     *  - tethered companions skip candidates farther than TetherLeaderRadius from the leader AND farther
     *    than TetherSelfRadius from self;
     *  - with a live Defend ping, companions skip candidates farther than DefendEngageRadius from the point
     *    unless the candidate is attacking self;
     *  - rank: Locked for Forced, Reflex for Diver (enemies) or a Focus ping (companions), Tactical for
     *    Marked (enemies) or the attacker of a Help-pinged ally (companions), Routine otherwise;
     *  - score within rank: (bEnemy ? Threat * 1000 : 0) - Distance + (current target ? TargetCommitment : 0)
     *    + PingEnemyScore * ping weight - PingIgnorePenalty * ignore weight.
     * Returns the roster index or INDEX_NONE.
     */
    DREADMERIDIAN_API int32 ChooseFocus(const FDMAIContext& Context, EDMAIRank* OutRank = nullptr, float* OutScore = nullptr);

    /** Appends every option (viable and vetoed) for the context. Focus may be INDEX_NONE. */
    DREADMERIDIAN_API void BuildOptions(const FDMAIContext& Context, int32 Focus, TArray<FDMAIOption>& Out);

    /**
     * Stable sort (Rank desc, Score desc, action ordinal asc, Target asc, Point.X, Point.Y) into Out.Ranked, then walk
     * the list reserving channels: skip vetoed or zero-score options, skip options whose channels intersect Taken,
     * otherwise choose and add its channels to Taken. Stops when Taken == All.
     */
    DREADMERIDIAN_API void Select(TArray<FDMAIOption>& Options, FDMAIDecision& Out);

    /**
     * Full decision: update latches, choose focus, build options, select, derive flags, request bot pings and
     * update memory (target, last move/act, runtime counters, decision cooldowns).
     * Casting self: the decision holds only HoldCast (All channels, Locked) with Focus INDEX_NONE and bClearFocus.
     */
    DREADMERIDIAN_API FDMAIDecision Decide(const FDMAIContext& Context);

    // Conservation helpers (pure, unit-tested)
    DREADMERIDIAN_API float Threshold(float Base, int32 Stock, int32 Capacity, int32 CooldownTicks, float KStock, float KCooldown);
    DREADMERIDIAN_API float ConservedScore(float Weight, float Value, float Threshold);
    /** Health + Shield - damage that friendlies already attacking the target will land within CastDelayTicks. */
    DREADMERIDIAN_API float ExpectedHealthAtResolve(const FDMAIContext& Context, int32 TargetIndex, int32 CastDelayTicks);
    /** Strongest live ping weight of the given kind(s) on a roster index (Focus and Enemy both count for PingFocusOnTarget). */
    DREADMERIDIAN_API float PingWeightOnTarget(const FDMAIContext& Context, int32 TargetIndex, EDMPingKind Kind);

    /**
     * Best stand-point for an intent, by scoring sampled candidates on incoming danger, hazards, the distance the
     * intent wants, teammate spacing, prepared ground and the cost of walking there.
     *
     * Candidates are the bot's current position, Fallback (what the fixed geometry would have chosen, so this can
     * never do worse than the formula it replaces), and points along each whisker clamped to its clearance.
     * Pure and deterministic: no RNG, and ties break on candidate order.
     */
    DREADMERIDIAN_API FVector ChoosePosition(const FDMAIContext& Context, EDMAIIntent Intent, const FDMAIActorView* Focus, const FVector& Fallback);

    DREADMERIDIAN_API const TCHAR* ActionName(EDMAIAction Action);
    DREADMERIDIAN_API const TCHAR* RankName(EDMAIRank Rank);
    DREADMERIDIAN_API const TCHAR* InputName(EDMAIInput Input);
    DREADMERIDIAN_API FString ChannelString(EDMAIChannel Channels);
    DREADMERIDIAN_API EDMAIAction ActionFromName(const FString& Name);
}
