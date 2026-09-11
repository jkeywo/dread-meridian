#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMLocomotionPresentation.h"
#include "DMCombatPresentation.generated.h"
class UAnimSequence;
class UStaticMeshComponent;
class USkeletalMesh;
class UStaticMesh;
class UNiagaraSystem;

// Cosmetic observer only: animations never apply damage, movement or cooldowns.
UCLASS()
class DREADMERIDIAN_API UDMCombatPresentation : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMCombatPresentation();
    UFUNCTION(BlueprintCallable) void UpdatePresentation();
    UFUNCTION(BlueprintCallable) void Cue(uint8 Event, FVector Target);
    UFUNCTION(BlueprintCallable) FString CurrentClip() const;
    UStaticMeshComponent* GetHeldItem() const { return HeldItem; }
    UStaticMeshComponent* GetStowedItem() const { return StowedItem; }
private:
    void Play(UAnimSequence* Clip, bool bLoop, float Rate = 1, bool bRestart = false);
    void Action(UAnimSequence* Clip, float Duration);
    /** One-shot effect at a world point, destroyed after Seconds so a looping system cannot outlive its cue. */
    void Spawn(UNiagaraSystem* System, const FVector& At, float Scale, float Seconds);
    /** Effect attached to the owner for Seconds; used for the ultimates' altered-state auras. */
    void Attach(UNiagaraSystem* System, float Seconds);
    void Equip(bool bCamera);
    bool UsesGunPose() const;
    void UpdateEnemyGrip();
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> EnemyLocomotion;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> EnemyActions;
    UPROPERTY() TArray<TObjectPtr<USkeletalMesh>> EnemySkins;
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> EnemyItems;
    UPROPERTY() TArray<TObjectPtr<USkeletalMesh>> Skins;
    UPROPERTY() TArray<TObjectPtr<UAnimSequence>> Clips;
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> Items;
    UPROPERTY() TObjectPtr<UNiagaraSystem> Explosion;
    UPROPERTY() TObjectPtr<UNiagaraSystem> Impact;
    // Named-kit effects, loaded per ability as its cues land.
    UPROPERTY() TObjectPtr<UNiagaraSystem> MuzzleFlash;
    UPROPERTY() TObjectPtr<UNiagaraSystem> WireSnap;
    UPROPERTY() TObjectPtr<UNiagaraSystem> Shock;
    UPROPERTY() TObjectPtr<UNiagaraSystem> DeferredAura;
    UPROPERTY() TObjectPtr<UNiagaraSystem> TargetMark;
    UPROPERTY() TObjectPtr<UNiagaraSystem> Flashbulb;
    UPROPERTY() TObjectPtr<UNiagaraSystem> Developed;
    UPROPERTY() TObjectPtr<UNiagaraSystem> PhotographAura;
    UPROPERTY() TObjectPtr<UNiagaraSystem> SpiritArrival;
    UPROPERTY() TObjectPtr<UNiagaraSystem> Intervention;
    UPROPERTY() TObjectPtr<UNiagaraSystem> SeanceCircle;
    UPROPERTY() TObjectPtr<UNiagaraSystem> BraceAura;
    UPROPERTY() TObjectPtr<UNiagaraSystem> Shockwave;
    UPROPERTY() TObjectPtr<UNiagaraSystem> DrownedAura;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> HeldItem;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> StowedItem;
    UPROPERTY() TObjectPtr<UAnimSequence> Playing;
    UPROPERTY() TObjectPtr<UAnimSequence> ActionClip;
    FDMLocomotionPresentation Locomotion;
    uint8 Kind = 255;
    bool bWasDown = false;
    bool bCameraHeld = false;
    float ActionUntil = 0;
    float LastHitTime = -1;
    /** Yaw offset (relative to the actor) left over from the last attack facing; cleared once real movement resumes. */
    float FacingOffset = 0;
    FVector AimPoint = FVector::ZeroVector;
    float AimUntil = -1;
    /** Camera raised until this time, for the Photographer's flash and ultimate. */
    float CameraUntil = -1;
};
