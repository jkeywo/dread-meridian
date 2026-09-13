#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMVision.generated.h"
class ADMCombatant;
/** Authored, known terrain/source. No new art; radius values are provisional. */
UCLASS()
class DREADMERIDIAN_API ADMVisionArea : public AActor
{
    GENERATED_BODY()
public:
    ADMVisionArea();
    UPROPERTY(EditAnywhere,Replicated) bool bReeds=true;
    UPROPERTY(EditAnywhere,Replicated) bool bEnabled=true;
    UPROPERTY(EditAnywhere,Replicated) float Radius=500;
    UPROPERTY(Replicated) FString SourceId;
    bool Contains(FVector Point) const;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
namespace DMVision
{
    DREADMERIDIAN_API bool InReeds(UWorld* World,FVector Point);
    DREADMERIDIAN_API bool CanSee(const ADMCombatant* Observer,const ADMCombatant* Target);
    DREADMERIDIAN_API bool Relevant(const ADMCombatant* Target,const AActor* Viewer);
    DREADMERIDIAN_API void Lighthouse(UWorld* World,const FString& Id,FVector Point,bool bEnabled=true);
}
