#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMInvestigatorComponent.h"
#include "DMProgressionComponent.generated.h"

// Provisional run cadence; XP is cumulative, never a currency.
USTRUCT()
struct FDMProgressionState
{
    GENERATED_BODY()
    UPROPERTY() int32 XP = 0;
    UPROPERTY() int32 Spent = 0;
    UPROPERTY() uint8 Q = 0;
    UPROPERTY() uint8 W = 0;
    UPROPERTY() uint8 E = 0;
    uint8 Node(uint8 Slot) const { return Slot == 0 ? Q : Slot == 1 ? W : Slot == 2 ? E : 255; }
    static float FloorCost(uint8 To) { return To == 1 || To == 2 ? 5.f : To >= 3 && To <= 5 ? 10.f : 0.f; }
    static bool Edge(uint8 From, uint8 To)
    { return (From == 0 && (To == 1 || To == 2)) || (From == 1 && (To == 3 || To == 4)) || (From == 2 && (To == 4 || To == 5)); }
    bool Choose(uint8 Slot, uint8 To)
    {
        if (Opportunities() <= 0 || Slot > 2 || !Edge(Node(Slot), To)) { return false; }
        (Slot == 0 ? Q : Slot == 1 ? W : E) = To; ++Spent; return true;
    }
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

struct DREADMERIDIAN_API FDMEvolutionNode
{
    FString Id, Name, Description;
    float Offense = 0, Control = 0, Protection = 0;
};
namespace DMEvolution
{
    DREADMERIDIAN_API const FDMEvolutionNode* Find(EDMInvestigator Kind, uint8 Slot, uint8 Node);
    DREADMERIDIAN_API float Value(const FDMEvolutionNode& Node, float Crowding, float Danger, float Elite);
}

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
    bool Choose(uint8 Slot, uint8 Node);
    uint8 Node(uint8 Slot) const { return State.Node(Slot); }
    FString Name(uint8 Slot) const;
    void ChooseForBot();
    void Cover(float Strength, int32 Until);
    float IncomingFrom(const class ADMCombatant* Source) const;
private:
    float CoverStrength = 0;
    int32 CoverUntil = 0;
public:

private:
    UPROPERTY(Replicated) FDMProgressionState State;
    TSet<FString> RewardedEvents;
};
