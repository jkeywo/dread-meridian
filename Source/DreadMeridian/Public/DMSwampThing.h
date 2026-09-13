#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMSwampThing.generated.h"
class ADMCombatant;
class ADMAbilityMarker;
UENUM()
enum class EDMSwampThing : uint8 { None, Crawler, Lurker, Spitter, Grasper, OldThing };
USTRUCT()
struct FDMSwampSnapshot
{
    GENERATED_BODY()
    UPROPERTY() int32 Version=1;
    UPROPERTY() EDMSwampThing Role=EDMSwampThing::None;
    UPROPERTY() FVector Home=FVector::ZeroVector;
    UPROPERTY() FVector Aim=FVector::ZeroVector;
    UPROPERTY() FVector CastOrigin=FVector::ZeroVector;
    UPROPERTY() FString TargetId;
    UPROPERTY() int32 ResolveAt=0;
    UPROPERTY() int32 ReadyAt=0;
    UPROPERTY() int32 InterruptSerial=0;
    UPROPERTY() FVector Pool=FVector::ZeroVector;
    UPROPERTY() int32 PoolUntil=0;
    UPROPERTY() int32 NextPoolTick=0;
};
/** Native Swamp Thing roles; all numeric values are provisional sandbox tuning. */
UCLASS()
class DREADMERIDIAN_API UDMSwampThing : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMSwampThing();
    bool Initialize(EDMSwampThing Role);
    void Step(int32 Tick);
    void Stop();
    bool TrySignature(ADMCombatant* Target);
    float Range() const;
    float Speed() const;
    float DamageMultiplier(const ADMCombatant* Target) const;
    bool IsIsolated(const ADMCombatant* Target) const;
    FDMSwampSnapshot Capture() const { return State; }
    bool Restore(const FDMSwampSnapshot& Snapshot);
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    UPROPERTY(Replicated) FDMSwampSnapshot State;
    UPROPERTY() TObjectPtr<ADMAbilityMarker> Tell;
    UPROPERTY() TObjectPtr<ADMAbilityMarker> PoolMarker;
    int32 LastTick=-1;
    ADMCombatant* Self() const;
    void Project();
    void Cancel(int32 Tick);
    void Resolve(int32 Tick);
    void Record(const FString& Action);
};
