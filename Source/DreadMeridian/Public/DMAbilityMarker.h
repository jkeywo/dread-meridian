#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMAbilityMarker.generated.h"
class ADMCombatant;
UCLASS()
class DREADMERIDIAN_API ADMAbilityMarker : public AActor
{
    GENERATED_BODY()
public:
    ADMAbilityMarker();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) bool bHostile = false;
    UPROPERTY(Replicated) FString CustomLabel;
    UPROPERTY(Replicated) bool bSpirit = false;
    UPROPERTY(Replicated) TObjectPtr<ADMCombatant> BoundTarget;
    UPROPERTY(Replicated) FString SpiritId;
    UPROPERTY(Replicated) float Radius = 220;
    UPROPERTY(Replicated) float Attention = 0;
    int32 ArmedTick = 0;
private:
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Orb;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Ring;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> Glow;
};
