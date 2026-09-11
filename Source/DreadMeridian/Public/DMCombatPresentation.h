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
};
