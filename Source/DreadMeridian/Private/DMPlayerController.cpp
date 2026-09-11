#include "DMPlayerController.h"
#include "DMGameMode.h"
#include "Engine/World.h"

void ADMPlayerController::DMRitualAdvance(int32 Points)
{
#if !UE_BUILD_SHIPPING
    if (ADMGameMode* Mode = GetWorld()->GetAuthGameMode<ADMGameMode>())
    {
        ClientMessage(Mode->AdvanceRitual(Points) ? TEXT("Ritual advanced") : TEXT("Ritual change rejected"));
    }
    else { ClientMessage(TEXT("Harness commands require the local authority.")); }
#endif
}

void ADMPlayerController::DMSummon()
{
#if !UE_BUILD_SHIPPING
    if (ADMGameMode* Mode = GetWorld()->GetAuthGameMode<ADMGameMode>())
    {
        ClientMessage(Mode->Summon() ? TEXT("Apocalypse entered") : TEXT("Summon rejected"));
    }
    else { ClientMessage(TEXT("Harness commands require the local authority.")); }
#endif
}

void ADMPlayerController::DMFinish(bool bVictory)
{
#if !UE_BUILD_SHIPPING
    if (ADMGameMode* Mode = GetWorld()->GetAuthGameMode<ADMGameMode>())
    {
        ClientMessage(Mode->FinishRun(bVictory) ? TEXT("Run finished") : TEXT("Finish rejected"));
    }
    else { ClientMessage(TEXT("Harness commands require the local authority.")); }
#endif
}
