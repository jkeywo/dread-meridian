#pragma once
#include "CoreMinimal.h"
#include "DMGameMode.h"
#include "DMSmugglerWave.h"
#include "DMPing.h"
#include "DMSquadBoard.h"
#include "DMSmugglerComponent.h"
#include "DMInvestigatorComponent.h"
#include "DMKitComponent.h"
#include "DMCombatGameMode.generated.h"

class ADMCombatant;
class UDMAIProfile;

/** Aggregate outcome metrics for headless tuning. Emitted as one DREAD_AI_RESULT log line at completion or soak timeout. */
struct DREADMERIDIAN_API FDMCombatMetrics
{
    float DamageToInvestigators = 0;
    float DamageToEnemies = 0;
    int32 InvestigatorDowns = 0;
    int32 EnemyKills = 0;
    int32 Revives = 0;
    int32 SignatureActivations = 0;
    int32 QCasts = 0;
    int32 WCasts = 0;
    int32 ECasts = 0;
    int32 RCasts = 0;
    int32 PingsCreated = 0;
    /** EntityId -> damage dealt, for per-role insight. */
    TMap<FString, float> DamageDealtBy;
    /** "<EntityId>.<w|e|r>" -> casts. The totals above cannot show that every kit fired, only that some did. */
    TMap<FString, int32> KitCastsBy;

    // Tactical-quality counters. The counters above say who won; these say whether the squad played well,
    // which damage and downs alone cannot distinguish from luck. All are observations, never inputs to play.
    /** Damage dealt past what was needed to kill: the cost of two bots committing to the same dying target. */
    float OverkillDamage = 0;
    /**
     * Focus quality, as a pair to be read as a ratio. Each tick adds the number of distinct enemies held by
     * living companions to the first, and the number of companions holding any target to the second. A ratio
     * near 1 is the whole squad on one enemy; near 4 is four bots on four enemies. "Do two bots share a target"
     * was tried first and is useless here: nearest-target selection already saturates it above 90%.
     */
    int32 FocusDistinctTicks = 0;
    int32 FocusHolderTicks = 0;
    /** Companion-ticks spent standing inside a hostile hazard circle. */
    int32 HazardTicks = 0;
    /** Companion-ticks spent attacking an enemy that is itself attacking a companion below PerilHealth. */
    int32 PeelTicks = 0;
    /**
     * The failure this is really about: ticks where a companion below PerilHealth is being attacked and no
     * other living companion is attacking their attacker. PeelTicks alone rises simply because everyone is
     * hurt, so it cannot tell good peeling from a losing fight; this one names the unanswered case.
     */
    int32 UnansweredPerilTicks = 0;
    /** Enemy deaths inside an investigator's armed satchel, wire or suppression zone: prepared ground paying off. */
    int32 TrapGroundKills = 0;
    /** Investigator downs with no living companion within IsolationRadius. */
    int32 IsolatedDowns = 0;
};

UCLASS()
class DREADMERIDIAN_API ADMCombatGameMode : public ADMGameMode
{
    GENERATED_BODY()
public:
    ADMCombatGameMode();
    virtual void StartPlay() override;
    virtual void PostLogin(APlayerController* Player) override;
    virtual void Logout(AController* Exiting) override;
    virtual void RestartPlayer(AController* Player) override;
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    int32 GetCombatTick() const { return CombatTick; }
    EDMSmugglerWave GetEncounterStage() const { return SmugglerWave.Stage; }
    bool IsCombatActive() const { return bCombatActive; }
    void UseBossOutcome() { if (HasAuthority()) { bBossOutcome=true; } }
    void CompleteBossEncounter(bool bVictory) { if (HasAuthority() && bBossOutcome && bCombatActive) { CompleteCombat(bVictory); } }
    bool UsesEncounterLayout() const { return SmokeOutcome.IsEmpty() && !bNetworkTest && !bSwampTest; }
    const TArray<TObjectPtr<ADMCombatant>>& GetCombatants() const { return Combatants; }
    ADMCombatant* FindCombatant(const FString& EntityId) const;
    /** Call between combat iterations, never while iterating the roster. */
    ADMCombatant* SpawnEncounterActor(const FString& Id, FVector Location, float HP, float Damage, bool bStatic = false);
    void Emit(const FString& Type, const TSharedRef<FJsonObject>& Data);
    void RequestRevive(ADMCombatant* Actor, ADMCombatant* Ally);
    void ReleaseInvestigator(AController* Player);
    /** Grievous stacks lengthen the channel on a nonlinear curve. Sandbox tuning, not locked balance. */
    static int32 ReviveDurationTicks(int32 GrievousCount)
    { const int64 G = FMath::Clamp<int64>(GrievousCount, 0, 1000000); return static_cast<int32>(FMath::Min<int64>(20 + 5 * G * G, MAX_int32 / 4)); }

