#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMRecoverySupply.generated.h"
class ADMCombatant;
UCLASS()
class DREADMERIDIAN_API ADMRecoverySupply : public AActor
{
    GENERATED_BODY()
public:
    ADMRecoverySupply();
    UPROPERTY(EditAnywhere, Replicated) bool bFood = false;
    UPROPERTY(EditAnywhere, Replicated) int32 Charges = 2;
    bool TryUse(ADMCombatant* Actor);
    void Step();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
};
