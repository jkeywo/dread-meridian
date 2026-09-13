#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMInjuryRules.h"
#include "DMInjuryComponent.generated.h"
class ADMCombatant;

UCLASS(Config=Game, ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class DREADMERIDIAN_API UDMInjuryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMInjuryComponent();
    UPROPERTY(Config, EditAnywhere) FDMInjurySettings Settings;
    UPROPERTY(Replicated, BlueprintReadOnly) TArray<EDMInjury> Specific;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 CastUntilTick = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 AttackUntilTick = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) float MovementFactor = 1;
    UPROPERTY(Replicated, BlueprintReadOnly) float HealingFactor = 1;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 HealingPulses = 0;
    void StartFoodHealing();
    void Reset();
    void Step(int32 Tick);
    void RecordLoss(float Loss, bool bDown, bool bHazard, ADMCombatant* Source, const FString& AbilityId);
    float Incoming(float Unshielded, bool bHazard) const;
    void OnCast(); void OnAttack(); void OnDisplacement(float Distance);
    bool Treat();
    bool ApplyMorphine();
    bool CanCast() const; bool CanAttack() const;
    FString Summary() const;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    FDMInjuryState State;
    FVector LastPosition = FVector::ZeroVector;
    bool bHasPosition = false;
    int32 NextHealingTick = 0;
    ADMCombatant* Self() const;
    int32 Now() const;
    void Project();
};
