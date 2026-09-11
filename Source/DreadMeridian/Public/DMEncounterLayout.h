#pragma once
#include "CoreMinimal.h"
#include "DMSmugglerComponent.h"

// Authored smuggler occupation; all numerical tuning remains provisional.
namespace DMEncounterLayout
{
    // Playable extent inside the arena walls. The movement clamp and the HUD minimap share it.
    inline constexpr float PlayableX = 2900;
    inline constexpr float PlayableY = 2400;
    inline constexpr int32 CampCount = 3;
    inline constexpr int32 CampSize = 3;
    inline constexpr int32 PatrolSize = 2;
    inline constexpr int32 EnemyCount = CampCount * CampSize + PatrolSize;
    inline constexpr int32 PosseCount = 5;
    inline EDMSmuggler EnemyRole(int32 Index)
    {
        const EDMSmuggler Roles[] = { EDMSmuggler::Gunman, EDMSmuggler::Bruiser, EDMSmuggler::Lookout,
            EDMSmuggler::Gunman, EDMSmuggler::Bomber, EDMSmuggler::Bruiser,
            EDMSmuggler::Gunman, EDMSmuggler::Lookout, EDMSmuggler::Bomber,
            EDMSmuggler::Gunman, EDMSmuggler::Lookout };
        return Roles[FMath::Clamp(Index,0,EnemyCount-1)];
    }
    inline EDMSmuggler PosseRole(int32 Index)
    {
        const EDMSmuggler Roles[] = { EDMSmuggler::GangBoss, EDMSmuggler::Gunman, EDMSmuggler::Bruiser, EDMSmuggler::Lookout, EDMSmuggler::Bomber };
        return Roles[FMath::Clamp(Index,0,PosseCount-1)];
    }
    inline FVector PossePosition(int32 Index) { return FVector(2400 - (Index%2)*160, (Index-2)*180, 95); }
    inline FVector Camp(int32 Index)
    {
        const FVector Centers[] = { FVector(0, -1500, 95), FVector(1600, 0, 95), FVector(0, 1500, 95) };
        return Centers[FMath::Clamp(Index, 0, CampCount - 1)];
    }
    inline FVector PatrolPoint(int32 Index)
    {
        const FVector Points[] = { FVector(-650, -850, 95), FVector(800, -850, 95), FVector(800, 850, 95), FVector(-650, 850, 95) };
        return Points[Index % 4];
    }
    inline FVector EnemyPosition(int32 Index)
    {
        if (Index >= CampCount * CampSize) { return PatrolPoint(0) + FVector(0, (Index - CampCount * CampSize) * 120, 0); }
        return Camp(Index / CampSize) + FVector((Index % CampSize == 1) ? 140 : -70, (Index % CampSize - 1) * 140, 0);
    }
}
