#include "DMPrimaryAbility.h"
#include "DMPrimaryComponent.h"
#include "DMCombatant.h"
UDMPrimaryAbility::UDMPrimaryAbility()
{ InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor; NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly; NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly; }
void UDMPrimaryAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    auto* Actor = Cast<ADMCombatant>(ActorInfo->AvatarActor.Get());
    const bool bApplied = Actor && Actor->Primary->Resolve();
    EndAbility(Handle, ActorInfo, ActivationInfo, true, !bApplied);
}
