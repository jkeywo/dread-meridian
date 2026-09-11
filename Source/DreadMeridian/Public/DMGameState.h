#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "DMRunState.h"
#include "DMPing.h"
#include "DMShellState.h"
#include "DMGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDMRunStateChanged, FDMRunState, State);

UCLASS()
class DREADMERIDIAN_API ADMGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FDMRunStateChanged OnRunStateChanged;

    UFUNCTION(BlueprintPure)
    FDMRunState GetRunState() const { return RunState; }

    void Publish(const FDMRunState& State);

    /** Server combat tick, replicated so client HUDs can time cooldowns and revive channels. */
    UFUNCTION(BlueprintPure)
    int32 GetCombatTick() const { return CombatTick; }
    void SetCombatTick(int32 Tick);
    UPROPERTY(Replicated, BlueprintReadOnly) FString EncounterObjective;
    void SetEncounterObjective(const FString& Text);
    /** Live pings, a public projection of the game mode's board. Perceive pings carry a location only. */
    UPROPERTY(Replicated, BlueprintReadOnly) TArray<FDMPing> Pings;
    void SetPings(const TArray<FDMPing>& Live);
    /** Which shell screen clients should draw. Always MainMenu when the sandbox map is opened directly. */
    UPROPERTY(Replicated, BlueprintReadOnly) EDMShellPhase ShellPhase = EDMShellPhase::MainMenu;
    /** Outcome shown by the case report; only meaningful while ShellPhase is CaseReport. */
    UPROPERTY(Replicated, BlueprintReadOnly) bool bShellVictory = false;
    void SetShellPhase(EDMShellPhase Phase, bool bVictory);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(ReplicatedUsing=OnRep_RunState)
    FDMRunState RunState;

    UPROPERTY(Replicated)
    int32 CombatTick = 0;

    UFUNCTION()
    void OnRep_RunState();
};
