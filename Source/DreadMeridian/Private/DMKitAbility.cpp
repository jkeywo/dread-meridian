#include "DMKitAbility.h"
#include "DMKitComponent.h"
#include "DMCombatant.h"
UDMKitAbility::UDMKitAbility()
{ InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor; NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly; NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly; }
void UDMKitAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    auto* Actor = Cast<ADMCombatant>(ActorInfo->AvatarActor.Get());
    const bool bApplied = Actor && Actor->Kit->Resolve();
    EndAbility(Handle, ActorInfo, ActivationInfo, true, !bApplied);
}
