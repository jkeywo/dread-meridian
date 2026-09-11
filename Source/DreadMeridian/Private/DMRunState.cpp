#include "DMRunState.h"

bool FDMRunState::Start()
{
    if (Phase != EDMRunPhase::Briefing) { return false; }
    Phase = EDMRunPhase::Expedition;
    return true;
}

bool FDMRunState::AdvanceRitual(int32 Points, int32 PointsPerStage)
{
    if (Phase != EDMRunPhase::Expedition || Points <= 0 || PointsPerStage <= 0)
    {
        return false;
    }
    const int64 Total = static_cast<int64>(RitualProgress) + Points;
    const int64 Stage = static_cast<int64>(RitualStage) + Total / PointsPerStage;
    if (Stage >= static_cast<int64>(EDMRitualStage::Apocalypse))
    {
        return Summon();
    }
    RitualStage = static_cast<EDMRitualStage>(Stage);
    RitualProgress = static_cast<int32>(Total % PointsPerStage);
    return true;
}

bool FDMRunState::Summon()
{
    if (Phase != EDMRunPhase::Expedition) { return false; }
    RitualStage = EDMRitualStage::Apocalypse;
    RitualProgress = 0;
    Phase = EDMRunPhase::Apocalypse;
    return true;
}

bool FDMRunState::Finish(bool bVictory)
{
    if (Phase != EDMRunPhase::Expedition && Phase != EDMRunPhase::Apocalypse) { return false; }
    // Victory requires the boss phase; team wipe can end the expedition early.
    if (bVictory && Phase != EDMRunPhase::Apocalypse) { return false; }
    Phase = bVictory ? EDMRunPhase::Victory : EDMRunPhase::Defeat;
    return true;
}
