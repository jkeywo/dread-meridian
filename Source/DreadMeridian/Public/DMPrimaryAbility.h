#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "DMPrimaryAbility.generated.h"
UCLASS()
class DREADMERIDIAN_API UDMPrimaryAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UDMPrimaryAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
