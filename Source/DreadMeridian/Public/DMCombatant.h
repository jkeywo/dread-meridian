#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "AttributeSet.h"
#include "DMInvestigatorComponent.h"
#include "DMPrimaryComponent.h"
#include "DMSmugglerComponent.h"
#include "DMCombatant.generated.h"

class UDMHealthAttributes;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class DREADMERIDIAN_API ADMCombatant : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()
public:
    ADMCombatant();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UDMSmugglerComponent> Smuggler;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UDMCombatPresentation> Presentation;
    UFUNCTION(NetMulticast, Unreliable) void MulticastPresentation(uint8 Event, FVector Target);
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    void InitializeCombatant(const FString& Id, bool bEnemy, float MaxHP, float Damage, float InitialShield = 0);
    void InitializeInvestigator(EDMInvestigator Kind, bool bUseProfileTuning);
    FString DisplayName() const;
    float GetAttackRange() const;
    void StepInvestigator(int32 Tick);
    void RecordResources(const FString& Reason);
    void ApplySlow(float Fraction, int32 UntilTick);
    void ApplyDisplacement(FVector Delta);
    UFUNCTION(NetMulticast, Unreliable) void MulticastAttackFX(FVector From, FVector To, FLinearColor Color, uint8 Style);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UDMInvestigatorComponent> Investigator;
    UPROPERTY(Replicated) bool bHumanEnemy = true;
    UPROPERTY(Replicated) bool bSuppressed = false;
    UPROPERTY(Replicated) bool bCommonEnemy = true;
    float Health() const;
    float MaxHealth() const;
    float Shield() const;
    bool IsDown() const { return Health() <= 0; }
    bool TryAttack(ADMCombatant* Target);
    bool ResolveAttack();
    bool DealCombatDamage(ADMCombatant* Target, float Damage, const FString& AbilityId, bool bBasic = false);
    bool IsRestrained() const { return IsValid(HeldBy); }
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UDMPrimaryComponent> Primary;
    UPROPERTY(Replicated) TObjectPtr<ADMCombatant> HeldBy;
    UPROPERTY(Replicated) bool bBreakVulnerable = false;
    UPROPERTY(Replicated) bool bTelegraphActive = false;
    UPROPERTY(Replicated) float SpiritProtection = 0;
    UPROPERTY(Replicated) float SpiritSlow = 0;
    int32 TelegraphEndTick = 0;
    bool Revive(ADMCombatant* Ally);
    /** Public revive projection for the HUD. Authority only; progress is 0..1. */
    void SetReviveChannel(const FString& Reviver, float Progress);
    void MoveToward(const FVector& Location);
    void StopGoal();
    bool HasMoveGoal() const { return bHasMoveGoal; }
    FVector GetMoveGoal() const { return MoveGoal; }
    void SetAttackTarget(ADMCombatant* Target);
    ADMCombatant* GetAttackTarget() const { return AttackTarget.Get(); }
    /**
     * Attack-channel gate driven by the bot brain. While held, TryAttack/ResolveAttack refuse and the
     * telegraph is cleared, but the attack target stays set (it also feeds Gunman set-position, Exposure decay
     * and the HUD aggro cue). Humans are never held: AttachBot/AssignInvestigator/ReleaseInvestigator clear it.
     */
    void SetAttackHold(bool bHold);
    bool IsAttackHeld() const { return bAttackHold; }

    UPROPERTY(Replicated, BlueprintReadOnly) FString EntityId;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bIsEnemy = false;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 InjuryCount = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 GrievousCount = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) FString ControlKind = TEXT("bot");
    // Public combat projections. Aggro, cooldown and revive state are player-facing, not hidden state.
    UPROPERTY(Replicated, BlueprintReadOnly) FString AttackTargetId;
    UPROPERTY(Replicated, BlueprintReadOnly) FString ReviverId;
    UPROPERTY(Replicated, BlueprintReadOnly) float ReviveProgress = 0;
    // Sandbox tuning. These are not investigator kits or locked balance values.
    float AttackDamage = 12;
    bool bProfileRange = false;
    static constexpr int32 AttackCooldownTicks = 10;
    UPROPERTY(Replicated) int32 AttackIntervalTicks = AttackCooldownTicks;
    TMap<FString, float> Threat;
    UPROPERTY(Replicated) int32 NextAttackTick = 0;
    int32 LastDamageTick = -1;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAbilitySystemComponent> AbilitySystem;
    UPROPERTY() TObjectPtr<UDMHealthAttributes> Attributes;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<class UAnimSequence> IdleAnimation;
    UPROPERTY() TObjectPtr<class UAnimSequence> RunAnimation;
    bool bWasRunning = false;
    bool bAttackHold = false;
    UPROPERTY(Replicated) float ReplicatedMoveSpeed = 420;
    FString LastResourceSnapshot;
    int32 SlowUntilTick = 0;
    float SlowFraction = 0;
    FGameplayAbilitySpecHandle BasicAttackHandle;
    TWeakObjectPtr<ADMCombatant> AttackTarget;
    TWeakObjectPtr<ADMCombatant> TelegraphTarget;
    FVector MoveGoal = FVector::ZeroVector;
    bool bHasMoveGoal = false;
    void ApplyAttributeDelta(const FGameplayAttribute& Attribute, float Delta);
};
