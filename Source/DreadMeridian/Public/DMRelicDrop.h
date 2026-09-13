#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMRelicComponent.h"
#include "DMRelicDrop.generated.h"
UENUM()
enum class EDMRelicChoice : uint8 { Pending, Pass, Greed, Need };
USTRUCT()
struct FDMRelicVote
{
    GENERATED_BODY()
    UPROPERTY() FString EntityId;
    UPROPERTY() EDMRelicChoice Choice = EDMRelicChoice::Pending;
};
USTRUCT()
struct FDMRelicRollSnapshot
{
    GENERATED_BODY()
    UPROPERTY() int32 Version = 1;
    UPROPERTY() FString AwardId;
    UPROPERTY() EDMRelic Relic = EDMRelic::Swagger;
    UPROPERTY() TArray<FDMRelicVote> Votes;
    UPROPERTY() int32 Deadline = 0;
    UPROPERTY() bool bResolved = false;
    UPROPERTY() FString Winner;
};
UCLASS()
class DREADMERIDIAN_API ADMRelicDrop : public AActor
{
    GENERATED_BODY()
public:
    ADMRelicDrop();
    bool Initialize(const FString& AwardId);
    bool Vote(class ADMCombatant* Actor, EDMRelicChoice Choice);
    void Step(int32 Tick);
    FDMRelicRollSnapshot Capture() const { return Roll; }
    bool Restore(const FDMRelicRollSnapshot& Snapshot);
    UPROPERTY(Replicated) FDMRelicRollSnapshot Roll;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    void Resolve();
};
