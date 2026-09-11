#pragma once

#include "CoreMinimal.h"
#include "DMCombatGameMode.h"
#include "DMShellState.h"
#include "DMShellGameMode.generated.h"

class ULevelStreamingDynamic;

/**
 * Game mode for the persistent shell level. It hosts the front-end screens, streams the mission
 * sublevel in on Launch Expedition, then starts the ordinary sandbox encounter in the same world.
 *
 * It derives from ADMCombatGameMode so the encounter, bots, pings and capture behave exactly as
 * they do when the sandbox map is opened directly; the only difference is that the encounter is
 * deferred until the lobby launches it.
 *
 * Scope: the lobby is a display of placeholder party/scenario data. Hero preference resolution,
 * invites, matchmaking, mutator selection, settings and the real case report have no
 * implementation behind them and are drawn as explicit placeholders.
 */
UCLASS()
class DREADMERIDIAN_API ADMShellGameMode : public ADMCombatGameMode
{
    GENERATED_BODY()
public:
    ADMShellGameMode();
    virtual void StartPlay() override;

    const FDMShellState& GetShellState() const { return Shell; }

    /** Server entry points for the shell screens. Each is a no-op in a phase that does not allow it. */
    void RequestStart();
    void RequestLaunch();
    void RequestDismiss();
    void RequestQuit();

    /** Mission sublevel, streamed in on launch. */
    UPROPERTY(EditDefaultsOnly, Category = "Shell")
    TSoftObjectPtr<UWorld> MissionLevel;

    /** Shell level reopened to tear a finished run down. */
    UPROPERTY(EditDefaultsOnly, Category = "Shell")
    FName ShellLevelName = TEXT("/Game/DreadMeridian/Maps/L_Shell");

protected:
    virtual bool ShouldBeginEncounterOnStartPlay() const override { return false; }
    virtual void OnEncounterComplete(bool bVictory) override;

private:
    FDMShellState Shell;
    FTimerHandle ProbeTimer;
    UPROPERTY() TObjectPtr<ULevelStreamingDynamic> MissionStream;

    /** Applies the action a transition asked for, then republishes the phase to clients. */
    void Apply(EDMShellAction Action);
    void PublishPhase();
    void StreamMission();

    UFUNCTION()
    void OnMissionShown();

    /** -DMShellProbe drives Start then Launch headlessly so the flow can be smoke tested. */
    void StartProbe();
    void StepProbe();
    /** -DMShellShot additionally paces the probe and captures each screen to Saved/Screenshots. */
    bool bProbeShots = false;
    int32 ProbeStep = 0;
};
