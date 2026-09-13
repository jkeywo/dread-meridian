#pragma once
#include "CoreMinimal.h"
#include "DMRunState.h"
#include "DMInjuryRules.h"
#include "DMInvestigatorComponent.h"
#include "DMPing.h"

class ADMCombatant;
class UWorld;

/** One combatant as the HUD sees it. Client-safe: every field is a replicated public projection. */
struct DREADMERIDIAN_API FDMHudUnit
{
    TWeakObjectPtr<ADMCombatant> Actor;
    FString EntityId;
    FString Name;
    EDMInvestigator Kind = EDMInvestigator::None;
    FLinearColor Tint = FLinearColor::White;
    float Health = 0;
    float MaxHealth = 0;
    float Shield = 0;
    bool bEnemy = false;
    bool bDown = false;
    bool bLocal = false;
    int32 Injuries = 0;
    int32 Grievous = 0;
    FString Control;
    FString Resources;
    FString ReviverId;
    float ReviveProgress = 0;
    /** This unit is currently attacking the locally controlled investigator. */
    bool bTargetingLocal = false;
    FVector Location = FVector::ZeroVector;
    /** Private local-owner Madness projection. */
    TArray<EDMInjury> SpecificInjuries;
    float Madness = 0, MadnessFloor = 0;
    int32 MadnessBand = 0;
    bool bCrisis = false, bGrounding = false;
    FString Symptom;
    /** 0..1 remaining elite Resolve; meaningless for common enemies. */
    float BreakFraction = 0;
    bool bElite = false;
    bool bBroken = false;
    bool bResisting = false;
    bool bInterruptible = false;
    bool bSuppressed = false;

    float HealthFraction() const { return MaxHealth > 0 ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f; }
    float ShieldFraction() const { return MaxHealth > 0 ? FMath::Clamp(Shield / MaxHealth, 0.f, 1.f) : 0.f; }
    bool IsReviving() const { return bDown && !ReviverId.IsEmpty(); }
};

/** One live ping as the HUD draws it: label, author tint, age and who answered. */
struct DREADMERIDIAN_API FDMHudPing
{
    int32 Id = 0;
    EDMPingKind Kind = EDMPingKind::Enemy;
    FString Label;
    FString AuthorId;
    FString AuthorName;
    bool bAuthorBot = false;
    bool bMine = false;
    bool bSubjective = false;
    FLinearColor Tint = FLinearColor::White;
    /** World location to project; follows the target when the ping has one. */
    FVector Location = FVector::ZeroVector;
    /** 0 = fresh, 1 = about to expire. */
    float Age = 0;
    int32 OnIt = 0;
    int32 Busy = 0;
    int32 Acknowledged = 0;
};

/** One ability slot as the HUD draws it: key glyph, name, cooldown and whether its window is running. */
struct DREADMERIDIAN_API FDMHudAbility
{
    FString Key;
    FString Name;
    FString Status;
    /** 0 when ready, else the fraction of the slot's own cooldown still to wait. */
    float Cooldown = 0;
    float CooldownSeconds = 0;
    bool bReady = false;
    /** A persistent window (R, Dig In) is currently running. */
    bool bActive = false;
    bool bImplemented = false;
};

/**
 * Everything the combat HUD draws, assembled once per frame from replicated state.
 * It reports only what the sandbox actually simulates: evolutions, objectives and the full
 * boss resonance remain gaps. Madness and its family cues are private to the owner; Resolve is public combat state.
 */
struct DREADMERIDIAN_API FDMHudModel
{
    bool bHasSelf = false;
    bool bHasTarget = false;
    FDMHudUnit Self;
    FDMHudUnit Target;
    TArray<FDMHudUnit> Allies;
    TArray<FDMHudUnit> Enemies;
    TArray<FDMHudPing> Pings;
    /** Q, W, E, R for the local investigator, in that order. */
    TArray<FDMHudAbility> Abilities;

    EDMRunPhase Phase = EDMRunPhase::Briefing;
    EDMRitualStage RitualStage = EDMRitualStage::Incipient;
    int32 RitualProgress = 0;
    FString EncounterObjective;
    int32 CombatTick = 0;
    int32 EnemiesStanding = 0;
    int32 EnemyCount = 0;
    int32 InvestigatorsStanding = 0;
    int32 InvestigatorCount = 0;
    /** 0 when the basic attack is ready, otherwise the fraction of the interval still to wait. */
    float BasicCooldown = 0;
    /** Seconds left on the basic attack, from the replicated server tick rate. */
    float BasicCooldownSeconds = 0;

    static constexpr float CombatTickSeconds = .1f;

    static FDMHudModel Build(UWorld* World, const ADMCombatant* LocalPawn, const ADMCombatant* SelectedTarget);
    static FDMHudUnit Read(const ADMCombatant& Unit, const FString& LocalEntityId);
    /** Short marker label per kind: ENEMY, FOCUS, IGNORE, GO, DEFEND, RETREAT, HELP, PICKUP, "?" for Perceive. */
    static FString PingLabel(EDMPingKind Kind);
};
