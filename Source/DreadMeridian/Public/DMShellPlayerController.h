#pragma once

#include "CoreMinimal.h"
#include "DMCombatPlayerController.h"
#include "DMShellPlayerController.generated.h"

/**
 * Adds the shell screens' pointer input to the ordinary combat controller. Shell buttons are HUD
 * hit boxes that consume the click, so combat input never sees a press aimed at a menu.
 */
UCLASS()
class DREADMERIDIAN_API ADMShellPlayerController : public ADMCombatPlayerController
{
    GENERATED_BODY()
public:
    ADMShellPlayerController();
    virtual void PlayerTick(float DeltaTime) override;

    /** Named by the HUD's hit boxes; see ADMShellHUD. Each is validated again by the state machine. */
    UFUNCTION(Server, Reliable) void ServerShellStart();
    UFUNCTION(Server, Reliable) void ServerShellLaunch();
    UFUNCTION(Server, Reliable) void ServerShellDismiss();
    UFUNCTION(Server, Reliable) void ServerShellQuit();
};
