#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "DMKitAbility.generated.h"

/** Server-only GAS shim for the W/E/R slots: activation resolves the kit component's pending request, like UDMPrimaryAbility for Q. */
UCLASS()
class DREADMERIDIAN_API UDMKitAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UDMKitAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
