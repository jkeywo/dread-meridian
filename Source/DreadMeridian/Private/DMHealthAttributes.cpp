#include "DMHealthAttributes.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UDMHealthAttributes::UDMHealthAttributes() { InitHealth(100); InitMaxHealth(100); InitShield(0); }
void UDMHealthAttributes::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    // Shield gains from spirits cap at half MaxHealth so a long fight cannot stack an unbounded buffer.
    SetShield(FMath::Clamp(GetShield(), 0.f, FMath::Max(GetMaxHealth() * .5f, 20.f)));
}
void UDMHealthAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(UDMHealthAttributes, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UDMHealthAttributes, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UDMHealthAttributes, Shield, COND_None, REPNOTIFY_Always);
}
void UDMHealthAttributes::OnRep_Health(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UDMHealthAttributes, Health, Old); }
void UDMHealthAttributes::OnRep_MaxHealth(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UDMHealthAttributes, MaxHealth, Old); }
void UDMHealthAttributes::OnRep_Shield(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UDMHealthAttributes, Shield, Old); }
