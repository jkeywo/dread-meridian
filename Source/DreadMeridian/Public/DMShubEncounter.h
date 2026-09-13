#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMShubEncounter.generated.h"
class ADMCombatant;
class ADMBossArena;
class ADMCorruption;
class ADMGrowthNetwork;
UENUM()
enum class EDMShubMove : uint8 { Idle, TramplingAdvance, BlackMilk, CallTheBrood, HornedSweep };
USTRUCT()
struct FDMShubEncounterSnapshot
{
    GENERATED_BODY()
    UPROPERTY() int32 Version=1;
    UPROPERTY() EDMShubMove Move=EDMShubMove::Idle;
    UPROPERTY() int32 Until=0;
    UPROPERTY() int32 NextMove=0;
    UPROPERTY() int32 SpawnSerial=0;
    UPROPERTY() bool bCharging=false;
    UPROPERTY() bool bFinished=false;
    UPROPERTY() FVector Destination=FVector::ZeroVector;
    UPROPERTY() TArray<FVector> Targets;
    UPROPERTY() TArray<FString> HitIds;
};
UCLASS()
class DREADMERIDIAN_API ADMShubEncounter : public AActor
{
    GENERATED_BODY()
public:
    ADMShubEncounter();
    bool Begin(ADMBossArena* Arena);
    void Step(int32 Tick);
    FDMShubEncounterSnapshot Capture() const { return State; }
    bool Restore(const FDMShubEncounterSnapshot& S);
    UPROPERTY(Replicated) TObjectPtr<ADMCombatant> Boss;
    UPROPERTY(Replicated) FDMShubEncounterSnapshot State;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY() TObjectPtr<ADMBossArena> Arena;
    UPROPERTY() TObjectPtr<ADMCorruption> Field;
    UPROPERTY() TObjectPtr<ADMGrowthNetwork> Growths;
    UPROPERTY() TArray<TObjectPtr<class ADMAbilityMarker>> Markers;
    void Windup(int32 Tick);
    void Execute(int32 Tick);
    void ClearMarkers();
    int32 LastTick=-1;
};
