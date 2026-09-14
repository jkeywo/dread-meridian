#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "DMUtilityAI.h"
#include "DMSquadController.generated.h"

class ADMCombatGameMode;
class UDMAIProfile;

/**
 * Bot controller for companions and enemies. Think builds a pure FDMAIContext from the world, asks
 * DMUtilityAI::Decide for a decision, executes it through the existing combatant verbs, answers pings and
 * emits an ai.decision trace when the decision changes. The same path runs in interactive and headless play.
 */
UCLASS()
class DREADMERIDIAN_API ADMSquadController : public AAIController
{
    GENERATED_BODY()
public:
    // Same decision/command path in interactive and headless play; no forced test outcomes.
    void Think(ADMCombatGameMode& Mode);
    void ConfigureEncounter(int32 Group, FVector Home, bool bPatrol, int32 Slot);
    int32 GetEncounterGroup() const { return EncounterGroup; }
    int32 GetPatrolWaypoint() const { return PatrolWaypoint; }
    const FDMAIDecision& GetLastDecision() const { return LastDecision; }
    /** The context the last Think decided from. Retained as the observation seam for the PIE vision tests. */
    const FDMAIContext& GetLastContext() const { return LastContext; }
    const FDMAIMemory& GetMemory() const { return Memory; }
    /** Weights resolved by the game mode (ADMCombatGameMode::ProfileFor); resolved lazily on first Think when unset. */
    void SetProfile(UDMAIProfile* InProfile) { Profile = InProfile; }
    UDMAIProfile* GetProfile() const { return Profile; }
private:
    /**
     * Reads the world once per tick. Perception parity with the previous cascade: line-of-sight traces
     * (AAIController::LineSightTo) only for hostiles within SightRange that are not otherwise alerted, and the
     * signature sight check only when the signature is ready and a focus exists. Hazards are hostile
     * ADMAbilityMarkers without a bound target (bomber circles). Pings come from the game mode's board.
     */
    void BuildContext(ADMCombatGameMode& Mode, FDMAIContext& Out) const;
    /** Applies the decision: focus, attack hold, movement goal, casts, revive requests, patrol advance, threat clear, ping requests and responses. Returns the rejection reason of a failed execution, or empty. */
    FString Execute(ADMCombatGameMode& Mode, const FDMAIContext& Context, const FDMAIDecision& Decision);
    /** Emits ai.decision only when focus, chosen move action or chosen act action changed, or an execution was rejected. */
    void Trace(ADMCombatGameMode& Mode, const FDMAIContext& Context, const FDMAIDecision& Decision, const FString& Rejected);

    UPROPERTY() TObjectPtr<UDMAIProfile> Profile;
    FDMAIMemory Memory;
    FDMAIDecision LastDecision;
    FDMAIContext LastContext;
    int32 TracedFocus = INDEX_NONE;
    EDMAIAction TracedMove = EDMAIAction::None;
    EDMAIAction TracedAct = EDMAIAction::None;
    bool bTracedOnce = false;
    int32 EncounterGroup = INDEX_NONE;
    FVector HomePosition = FVector::ZeroVector;
    bool bPatrolMember = false;
    int32 PatrolSlot = 0;
    int32 PatrolWaypoint = 1;
};
