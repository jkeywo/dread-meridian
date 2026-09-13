#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMKitRules.h"
#include "DMRelicComponent.generated.h"
class ADMCombatant;
DECLARE_MULTICAST_DELEGATE_TwoParams(FDMRelicControlEvent,ADMCombatant*,const FDMControl&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDMRelicBreakEvent,ADMCombatant*,float,bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FDMRelicOverhealEvent,float);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDMRelicTierEvent,int32,int32);
DECLARE_MULTICAST_DELEGATE(FDMRelicObjectiveEvent);
UENUM()
enum class EDMRelic : uint8 { Swagger, Medal, Knot, Overheal, Morphine, Rosary, Coin, Gloves, Count };
USTRUCT()
struct DREADMERIDIAN_API FDMRelicInventory
{
    GENERATED_BODY()
    UPROPERTY() int32 Version = 1;
    UPROPERTY() TArray<EDMRelic> Items;
    UPROPERTY() TArray<FString> AwardIds;
};
UCLASS(Config=Game)
class DREADMERIDIAN_API UDMRelicComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMRelicComponent();
    FDMRelicControlEvent AcceptedControl;
    FDMRelicBreakEvent AcceptedBreak;
    FDMRelicOverhealEvent ExcessHealing;
    FDMRelicTierEvent TierEntered;
    FDMRelicObjectiveEvent ObjectiveFinished;
    UPROPERTY(Config,EditAnywhere) int32 Capacity = 2; // Provisional, not a locked cap.
    bool CanAcquire(EDMRelic Relic) const;
    bool Acquire(EDMRelic Relic,const FString& AwardId);
    bool Has(EDMRelic Relic) const { return Inventory.Items.Contains(Relic); }
    FDMRelicInventory Capture() const { return Inventory; }
    bool Restore(const FDMRelicInventory& Snapshot);
    static FString Name(EDMRelic Relic);
    static FString Description(EDMRelic Relic);
    FString Summary() const;
    float Value(EDMRelic Relic) const;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) FDMRelicInventory Inventory;
    ADMCombatant* Self() const;
};
