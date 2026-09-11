#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMAttackFX.generated.h"

// Short-lived local particle burst, spawned by accepted server attack multicast.
UCLASS()
class DREADMERIDIAN_API ADMAttackFX : public AActor
{
    GENERATED_BODY()
public:
    ADMAttackFX();
    void Initialize(FVector From, FVector To, FLinearColor Color, uint8 Style);
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Particles;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> Material;
    FVector Start, End;
    float Age = 0;
    uint8 EffectStyle = 0;
};
