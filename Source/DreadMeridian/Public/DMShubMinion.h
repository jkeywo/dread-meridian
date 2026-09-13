#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMShubMinion.generated.h"
class ADMCombatant;
UENUM()
enum class EDMShubMinionKind : uint8 { None, Broodling, Goat };
UENUM()
enum class EDMShubMinionAction : uint8 { Hunting, Feeding, Splitting, ChargeWindup, Charging, Retired };
USTRUCT()
struct FDMShubMinionSnapshot
{
    GENERATED_BODY()
    UPROPERTY() int32 Version=1;
    UPROPERTY() EDMShubMinionKind Kind=EDMShubMinionKind::None;
    UPROPERTY() EDMShubMinionAction Action=EDMShubMinionAction::Hunting;
    UPROPERTY() float Feeding=0;
    UPROPERTY() int32 Until=0;
    UPROPERTY() bool bHealing=false;
    UPROPERTY() int32 LastDamage=-1;
    UPROPERTY() int32 LastInterrupt=0;
    UPROPERTY() FString CorpseId;
    UPROPERTY() FVector Destination=FVector::ZeroVector;
};
UCLASS()
class DREADMERIDIAN_API UDMShubMinion : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMShubMinion();
    void Initialize(EDMShubMinionKind Kind,bool bOffspring=false);
    bool ControlsMovement() const;
    void Step(int32 Tick);
    FDMShubMinionSnapshot Capture() const { return State; }
    bool Restore(const FDMShubMinionSnapshot& S);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) FDMShubMinionSnapshot State;
    ADMCombatant* Self() const;
    void BroodStep(int32 Tick);
    void GoatStep(int32 Tick);
    void Event(const FString& Action);
};
