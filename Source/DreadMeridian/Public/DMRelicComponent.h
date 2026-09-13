#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMKitRules.h"
#include "DMRelicRuntime.h"
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
USTRUCT()
struct FDMRelicSnapshot
{
    GENERATED_BODY()
    UPROPERTY() int32 Version=1;
    UPROPERTY() FDMRelicInventory Inventory;
    UPROPERTY() FDMRelicRuntime Runtime;
};
UCLASS(Config=Game)
class DREADMERIDIAN_API UDMRelicComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMRelicComponent();
    virtual void BeginPlay() override;
    float BreakMultiplierAgainst(const ADMCombatant* Target) const;
    float SpendMedal(ADMCombatant* Target,const FString& AbilityId,bool bBasic);
    void ClearBreakContributions() { Runtime.Contributions.Reset(); }
    void Step(int32 Tick);
    float ReviveFactor() const { return Has(EDMRelic::Morphine) ? .65f : 1.f; }
    float ResourceMultiplier() const;
    float MovementMultiplier() const;
    float ControlExposure() const;
    bool ProtectsObjective() const;
    float IncomingMultiplier() const;
    void ShieldSpent(float Amount) { if (Self() && GetOwner()->HasAuthority()) { Runtime.OwnedShield=FMath::Max(0.f,Runtime.OwnedShield-Amount); } }
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
    FDMRelicSnapshot CaptureFull() const { FDMRelicSnapshot S; S.Inventory=Inventory; S.Runtime=Runtime; return S; }
    bool RestoreFull(const FDMRelicSnapshot& Snapshot);
    static FString Name(EDMRelic Relic);
    static FString Description(EDMRelic Relic);
    FString Summary() const;
    float Value(EDMRelic Relic) const;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) FDMRelicInventory Inventory;
    FDMRelicRuntime Runtime;
    ADMCombatant* Self() const;
    void OnControl(ADMCombatant* Target,const FDMControl& Control);
    void OnBreak(ADMCombatant* Source,float Amount,bool bBroke);
    void PromoteThreat(ADMCombatant* Target);
    bool bResolvingKnot=false;
    int32 LastStepTick=-1;
    void StoreOverheal(float Amount);
    void BoostResource(int32 Before,int32 After);
    void StepWakes(int32 Tick);
    void ProjectWakes();
    void OnObjectiveFinished();
    UPROPERTY() TArray<TObjectPtr<class ADMAbilityMarker>> WakeMarkers;
};
