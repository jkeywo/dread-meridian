#pragma once
#include "CoreMinimal.h"
#include "DMPing.generated.h"

/**
 * Ping grammar (GDD section 9.3, LOCKED). Tap pings infer the kind from the cursor context; the hold radial
 * exposes the command kinds. AI consumes the same grammar as humans. Perceive is the only subjective kind and
 * never carries a target: it communicates "this investigator perceives something here", not shared truth.
 */
UENUM(BlueprintType)
enum class EDMPingKind : uint8
{
    Enemy,    // "enemy here": raises that target's focus score; idle bots investigate the location
    Focus,    // "kill this one": companions treat the target as a Reflex-rank focus
    Ignore,   // "leave it": that target's focus and act scores are scaled down, never zeroed
    GoHere,   // rally: bots move to the point, then hold there
    Defend,   // hold near the point; engage only enemies within DefendRadius of it
    Retreat,  // all bots disengage toward the point; overrides the Flee goal
    Help,     // rescue if the ally is down, else move to the ally and focus their attacker
    Pickup,   // Sapper bots prefer this pickup
    Perceive, // subjective: bots go and look, never target it
    Count UMETA(Hidden)
};

/** One live ping. Replicated as a public projection on ADMGameState so every HUD draws the same markers. */
USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMPing
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 Id = 0;
    UPROPERTY(BlueprintReadOnly) EDMPingKind Kind = EDMPingKind::Enemy;
    UPROPERTY(BlueprintReadOnly) FString AuthorId;
    UPROPERTY(BlueprintReadOnly) bool bAuthorBot = false;
    UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
    /** EntityId of the pinged combatant; empty for ground, pickup and Perceive pings. */
    UPROPERTY(BlueprintReadOnly) FString TargetId;
    UPROPERTY(BlueprintReadOnly) int32 CreatedTick = 0;
    UPROPERTY(BlueprintReadOnly) int32 ExpiresTick = 0;
    UPROPERTY(BlueprintReadOnly) bool bSubjective = false;
    /** Bots that committed to the ping ("on it"). */
    UPROPERTY(BlueprintReadOnly) TArray<FString> OnIt;
    /** Bots that could not act on it within FDMPingBoard::BusyAfterTicks. */
    UPROPERTY(BlueprintReadOnly) TArray<FString> Busy;
    /** Humans who tapped the ping to acknowledge it. */
    UPROPERTY(BlueprintReadOnly) TArray<FString> Acknowledged;

    bool IsLive(int32 Tick) const { return Tick < ExpiresTick; }
    float AgeFraction(int32 Tick) const
    { return ExpiresTick > CreatedTick ? FMath::Clamp(static_cast<float>(Tick - CreatedTick) / (ExpiresTick - CreatedTick), 0.f, 1.f) : 1.f; }
};

enum class EDMPingEnd : uint8 { Expired, Fulfilled, Cancelled, Replaced };

struct FDMPingEnded
{
    FDMPing Ping;
    EDMPingEnd Reason = EDMPingEnd::Expired;
};

/**
 * Pure ping rules, testable without a world (same pattern as FDMSmugglerWave). ADMCombatGameMode owns one
 * board and projects it to ADMGameState after every StepPings. Nothing here touches actors.
 *
 * Rules (Apex-derived, see docs/pings.md):
 *  - At most MaxPerAuthor live pings per author. A new ping of a kind the author already has live replaces
 *    the author's oldest ping of that kind (ended with Replaced). Beyond the cap, the author's oldest ping
 *    of any kind is replaced.
 *  - An author may not create a ping again until AuthorCooldownTicks after their previous creation.
 *  - Lifetime per kind (ticks): Enemy 150, Focus 300, Ignore 300, GoHere 300, Defend 300, Retreat 200,
 *    Help 300, Pickup 300, Perceive 100.
 *  - NeedsTarget: Enemy, Focus, Ignore, Help require a TargetId; Perceive must NOT have one (it is forced
 *    subjective); GoHere, Defend, Retreat, Pickup carry only a location.
 *  - Cancel succeeds only for the author. Acknowledge is for anyone but the author and is idempotent.
 *  - Respond records on_it/busy per responder once; a later on_it upgrades a busy.
 *  - Step ends expired pings (Expired) and pings for which IsFulfilled returns true (Fulfilled).
 */
struct DREADMERIDIAN_API FDMPingBoard
{
    static constexpr int32 MaxPerAuthor = 3;
    static constexpr int32 AuthorCooldownTicks = 5;
    static constexpr int32 BusyAfterTicks = 20;

    TArray<FDMPing> Pings;
    int32 NextId = 1;
    TMap<FString, int32> AuthorCooldownUntil;

    static int32 LifetimeTicks(EDMPingKind Kind);
    static bool NeedsTarget(EDMPingKind Kind);
    static bool AllowsTarget(EDMPingKind Kind);
    static const TCHAR* KindName(EDMPingKind Kind);
    static const TCHAR* EndName(EDMPingEnd Reason);
    /**
     * World point a marker for this ping is drawn at: above the head for the kinds that name a combatant,
     * just off the ground for the kinds that name a place. The HUD projects it and the cursor hit-tests
     * against it, so both must read this one rule or clicking a marker stops matching what is drawn.
     */
    static FVector MarkerAnchor(EDMPingKind Kind, const FVector& Location);

    /** Returns the new ping id, or INDEX_NONE when rejected (cooldown, target rule violated, empty author). */
    int32 Create(EDMPingKind Kind, const FString& AuthorId, bool bAuthorBot, FVector Location, const FString& TargetId, int32 Tick, TArray<FDMPingEnded>& Ended);
    bool Cancel(int32 Id, const FString& AuthorId, TArray<FDMPingEnded>& Ended);
    bool Acknowledge(int32 Id, const FString& WhoId);
    bool Respond(int32 Id, const FString& ResponderId, bool bOnIt);
    bool HasResponse(int32 Id, const FString& ResponderId) const;
    const FDMPing* Find(int32 Id) const;
    FDMPing* Find(int32 Id);
    int32 LiveCountFor(const FString& AuthorId) const;
    void Step(int32 Tick, TFunctionRef<bool(const FDMPing&)> IsFulfilled, TArray<FDMPingEnded>& Ended);
};
