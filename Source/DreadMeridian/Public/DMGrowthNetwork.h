#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMGrowthNetwork.generated.h"
class ADMCombatant;
class ADMBossArena;
class ADMCorruption;
/** Private authority snapshot. Do not replicate genuine node membership. */
struct FDMGrowthSnapshot
{
    int32 Version = 1;
    TArray<FString> GenuineIds;
    TArray<FString> ResolvedIds;
    int32 NextSpread = 0;
    int32 SpreadWave = 0;
};
UCLASS()
class DREADMERIDIAN_API ADMGrowthNetwork : public AActor
{
    GENERATED_BODY()
public:
    ADMGrowthNetwork();
    bool Initialize(ADMBossArena* Arena,ADMCombatant* Boss,ADMCorruption* Field,uint32 Draw);
    void Step(int32 Tick);
    FDMGrowthSnapshot Capture() const { return State; }
    bool Restore(const FDMGrowthSnapshot& Snapshot);
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMCombatant>> Nodes;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY() TObjectPtr<ADMCombatant> Boss;
    UPROPERTY() TObjectPtr<ADMCorruption> Field;
    FDMGrowthSnapshot State;
    int32 LastTick = -1;
};
