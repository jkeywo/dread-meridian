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

/** Fixed, provisional scenario composition with authoritative Ritual pressure. No HTN. */
UCLASS()
class DREADMERIDIAN_API ADMFishingVillage : public AActor
{
    GENERATED_BODY()
public:
    ADMFishingVillage();
    void StartScenario();
    void StepScenario();
    void StepRitual(int32 Tick);
    bool MandatoryComplete() const;
    // Provisional: one point per 3 seconds, 5 minutes per default stage.
    static constexpr int32 RitualTicksPerPoint = 30;
    bool DriveBot(ADMCombatant* Hero);
    FVector Waypoint(FVector From,FVector Goal) const;
    bool RouteClear(FVector From,FVector To) const;
    /**
     * A click-to-move destination is only ever routed around obstacles in flight (see Waypoint); it was never
     * validated at the point of click, so a click landing on non-navigable ground (deep water, inside a
     * building) set a MoveGoal the pawn could never actually reach and just idled trying to close on it. This
     * walks the straight line from From (the player) back from To (the raw click point) and returns the first
     * point that clears every obstacle, i.e. the nearest navigable spot on that line to the click.
     */
    FVector ClampToNavigable(FVector From,FVector To) const;
    ADMObjective* NextObjective() const;
    void Drain();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(Replicated) int32 CoreStage=0;
    UPROPERTY(ReplicatedUsing=OnRep_Drained) bool bDrained=false;
    UPROPERTY(Replicated) bool bManifested=false;
    UPROPERTY(Replicated) TObjectPtr<ADMObjective> Core;
    UPROPERTY(Replicated) TObjectPtr<ADMObjective> Manifestation;
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMObjective>> Disruptions;
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMObjective>> Optional;
    UPROPERTY() TObjectPtr<ADMShubEncounter> Encounter;
    static FVector Basin() { return FVector(1550,600,0); }
private:
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Geometry;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Flood;
    TArray<FBox> Obstacles;
    bool bStarted=false;
    int32 LastRitualTick=0;
    int32 AppliedRitualStage=0;
    void Manifest(bool bDeliberate);
    ADMObjective* SpawnObjective(const FString& Id,FVector Location);
    void SpawnCore();
    UFUNCTION() void OnRep_Drained();
};
