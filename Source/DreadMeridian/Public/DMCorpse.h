#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMCorpse.generated.h"
USTRUCT()
struct FDMCorpseSnapshot
{
    GENERATED_BODY()
    UPROPERTY() int32 Version=1;
    UPROPERTY() FString SourceId;
    UPROPERTY() FString SourceKind;
    UPROPERTY() float Value=1;
    UPROPERTY() bool bConsumed=false;
    UPROPERTY() bool bCleanupEligible=false;
    UPROPERTY() FVector Location=FVector::ZeroVector;
};
UCLASS()
class DREADMERIDIAN_API ADMCorpse : public AActor
{
    GENERATED_BODY()
public:
    ADMCorpse();
    bool Initialize(const class ADMCombatant* Source);
    float Consume();
    FDMCorpseSnapshot Capture() const { return State; }
    bool Restore(const FDMCorpseSnapshot& S);
    UPROPERTY(Replicated) FDMCorpseSnapshot State;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
