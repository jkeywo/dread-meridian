#include "DMBasicAttackAbility.h"
#include "DMCombatant.h"

UDMBasicAttackAbility::UDMBasicAttackAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}
void UDMBasicAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    ADMCombatant* Actor = Cast<ADMCombatant>(ActorInfo->AvatarActor.Get());
    const bool bApplied = Actor && Actor->ResolveAttack();
    EndAbility(Handle, ActorInfo, ActivationInfo, true, !bApplied);
}
