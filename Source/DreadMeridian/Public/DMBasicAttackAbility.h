#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "DMBasicAttackAbility.generated.h"

UCLASS()
class DREADMERIDIAN_API UDMBasicAttackAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UDMBasicAttackAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
