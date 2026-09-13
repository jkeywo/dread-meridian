#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMObjective.generated.h"

class ADMCombatant;
UENUM()
enum class EDMObjectiveState : uint8 { Available, Active, Deteriorating, Critical, Imminent, Completed, Failed, Repaired, ApocalypseConverted };
UENUM()
enum class EDMObjectiveVerb : uint8 { Inspect, Operate, Carry, Escort, Defend, Sequence, Destroy };
UENUM()
enum class EDMObjectiveReward : uint8 { None, Relic, Treatment, Vision, DrainBasin };

USTRUCT()
struct DREADMERIDIAN_API FDMObjectiveStep
{
    GENERATED_BODY()
    UPROPERTY() EDMObjectiveVerb Verb = EDMObjectiveVerb::Inspect;
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() FVector Destination = FVector::ZeroVector;
    UPROPERTY() int32 WorkTicks = 20;
    UPROPERTY() FString Instruction;
    UPROPERTY() TArray<int32> Sequence;
};

/** Accepted objective state, suitable for an authority snapshot; not a whole-run migration save. */
USTRUCT()
struct DREADMERIDIAN_API FDMObjectiveSnapshot
{
    GENERATED_BODY()
    UPROPERTY() int32 Version = 1;
    UPROPERTY() FString Id;
    UPROPERTY() EDMObjectiveState State = EDMObjectiveState::Available;
    UPROPERTY() int32 Step = 0;
    UPROPERTY() int32 Progress = 0;
    UPROPERTY() int32 Deadline = 0;
    UPROPERTY() bool bRewarded = false;
    UPROPERTY() bool bRevealed = false;
    UPROPERTY() bool bApocalypse = false;
    UPROPERTY() FVector PayloadLocation = FVector::ZeroVector;
};

/** Native interactive objective. Durations/ranges are provisional fixture tuning. */
UCLASS()
class DREADMERIDIAN_API ADMObjective : public AActor
{
    GENERATED_BODY()
public:
    ADMObjective();
    virtual void Tick(float Delta) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    bool Configure(const FString& Id, const FString& Title, const TArray<FDMObjectiveStep>& Plan, EDMObjectiveReward RewardKind, int32 Deadline = 0);
    bool ConfigureAuthored(const FString& TemplateId, FVector Origin, int32 Difficulty, const FString& InstanceId);
    bool Interact(ADMCombatant* Actor, int32 Symbol = -1);
    void Release(ADMCombatant* Actor, bool bExplicitInterrupt = false);
    void Step(int32 Tick);
    bool Fail();
    bool Repair(const TArray<FDMObjectiveStep>& Replacement, const FString& Explanation);
    bool ConvertApocalypse();
    bool Restore(const FDMObjectiveSnapshot& Snapshot);
    FDMObjectiveSnapshot Capture() const { return PublicState; }
    bool IsInteracting(const ADMCombatant* Actor) const;
    bool IsTerminal() const;
    const FDMObjectiveStep* Current() const;
    UPROPERTY(Replicated) FDMObjectiveSnapshot PublicState;
    UPROPERTY(Replicated) FString DisplayTitle;
    UPROPERTY(Replicated) FString RepairExplanation;
    UPROPERTY(Replicated) TArray<FDMObjectiveStep> Steps;
    UPROPERTY(Replicated) EDMObjectiveReward Reward = EDMObjectiveReward::None;
    UPROPERTY(Replicated) TObjectPtr<ADMCombatant> Destructible;
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMCombatant>> Targets;
    UPROPERTY(Replicated) int32 Difficulty = 0;
    UPROPERTY(Replicated) int32 ObservedSymbol = 0;
    UPROPERTY(Replicated) bool bSequenceReady = false;
    UPROPERTY(Replicated) FString CarrierId;
    UPROPERTY(Replicated) bool bVisionOnline = false;
    UPROPERTY(Replicated) bool bBasinDrained = false;
    bool bDisruption = false;
    int32 XPReward = 100;
private:
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
    TWeakObjectPtr<ADMCombatant> Participant;
    int32 DamageAtStart = -1;
    int32 LastTick = -1;
    int32 SequenceStarted = -1;
    bool Eligible(ADMCombatant* Actor, FVector At) const;
    bool Contested(FVector At) const;
    void Advance();
    void GrantReward();
    void Publish(const FString& Reason);
};
