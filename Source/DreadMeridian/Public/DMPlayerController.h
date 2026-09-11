#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DMPlayerController.generated.h"

/** Harness commands only. Production investigator input is a separate next milestone. */
UCLASS()
class DREADMERIDIAN_API ADMPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    UFUNCTION(Exec)
    void DMRitualAdvance(int32 Points = 100);

    UFUNCTION(Exec)
    void DMSummon();

    UFUNCTION(Exec)
    void DMFinish(bool bVictory = true);
};
