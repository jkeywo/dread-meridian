#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DMRunState.h"
#include "DMRandomStreams.h"
#include "DMGameMode.generated.h"

UCLASS(Config=Game)
class DREADMERIDIAN_API ADMGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADMGameMode();
    virtual void StartPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    bool AdvanceRitual(int32 Points);
    bool Summon();
    bool FinishRun(bool bVictory);

protected:
    virtual void ConfigureCaptureMetadata(const TSharedRef<class FJsonObject>& Metadata) {}

private:
    UPROPERTY(Config)
    int32 DefaultRunSeed = 1927;

    // Provisional harness tuning, not a GDD balance commitment.
    UPROPERTY(Config)
    int32 RitualPointsPerStage = 100;

    FDMRunState State;
    TUniquePtr<FDMRandomStreams> RandomStreams;
    void Publish(const FString& EventType, int32 RitualDelta = 0);
    void RunSmokeTest();
};
