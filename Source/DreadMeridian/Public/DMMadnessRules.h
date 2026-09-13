#pragma once
#include "CoreMinimal.h"
#include "DMMadnessRules.generated.h"

UENUM(BlueprintType)
enum class EDMMadnessFamily : uint8 { None, Obsession, Compulsion, Dissociation, Perception };

USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMMadnessCue
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString Id;
    UPROPERTY(BlueprintReadOnly) FString TargetId;
    UPROPERTY(BlueprintReadOnly) FString Label;
    UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) int32 Until = 0;
    UPROPERTY(BlueprintReadOnly) int32 Progress = 0;
    UPROPERTY(BlueprintReadOnly) int32 Goal = 1;
};

USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMMadnessSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) float Early = 25;
    UPROPERTY(EditAnywhere) float Mid = 50;
    UPROPERTY(EditAnywhere) float High = 75;
    UPROPERTY(EditAnywhere) int32 CrisisTicks = 100;
    UPROPERTY(EditAnywhere) float CrisisRecovery = 50;
    UPROPERTY(EditAnywhere) int32 CrisisRestTicks = 100;
    UPROPERTY(EditAnywhere) int32 GroundTicks = 30;
    UPROPERTY(EditAnywhere) float GroundRecovery = 10;
    UPROPERTY(EditAnywhere) int32 FamilyInterval = 100;
    UPROPERTY(EditAnywhere) int32 IgnoreTicks = 40;
    UPROPERTY(EditAnywhere) float IgnorePressure = 5;
    UPROPERTY(EditAnywhere) float IndulgeRecovery = 2;
    UPROPERTY(EditAnywhere) float FamilyRange = 1000;
    void Sanitize();
};

/** Private owner presentation, carried only on the owning PlayerController. */
USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMMadnessView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString EntityId;
    UPROPERTY(BlueprintReadOnly) float Current = 0;
    UPROPERTY(BlueprintReadOnly) float Floor = 0;
    UPROPERTY(BlueprintReadOnly) int32 Band = 0;
    UPROPERTY(BlueprintReadOnly) int32 CrisisUntil = 0;
    UPROPERTY(BlueprintReadOnly) int32 GroundUntil = 0;
    UPROPERTY(BlueprintReadOnly) FString SymptomId;
    UPROPERTY(BlueprintReadOnly) FString SymptomText;
    UPROPERTY(BlueprintReadOnly) int32 SymptomUntil = 0;
    UPROPERTY(BlueprintReadOnly) EDMMadnessFamily Family = EDMMadnessFamily::None;
    UPROPERTY(BlueprintReadOnly) TArray<FDMMadnessCue> Cues;
    FString Summary() const;
};

/** Pure authoritative core. Copy/restore is a rules test boundary, not a migration save. */
struct DREADMERIDIAN_API FDMMadnessState
{
    float Current = 0, Floor = 0;
    int32 CrisisUntil = 0, RestUntil = 0;
    int32 Band(const FDMMadnessSettings& S) const;
    bool Add(float Amount, int32 Tick, const FDMMadnessSettings& S);
    bool Recover(float Amount);
    bool RaiseFloor(float Value, int32 Tick, const FDMMadnessSettings& S);
    void Step(int32 Tick, const FDMMadnessSettings& S);
private:
    void Enter(int32 Tick, const FDMMadnessSettings& S);
};
