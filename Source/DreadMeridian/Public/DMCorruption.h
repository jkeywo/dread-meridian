#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMCorruption.generated.h"
class ADMBossArena;
USTRUCT()
struct FDMCorruptionSnapshot
{
    GENERATED_BODY()
    UPROPERTY() int32 Version = 1;
    UPROPERTY() TArray<FVector> Cells;
};
UCLASS()
class DREADMERIDIAN_API ADMCorruption : public AActor
{
    GENERATED_BODY()
public:
    ADMCorruption();
    bool Initialize(ADMBossArena* InArena);
    bool Spread(FVector Location);
    bool Contains(FVector Location) const;
    void Step(int32 Tick);
    FDMCorruptionSnapshot Capture() const { return State; }
    bool Restore(const FDMCorruptionSnapshot& Snapshot);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(ReplicatedUsing=Rebuild) FDMCorruptionSnapshot State;
    UPROPERTY() TObjectPtr<ADMBossArena> Arena;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Ground;
    UFUNCTION() void Rebuild();
    int32 LastTick = -1;
};
