#pragma once

#include "CoreMinimal.h"
#include "DMShellState.generated.h"

/**
 * Front-end screens hosted by the persistent shell level. The mission itself is a streamed
 * sublevel, so every phase below runs in the same world with the same game mode.
 */
UENUM(BlueprintType)
enum class EDMShellPhase : uint8
{
    /** Title with Start / Settings / Exit. */
    MainMenu,
    /** Expedition lobby: scenario, seats, mutators, Leads. Mostly placeholder data. */
    Lobby,
    /** Mission sublevel is streaming in; nothing is interactive. */
    Loading,
    /** Encounter is running. The combat HUD owns the screen. */
    Mission,
    /** Case report with the finished run's outcome. */
    CaseReport
};

/** Side effect the owning game mode must perform after a transition. */
UENUM()
enum class EDMShellAction : uint8
{
    /** Transition was rejected, or needs nothing beyond the phase change. */
    None,
    /** Begin streaming the mission sublevel. */
    StreamMission,
    /** Sublevel is visible; spawn the roster and start the combat timer. */
    BeginEncounter,
    /** Reopen the shell level from scratch, discarding the finished run. */
    RestartShell,
    /** Quit to desktop. */
    Quit
};

/**
 * Shell screen flow as a pure state machine, so the legal transitions can be tested without an
 * engine world. Every request returns the action the caller owes, and an illegal request is a
 * no-op that returns None and leaves Phase untouched.
 */
USTRUCT()
struct DREADMERIDIAN_API FDMShellState
{
    GENERATED_BODY()

    UPROPERTY()
    EDMShellPhase Phase = EDMShellPhase::MainMenu;

    /** Only meaningful in CaseReport. */
    UPROPERTY()
    bool bVictory = false;

    /** Main menu Start. */
    EDMShellAction Start();
    /** Lobby Launch Expedition. */
    EDMShellAction Launch();
    /** The streamed mission sublevel finished loading and is visible. */
    EDMShellAction MissionReady();
    /** The encounter reached victory or defeat. */
    EDMShellAction Complete(bool bInVictory);
    /** Case report dismissal, and the lobby's Back. */
    EDMShellAction Dismiss();
    /** Main menu Exit to desktop. */
    EDMShellAction RequestQuit();

    /** True while the combat HUD should draw instead of the shell screens. */
    bool ShowsMission() const { return Phase == EDMShellPhase::Mission; }
    /** True while the shell accepts pointer input. Loading and Mission do not. */
    bool AcceptsInput() const { return Phase == EDMShellPhase::MainMenu || Phase == EDMShellPhase::Lobby || Phase == EDMShellPhase::CaseReport; }
};
