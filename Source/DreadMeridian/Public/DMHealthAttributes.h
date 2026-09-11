#pragma once
#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "DMHealthAttributes.generated.h"

#define DM_ATTRIBUTE(Name) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UDMHealthAttributes, Name) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Name) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(Name) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(Name)

UCLASS()
class DREADMERIDIAN_API UDMHealthAttributes : public UAttributeSet
{
    GENERATED_BODY()
public:
    UDMHealthAttributes();
    UPROPERTY(ReplicatedUsing=OnRep_Health) FGameplayAttributeData Health;
    UPROPERTY(ReplicatedUsing=OnRep_MaxHealth) FGameplayAttributeData MaxHealth;
    UPROPERTY(ReplicatedUsing=OnRep_Shield) FGameplayAttributeData Shield;
    DM_ATTRIBUTE(Health)
    DM_ATTRIBUTE(MaxHealth)
    DM_ATTRIBUTE(Shield)
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    UFUNCTION() void OnRep_Health(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& Old);
    UFUNCTION() void OnRep_Shield(const FGameplayAttributeData& Old);
};
