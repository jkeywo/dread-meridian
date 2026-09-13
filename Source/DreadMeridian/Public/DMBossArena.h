#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMElderOne.h"
#include "DMBossArena.generated.h"
USTRUCT()
struct FDMBossAffordance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName Kind;
    UPROPERTY(EditAnywhere) FVector Location = FVector::ZeroVector;
    UPROPERTY(EditAnywhere) float Radius = 150;
};
UCLASS()
class DREADMERIDIAN_API ADMBossArena : public AActor
{
    GENERATED_BODY()
public:
    ADMBossArena();
    UPROPERTY(EditAnywhere,Replicated) TArray<FDMBossAffordance> Sites;
    TArray<FVector> Locations(FName Kind) const;
    bool Contains(FName Kind,FVector Point) const;
    bool Validate(EDMElderOne Boss,FString& Error) const;
    void BuildFixture(FVector Origin);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
