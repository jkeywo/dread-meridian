#pragma once
#include "CoreMinimal.h"

/**
 * What one companion is currently committing to, as the rest of the squad hears it.
 *
 * This is communicated intent, not shared sight (GDD 4.13 knowledge rules): a bot publishes what it has
 * decided to do, the way a player would call it out, and teammates may act on the callout. It never grants
 * knowledge of anything the author has not committed to, and enemies never read the board.
 */
UENUM()
enum class EDMClaimKind : uint8
{
    /** "I am on this one": the author's attack focus, by roster index. */
    Focus,
    /** "Mine": a resource cast already committed, with where and when it lands, so nobody stacks a second one. */
    Cast,
    /** "I have them": the author is reviving this downed ally. */
    Rescue,
    /** "Falling back to here": the author's retreat destination. */
    Retreat,
    /** "Wire's down over there": an armed trap, zone or satchel the author has placed. */
    Ground,
    /** "I've got them off you": the author is covering this ally. */
    Peel,
};

/**
 * One claim. Claims are facts about their author's own decision, so they carry no arbitration of their own:
 * a reader weighs them, it is never told what to do.
 */
struct DREADMERIDIAN_API FDMSquadClaim
{
    EDMClaimKind Kind = EDMClaimKind::Focus;
    FString AuthorId;
    /** Focus/Rescue/Peel: roster index of the subject. INDEX_NONE otherwise. */
    int32 TargetIndex = INDEX_NONE;
    /** Cast/Retreat/Ground: where. Ground wires use Location2 for the far end. */
    FVector Location = FVector::ZeroVector;
    FVector Location2 = FVector::ZeroVector;
    /** Cast: effect radius. Ground: the trap's radius, or a wire's band. */
    float Radius = 0;
    /** Cast: expected damage-equivalent magnitude, so a reader can tell a satchel from a flashbulb. */
    float Magnitude = 0;
    /** The tick the claim was published. */
    int32 Tick = 0;
    /** Cast: the tick the effect actually lands (== Tick for instant casts). */
    int32 ResolveTick = 0;
    /** Ground: the placing component's stable marker serial, never an array index. */
    int32 Serial = 0;

    int32 AgeTicks(int32 Now) const { return Now - Tick; }
};

/**
 * Pure squad-intent rules, testable without a world (same pattern as FDMPingBoard). ADMCombatGameMode owns
 * one board and steps it before the Think loop. Nothing here touches actors, and none of it replicates: the
 * player-facing half of coordination is the ping board, which already has a locked grammar and a HUD.
 *
 * Claims live for LifetimeTicks rather than one tick, and that is the whole trick. Bots Think sequentially in
 * roster order and execute before the next one builds its context, so within a single tick a later bot sees
 * an earlier bot's claims but not vice versa. A two-tick lifetime makes the exchange symmetric at a cost of
 * one tick of staleness, instead of restructuring the tick into a gather phase and a commit phase.
 * Readers get AgeTicks and are expected to discount accordingly.
 */
struct DREADMERIDIAN_API FDMSquadBoard
{
    /** Two ticks: this tick's claims plus last tick's, so every bot hears every other bot within 0.1s. */
    static constexpr int32 LifetimeTicks = 2;

    TArray<FDMSquadClaim> Claims;

    static const TCHAR* KindName(EDMClaimKind Kind);

    /** Replaces the author's previous claim of this kind, so a bot speaks once per kind and never floods. */
    void Publish(const FDMSquadClaim& Claim);
    /** Drops claims older than LifetimeTicks. Authors that stop publishing simply age out. */
    void Step(int32 Tick);
    /** Forgets everything an author has said; used when they go down or leave. */
    void ClearAuthor(const FString& AuthorId);
    /** Live claims of a kind, excluding ExceptAuthor (a bot must never read its own intent back). */
    void Gather(EDMClaimKind Kind, const FString& ExceptAuthor, int32 Tick, TArray<FDMSquadClaim>& Out) const;
    /** How many other authors claim Focus on this roster index. */
    int32 FocusCount(int32 TargetIndex, const FString& ExceptAuthor, int32 Tick) const;
    /** The claim of this kind on this subject from anyone but ExceptAuthor, or null. */
    const FDMSquadClaim* FindOnTarget(EDMClaimKind Kind, int32 TargetIndex, const FString& ExceptAuthor, int32 Tick) const;
};
