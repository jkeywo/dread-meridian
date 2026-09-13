#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMMadnessRules.h"
#include "DMMadnessComponent.generated.h"
class ADMCombatPlayerController;
UCLASS(Config=Game, ClassGroup=(Combat))
class DREADMERIDIAN_API UDMMadnessComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere) FDMMadnessSettings Settings;
    void Reset();
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
    int32 GroundUntil = 0, SymptomUntil = 0;
    FVector GroundPosition = FVector::ZeroVector;
    FString SymptomId, SymptomText, LastSent;
    TWeakObjectPtr<ADMCombatPlayerController> LastOwner;
    int32 Now() const;
    bool Authority() const;
    void Publish(const FDMMadnessState& Before, const FString& Reason);
    void Deliver();
};
