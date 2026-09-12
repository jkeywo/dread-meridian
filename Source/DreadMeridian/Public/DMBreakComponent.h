#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMKitRules.h"
#include "DMBreakComponent.generated.h"

class ADMCombatant;

/** Authority owns Resolve transitions; clients receive only the public encounter projection. */
UCLASS(ClassGroup=(Combat), Config=Game, meta=(BlueprintSpawnableComponent))
class DREADMERIDIAN_API UDMBreakComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMBreakComponent();
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Break") FDMBreakSettings Settings;
    UPROPERTY(Replicated, BlueprintReadOnly) float CurrentResolve = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) float MaxResolve = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bResisting = false;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ResistUntilTick = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 InterruptUntilTick = 0;
    void Reset();
    void Step(int32 Tick);
    void AddPressure(float Amount, ADMCombatant* Source = nullptr, const FString& AbilityId = FString());
    /** Encounter-authored, independent interrupt permission; does not grant roots, holds or displacement. */
    void OpenInterruptWindow(int32 DurationTicks);
    bool IsProtected() const;
    FString ReplicationSummary() const;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    ADMCombatant* Self() const;
    FDMBreakMeter Meter;
    void Project(int32 Tick);
    void Record(const TCHAR* Event, ADMCombatant* Source = nullptr, const FString& AbilityId = FString(), float Applied = 0);
};
