#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMSmugglerComponent.generated.h"
class ADMCombatant;
class ADMCombatGameMode;
class ADMAbilityMarker;

UENUM(BlueprintType)
enum class EDMSmuggler : uint8 { None, Gunman, Bruiser, Lookout, Bomber, GangBoss };

/** Native-faction signatures. Numerical values are sandbox tuning, not locked balance. */
UCLASS()
class DREADMERIDIAN_API UDMSmugglerComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMSmugglerComponent();
    void Initialize(EDMSmuggler NewRole);
    void Step(ADMCombatGameMode& Mode);
    /** Activates the role's signature on Target if CanSignature passes. Behaviour and timings are unchanged; tests call this directly. */
    bool TrySignature(ADMCombatGameMode& Mode, ADMCombatant* Target);
    /**
     * Pure check extracted from TrySignature: authority, combat active, enemy team, not down/restrained, target valid,
     * hostile, alive, not casting, cooldown elapsed, no burning ground active, line of sight, and the per-role range
     * (Bruiser 200, Bomber 650, Lookout/GangBoss 900; Gunman and None have no signature). Reason (when given)
     * receives one of: authority, inactive, state, target, casting, cooldown, burning, no_sight, out_of_range, no_signature.
     */
    bool CanSignature(const ADMCombatGameMode& Mode, const ADMCombatant* Target, const TCHAR** Reason = nullptr) const;
    /** Per-role signature range (200/650/900/900) or 0 when the role has none. */
    float SignatureRange() const;
    /** Per-role cooldown in ticks (Bruiser 65, Bomber 90, Lookout 85, GangBoss 95) or 0. */
    int32 SignatureCooldownTicks() const;
    /** Ticks from activation to effect (Bruiser 6, Bomber 12, others 0). */
    int32 SignatureCastDelayTicks() const;
    /** Bruiser and Bomber stop moving and attacking while casting. */
    bool RootsCaster() const { return Role == EDMSmuggler::Bruiser || Role == EDMSmuggler::Bomber; }
    bool HasSignature() const { return SignatureRange() > 0; }
    /** Cooldown elapsed and no burning ground in progress. */
    bool IsSignatureReady(int32 Tick) const { return HasSignature() && Tick >= NextSignatureTick && FireUntil <= Tick; }
    /** Line of sight between two points ignoring combatant bodies (world geometry blocks). */
    bool Sight(FVector From, FVector To) const;
    void Cancel();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    FString Name() const;
    FString Status() const;
    float Range() const;
    float BaseHealth() const;
    float BaseDamage() const;
    int32 Interval() const;
    float Speed() const;
    bool IsRanged() const { return Role != EDMSmuggler::None && Role != EDMSmuggler::Bruiser; }
    bool IsCasting() const { return ResolveTick > 0; }
    float DamageMultiplier(const ADMCombatGameMode& Mode, const ADMCombatant* Target) const;
    static ADMCombatant* FocusTarget(const ADMCombatGameMode& Mode, const ADMCombatant* Recipient, bool bBossOnly);
    static ADMCombatant* DiverTarget(const ADMCombatGameMode& Mode, const ADMCombatant* Bodyguard);
    UPROPERTY(Replicated, BlueprintReadOnly) EDMSmuggler Role = EDMSmuggler::None;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bSetPosition = false;
    UPROPERTY(Replicated, BlueprintReadOnly) TObjectPtr<ADMCombatant> OrderTarget;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 OrderUntil = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ResolveTick = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 NextSignatureTick = 0;
private:
    ADMCombatant* Self() const;
    void Record(ADMCombatGameMode& Mode, const FString& Stage, ADMCombatant* Target = nullptr);
    void ClearMarker();
    UPROPERTY() TObjectPtr<ADMAbilityMarker> Marker;
    UPROPERTY() TObjectPtr<ADMCombatant> PendingTarget;
    FVector BlastPoint = FVector::ZeroVector;
    FVector PositionAnchor = FVector::ZeroVector;
    int32 StationarySince = -1, FireUntil = 0, NextFireTick = 0;
};
