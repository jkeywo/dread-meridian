#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMCombatGameMode.h"
#include "DMFishingVillage.generated.h"
class ADMObjective;
class ADMShubEncounter;
class UStaticMeshComponent;

UCLASS()
class DREADMERIDIAN_API ADMFishingVillageGameMode : public ADMCombatGameMode
{
    GENERATED_BODY()
public:
    ADMFishingVillageGameMode();
};

/** Fixed, provisional scenario composition. No HTN or timed Ritual escalation. */
UCLASS()
class DREADMERIDIAN_API ADMFishingVillage : public AActor
{
    GENERATED_BODY()
public:
    ADMFishingVillage();
    void StartScenario();
    void StepScenario();
    bool DriveBot(ADMCombatant* Hero);
    FVector Waypoint(FVector From,FVector Goal) const;
    bool RouteClear(FVector From,FVector To) const;
    ADMObjective* NextObjective() const;
    void Drain();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) int32 CoreStage=0;
    UPROPERTY(ReplicatedUsing=OnRep_Drained) bool bDrained=false;
    UPROPERTY(Replicated) bool bManifested=false;
    UPROPERTY(Replicated) TObjectPtr<ADMObjective> Core;
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMObjective>> Disruptions;
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMObjective>> Optional;
    UPROPERTY() TObjectPtr<ADMShubEncounter> Encounter;
    static FVector Basin() { return FVector(1550,600,0); }
private:
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Geometry;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Flood;
    TArray<FBox> Obstacles;
    bool bStarted=false;
    ADMObjective* SpawnObjective(const FString& Id,FVector Location);
    void SpawnCore();
    UFUNCTION() void OnRep_Drained();
};
