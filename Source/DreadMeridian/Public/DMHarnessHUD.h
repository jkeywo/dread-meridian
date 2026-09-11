#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DMHarnessHUD.generated.h"

UCLASS()
class DREADMERIDIAN_API ADMHarnessHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
