#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "DMPrimaryComponent.generated.h"
class ADMCombatant;
class ADMAbilityMarker;
class ADMScroungePickup;
UCLASS()
class DREADMERIDIAN_API UDMPrimaryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMPrimaryComponent();
    void Initialize();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    bool Request(ADMCombatant* Target, FVector Point, bool bDetonate = false);
    bool Resolve();
    FString Validate(ADMCombatant* Target, FVector Point, bool bDetonate = false) const;
    void Step(int32 Tick);
    void CancelChannel();
    void ReleaseClinch();
    FString Name() const;
    FString Status() const;
    FString ReplicationSummary() const;
    float Range() const;
    // ---- Bot-facing helpers (the bot policy itself lives in DMUtilityAI / ADMSquadController)
    /** Nearest scrounge pickup within MaxDistance (2D), or nullptr. */
    ADMScroungePickup* NearestPickup(float MaxDistance) const;
    /** Ticks between Q activation and effect for this kind: Sapper 5 (arming), Photographer 30 (channel), others 0. */
    int32 CastDelayTicks() const;
    /** Cooldown length in ticks after a cast for this kind: Sapper 8, Photographer 20, Medium 25, Smuggler 40. */
    int32 CooldownTicks() const;
    /** Ticks until the next cast is allowed (0 when ready). */
    int32 CooldownRemaining(int32 Tick) const { return FMath::Max(0, NextCastTick - Tick); }
    /** Mirrors Validate's state gate: not held/framing, cooldown elapsed. Does not check range, sight or stock. */
    bool IsReady(int32 Tick) const;
    static constexpr float SatchelRadius = 220;
    static constexpr float SpiritRadius = 180;
    /** Valid arena ground under Point (with clearance above it); shared with the kit abilities. */
    bool Ground(FVector Point, FVector& Out) const;
    /** Line of sight from self to Point ignoring combatant bodies. */
    bool Sight(FVector Point, ADMCombatant* Target = nullptr) const;
    /** Destroys the satchels with these serials without detonating them: Dead Ground already resolved their blast. */
    void ConsumeSatchels(const TSet<int32>& Serials);
    UPROPERTY(Replicated) float Cooldown = 0;
    UPROPERTY(Replicated) TObjectPtr<ADMCombatant> FrameTarget;
    UPROPERTY(Replicated) TObjectPtr<ADMCombatant> HeldTarget;
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMAbilityMarker>> Satchels;
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMAbilityMarker>> Bindings;
    FString LastFailure;
    void DevelopLinked(ADMCombatant* Subject, float Exposure);
    float EvolvedSatchelRadius() const;
    void ApplySatchel(ADMCombatant* Enemy, FVector Center);
    bool TriggerNearbySatchel(FVector Point, float Distance);

private:
    TMap<FString, int32> SpiritPulseUntil;
    TMap<FString, int32> StudiedTells;
    TArray<FString> PortraitSubjects;
    int32 PortraitUntil = 0;
    bool SatchelCanHit(const ADMAbilityMarker* Charge, ADMCombatant* Enemy) const;
    bool DetonateSatchel(int32 Index);
    ADMCombatant* Self() const;
    void Emit(const FString& Action, ADMCombatant* Target = nullptr);
    int32 Now() const;
    int32 NextCastTick = 0;
    int32 FrameEndTick = 0;
    int32 HoldEndTick = 0;
    int32 SpiritSerial = 0;
    int32 ChargeSerial = 0;
    FGameplayAbilitySpecHandle AbilityHandle;
    TWeakObjectPtr<ADMCombatant> RequestedTarget;
    FVector RequestedPoint = FVector::ZeroVector;
    bool bRequestedDetonate = false;
    bool bResolved = false;
};