    // ---- Pings (server-authoritative board, projected to ADMGameState::Pings after every change)
    /** Validates the request against FDMPingBoard rules plus world rules (target must exist and be alive for Enemy/Focus/Ignore; Help targets an investigator; location inside the playable extent). Returns the id or INDEX_NONE. Emits ping.created. */
    int32 CreatePing(EDMPingKind Kind, const FString& AuthorId, bool bAuthorBot, FVector Location, const FString& TargetId);
    bool CancelPing(int32 Id, const FString& AuthorId);
    bool AcknowledgePing(int32 Id, const FString& WhoId);
    /** Records a bot response and emits ping.responded ("on_it" or "busy"). */
    bool RespondToPing(int32 Id, const FString& ResponderId, bool bOnIt);
    const FDMPingBoard& GetPingBoard() const { return PingBoard; }

    // ---- Squad intent
    /** Companions publish what they are committing to here; teammates read it on their next Think. */
    FDMSquadBoard& GetSquadBoard() { return SquadBoard; }
    const FDMSquadBoard& GetSquadBoard() const { return SquadBoard; }

    // ---- AI profiles
    /** Resolves (and caches per profile name) the UDMAIProfile for an actor's role/kind, applying -DMAIWeights overrides. */
    UDMAIProfile* ProfileFor(const ADMCombatant& Actor);

    // ---- Tuning metrics (called by ADMCombatant on the authority)
    /** PoolBefore is the target's Health + Shield before this hit, so the overkill past a kill can be counted. */
    void NoteDamage(const ADMCombatant& From, const ADMCombatant& To, float Damage, float PoolBefore);
    void NoteDowned(const ADMCombatant& Target);
    void NoteRevive();
    void NoteSignature();
    void NoteQCast();
    void NoteKitCast(EDMKitSlot Slot, const ADMCombatant& Caster);
    const FDMCombatMetrics& GetMetrics() const { return Metrics; }
protected:
    virtual void ConfigureCaptureMetadata(const TSharedRef<FJsonObject>& Metadata) override;
    /**
     * False defers the encounter so a shell (menu/lobby) can start it later. The sandbox map starts
     * immediately; ADMShellGameMode overrides this and calls BeginEncounter() on Launch Expedition.
     */
    virtual bool ShouldBeginEncounterOnStartPlay() const { return true; }
    /** Spawns the roster, publishes the encounter and starts the combat timer. Safe to call once. */
    void BeginEncounter();
    /** Fires on the authority after the run is finished and combatants are stopped. */
    virtual void OnEncounterComplete(bool bVictory) {}
private:
    UPROPERTY() TArray<TObjectPtr<ADMCombatant>> Combatants;
    UPROPERTY() TMap<FString, TObjectPtr<UDMAIProfile>> Profiles;
    TMap<ADMCombatant*, TPair<TWeakObjectPtr<ADMCombatant>, int32>> Revives;
    FTimerHandle CombatTimer;
    int32 CombatTick = 0;
    bool bCombatActive = false;
    bool bBossOutcome = false;
    FString SmokeOutcome;
    FString AIWeightsPath;
    bool bNetworkTest = false;
    bool bSwampTest = false;
    int32 RevivesCompleted = 0;
    bool bGuardChecksPassed = true;
    FDMSmugglerWave SmugglerWave;
    FDMPingBoard PingBoard;
    FDMSquadBoard SquadBoard;
    FDMCombatMetrics Metrics;
    void SpawnBossPosse();
    void PublishEncounter();
    void StepCombat();
    /** Expires/fulfils pings (dead targets, revived allies, collected pickups), emits ping.ended, republishes. Runs before the Think loop. */
    void StepPings();
    /**
     * Samples the per-tick tactical counters (focus overlap, hazard standing, peeling) after the Think loop,
     * so they describe the decisions the squad just committed to. Observation only: it changes no gameplay state.
     */
    void StepMetrics();
    void PublishPings();
    void EmitPingEnded(const TArray<FDMPingEnded>& Ended);
    /** One DREAD_AI_RESULT line with FDMCombatMetrics plus outcome/tick; Outcome is victory, defeat or timeout. */
    void LogResult(const FString& Outcome);
    void AssignInvestigator(APlayerController* Player);
    void AttachBot(ADMCombatant* Actor);
    void CompleteCombat(bool bVictory);
};
