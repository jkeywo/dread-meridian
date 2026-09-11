#include "DMShellPlayerController.h"
#include "DMShellGameMode.h"
#include "DMGameState.h"
#include "Engine/World.h"

namespace
{
ADMShellGameMode* ShellMode(const APlayerController* Player)
{
    return Player && Player->GetWorld() ? Player->GetWorld()->GetAuthGameMode<ADMShellGameMode>() : nullptr;
}
}

ADMShellPlayerController::ADMShellPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void ADMShellPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    // A pointer belongs to the shell screens; the mission keeps the combat crosshair.
    const ADMGameState* State = GetWorld() ? GetWorld()->GetGameState<ADMGameState>() : nullptr;
    const bool bShell = State && State->ShellPhase != EDMShellPhase::Mission;
    DefaultMouseCursor = bShell ? EMouseCursor::Default : EMouseCursor::Crosshairs;
}

void ADMShellPlayerController::ServerShellStart_Implementation()
{ if (ADMShellGameMode* Mode = ShellMode(this)) { Mode->RequestStart(); } }

void ADMShellPlayerController::ServerShellLaunch_Implementation()
{ if (ADMShellGameMode* Mode = ShellMode(this)) { Mode->RequestLaunch(); } }

void ADMShellPlayerController::ServerShellDismiss_Implementation()
{ if (ADMShellGameMode* Mode = ShellMode(this)) { Mode->RequestDismiss(); } }

void ADMShellPlayerController::ServerShellQuit_Implementation()
{ if (ADMShellGameMode* Mode = ShellMode(this)) { Mode->RequestQuit(); } }
