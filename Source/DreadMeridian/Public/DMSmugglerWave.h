#pragma once
#include "CoreMinimal.h"
enum class EDMSmugglerWave : uint8 { Occupation, Arrival, Finale, Finished };
enum class EDMWaveAction : uint8 { None, Announce, SpawnPosse, Victory, Defeat };
struct FDMSmugglerWave
{
    EDMSmugglerWave Stage = EDMSmugglerWave::Occupation;
    int32 ArrivalTick = 0;
    EDMWaveAction Advance(bool bInvestigatorsAlive, bool bEnemiesAlive, int32 Tick)
    {
        if (Stage == EDMSmugglerWave::Finished) { return EDMWaveAction::None; }
        if (!bInvestigatorsAlive) { Stage = EDMSmugglerWave::Finished; return EDMWaveAction::Defeat; }
        if (Stage == EDMSmugglerWave::Occupation && !bEnemiesAlive)
        { Stage = EDMSmugglerWave::Arrival; ArrivalTick = Tick + 30; return EDMWaveAction::Announce; }
        if (Stage == EDMSmugglerWave::Arrival)
        {
            if (bEnemiesAlive) { Stage = EDMSmugglerWave::Occupation; return EDMWaveAction::None; }
            if (Tick >= ArrivalTick) { Stage = EDMSmugglerWave::Finale; return EDMWaveAction::SpawnPosse; }
        }
        if (Stage == EDMSmugglerWave::Finale && !bEnemiesAlive)
        { Stage = EDMSmugglerWave::Finished; return EDMWaveAction::Victory; }
        return EDMWaveAction::None;
    }
};
