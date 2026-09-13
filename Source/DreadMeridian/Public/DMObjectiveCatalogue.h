#pragma once
#include "DMObjective.h"

/** Authored mechanical alternatives. Selection belongs to the future HTN planner. */
struct DREADMERIDIAN_API FDMObjectiveDefinition
{
    FString Id, Title;
    TArray<FDMObjectiveStep> Steps;
    EDMObjectiveReward Reward = EDMObjectiveReward::None;
    int32 ChainStage = -1;
    bool bDisruption = false;
};
namespace DMObjectiveCatalogue
{
    DREADMERIDIAN_API TArray<FString> Ids();
    /** Difficulty 0..4 is explicitly supplied; this module never advances or subscribes to Ritual. */
    DREADMERIDIAN_API bool Build(const FString& Id, FVector Origin, int32 Difficulty, FDMObjectiveDefinition& Out);
}
