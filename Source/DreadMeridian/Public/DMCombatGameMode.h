#pragma once
#include "CoreMinimal.h"
#include "DMGameMode.h"
#include "DMSmugglerWave.h"
#include "DMPing.h"
#include "DMSmugglerComponent.h"
#include "DMInvestigatorComponent.h"
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
    int32 PingsCreated = 0;
    /** EntityId -> damage dealt, for per-role insight. */
    TMap<FString, float> DamageDealtBy;
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
    bool UsesEncounterLayout() const { return SmokeOutcome.IsEmpty() && !bNetworkTest; }
    const TArray<TObjectPtr<ADMCombatant>>& GetCombatants() const { return Combatants; }
    ADMCombatant* FindCombatant(const FString& EntityId) const;
    void Emit(const FString& Type, const TSharedRef<FJsonObject>& Data);
    void RequestRevive(ADMCombatant* Actor, ADMCombatant* Ally);
    void ReleaseInvestigator(AController* Player);
    /** Grievous stacks lengthen the channel on a nonlinear curve. Sandbox tuning, not locked balance. */
    static int32 ReviveDurationTicks(int32 GrievousCount) { return 20 + 5 * GrievousCount * GrievousCount; }

    // ---- Pings (server-authoritative board, projected to ADMGameState::Pings after every change)
    /** Validates the request against FDMPingBoard rules plus world rules (target must exist and be alive for Enemy/Focus/Ignore; Help targets an investigator; location inside the playable extent). Returns the id or INDEX_NONE. Emits ping.created. */
    int32 CreatePing(EDMPingKind Kind, const FString& AuthorId, bool bAuthorBot, FVector Location, const FString& TargetId);
    bool CancelPing(int32 Id, const FString& AuthorId);
    bool AcknowledgePing(int32 Id, const FString& WhoId);
    /** Records a bot response and emits ping.responded ("on_it" or "busy"). */
    bool RespondToPing(int32 Id, const FString& ResponderId, bool bOnIt);
    const FDMPingBoard& GetPingBoard() const { return PingBoard; }

    // ---- AI profiles
    /** Resolves (and caches per profile name) the UDMAIProfile for an actor's role/kind, applying -DMAIWeights overrides. */
    UDMAIProfile* ProfileFor(const ADMCombatant& Actor);

    // ---- Tuning metrics (called by ADMCombatant on the authority)
    void NoteDamage(const ADMCombatant& From, const ADMCombatant& To, float Damage);
    void NoteDowned(const ADMCombatant& Target);
    void NoteRevive();
    void NoteSignature();
    void NoteQCast();
    const FDMCombatMetrics& GetMetrics() const { return Metrics; }
protected:
    virtual void ConfigureCaptureMetadata(const TSharedRef<FJsonObject>& Metadata) override;
private:
    UPROPERTY() TArray<TObjectPtr<ADMCombatant>> Combatants;
    UPROPERTY() TMap<FString, TObjectPtr<UDMAIProfile>> Profiles;
    TMap<ADMCombatant*, TPair<TWeakObjectPtr<ADMCombatant>, int32>> Revives;
    FTimerHandle CombatTimer;
    int32 CombatTick = 0;
    bool bCombatActive = false;
    FString SmokeOutcome;
    FString AIWeightsPath;
    bool bNetworkTest = false;
    int32 RevivesCompleted = 0;
    bool bGuardChecksPassed = true;
    FDMSmugglerWave SmugglerWave;
    FDMPingBoard PingBoard;
    FDMCombatMetrics Metrics;
    void SpawnBossPosse();
    void PublishEncounter();
    void StepCombat();
    /** Expires/fulfils pings (dead targets, revived allies, collected pickups), emits ping.ended, republishes. Runs before the Think loop. */
    void StepPings();
    void PublishPings();
    void EmitPingEnded(const TArray<FDMPingEnded>& Ended);
    /** One DREAD_AI_RESULT line with FDMCombatMetrics plus outcome/tick; Outcome is victory, defeat or timeout. */
    void LogResult(const FString& Outcome);
    void AssignInvestigator(APlayerController* Player);
    void AttachBot(ADMCombatant* Actor);
    void CompleteCombat(bool bVictory);
};
