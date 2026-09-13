#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMProgressionComponent.generated.h"

// Provisional run cadence; XP is cumulative, never a currency.
USTRUCT()
struct FDMProgressionState
{
    GENERATED_BODY()
    UPROPERTY() int32 XP = 0;
    UPROPERTY() int32 Spent = 0;
    static constexpr int32 Cap = 1200;
    int32 Level() const { return 1 + XP / 100; }
    int32 Opportunities() const { return FMath::Min(6, Level() / 2) - Spent; }
    float BasicMultiplier() const { return 1.f + .015f * (Level() - 1); }
    bool Add(int32 Amount)
    {
        if (Amount <= 0 || XP >= Cap) { return false; }
        XP += FMath::Min(Amount, Cap - XP); return true;
    }
};

UCLASS()
class DREADMERIDIAN_API UDMProgressionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMProgressionComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    /** Authority-only accepted event identifier prevents repeated rewards, including across possession. */
    bool Award(const FString& EventId, int32 Amount);
    const FDMProgressionState& Get() const { return State; }
    FString Summary() const;
private:
    UPROPERTY(Replicated) FDMProgressionState State;
    TSet<FString> RewardedEvents;
};
