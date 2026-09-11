#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMScroungePickup.generated.h"
UCLASS()
class DREADMERIDIAN_API ADMScroungePickup : public AActor
{
    GENERATED_BODY()
public:
    ADMScroungePickup();
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
};
