#include "DMHealthAttributes.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UDMHealthAttributes::UDMHealthAttributes() { InitHealth(100); InitMaxHealth(100); InitShield(0); }
void UDMHealthAttributes::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    SetShield(FMath::Max(0.f, GetShield()));
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
