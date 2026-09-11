#pragma once

#include "CoreMinimal.h"
#include "DMRunState.generated.h"

UENUM(BlueprintType)
enum class EDMRunPhase : uint8
{
    Briefing, Expedition, Apocalypse, Victory, Defeat
};

UENUM(BlueprintType)
enum class EDMRitualStage : uint8
{
    Incipient, Stirring, Intrusion, Convergence, Apocalypse
};

/** Public, non-secret projection. Never put boss choice, RNG state or Madness here. */
USTRUCT(BlueprintType)
struct DREADMERIDIAN_API FDMRunState
{
    GENERATED_BODY()

    static constexpr int32 InvestigatorCount = 4;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EDMRunPhase Phase = EDMRunPhase::Briefing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EDMRitualStage RitualStage = EDMRitualStage::Incipient;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 RitualProgress = 0;

    bool Start();
    bool AdvanceRitual(int32 Points, int32 PointsPerStage);
    bool Summon();
    bool Finish(bool bVictory);
};
