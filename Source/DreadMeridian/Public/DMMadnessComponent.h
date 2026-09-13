#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMMadnessRules.h"
#include "DMMadnessComponent.generated.h"
class ADMCombatPlayerController;
class ADMCombatant;
UCLASS(Config=Game, ClassGroup=(Combat))
class DREADMERIDIAN_API UDMMadnessComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere) FDMMadnessSettings Settings;
    void Reset();
    static constexpr int32 SupportedFamilies = 4;
    void AssignFamily(EDMMadnessFamily Value);
    void ScheduleEcho(uint8 Slot, ADMCombatant* Target, FVector Point, float Strength = 0, FVector WireStart = FVector::ZeroVector);
    bool InteractPerception(const FString& CueId);
    void OnDamage(ADMCombatant* Target, float HealthLoss);
    float Outgoing(ADMCombatant* Target) const;
    void ResolveCrisis(const FString& Reason);
    bool Add(float Amount, const FString& Reason);
    bool Recover(float Amount, const FString& Reason);
    bool RaiseFloor(float Value, const FString& Reason);
    bool BeginGrounding();
    void InterruptGrounding();
    void Step(int32 Tick);
    /** Authored server-side context chooses a symptom from an unlocked pool. No client submission. */
    bool Manifest(const FString& Id, const FString& Text, int32 RequiredBand, int32 DurationTicks);
    FDMMadnessView View() const;
private:
    FDMMadnessState State;
    EDMMadnessFamily Family = EDMMadnessFamily::None;
    TArray<FDMMadnessCue> Cues;
    int32 NextFamilyTick = 0, NextIgnoreTick = 0;
    bool bFamilyCrisis = false;
    void StepFamily(int32 Tick);
    struct FEcho
    {
        uint8 Slot = 0, Kind = 0;
        FVector Origin, Point, WireStart;
        float Strength = 0;
        int32 Due = 0;
        bool bAnchored = false;
    };
    TArray<FEcho> Echoes;
    void StepEchoes(int32 Tick);
    void ResolveEcho(const FEcho& E, int32 Tick);
    void StepPerception(int32 Tick);
    int32 NextPerceptionUse = 0, NextPerceptionHazard = 0, PerceptionSerial = 0;
    void StepCompulsion(int32 Tick);
    void Indulge(int32 Index);
    void FamilyEvent(const FString& Action, const FString& Target = TEXT(""));
    TArray<ADMCombatant*> Candidates() const;
    int32 GroundUntil = 0, SymptomUntil = 0;
    FVector GroundPosition = FVector::ZeroVector;
    FString SymptomId, SymptomText, LastSent;
    TWeakObjectPtr<ADMCombatPlayerController> LastOwner;
    int32 Now() const;
    bool Authority() const;
    void Publish(const FDMMadnessState& Before, const FString& Reason);
    void Deliver();
};
