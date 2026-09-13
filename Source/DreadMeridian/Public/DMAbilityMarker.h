#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMAbilityMarker.generated.h"
class ADMCombatant;

/** How the marker draws itself. Circle covers satchels, spirits and hostile hazards; Cone and Wire are kit zones. */
UENUM()
enum class EDMMarkerShape : uint8 { Circle, Cone, Wire };

UCLASS()
class DREADMERIDIAN_API ADMAbilityMarker : public AActor
{
    GENERATED_BODY()
public:
    ADMAbilityMarker();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    /** Combat tick on either side: the authority's game mode, or the replicated GameState projection on clients. */
    int32 CurrentTick() const;
    UPROPERTY(Replicated) bool bHostile = false;
    UPROPERTY(Replicated) FString CustomLabel;
    UPROPERTY(Replicated) bool bSpirit = false;
    UPROPERTY(Replicated) TObjectPtr<ADMCombatant> BoundTarget;
    UPROPERTY(Replicated) FString SpiritId;
    UPROPERTY(Replicated) float Radius = 220;
    UPROPERTY(Replicated) float Attention = 0;
    UPROPERTY(Replicated) EDMMarkerShape Shape = EDMMarkerShape::Circle;
    /** Cone: direction and half-angle in degrees from the marker's location, out to Length. */
    UPROPERTY(Replicated) FVector Direction = FVector::ZeroVector;
    UPROPERTY(Replicated) float HalfAngle = 25;
    UPROPERTY(Replicated) float Length = 600;
    /** Wire: the far endpoint (the marker sits at the near one). */
    UPROPERTY(Replicated) FVector WireEnd = FVector::ZeroVector;
    /** Beckoned spirit in flight: passive effects are suspended and the orb trails. */
    UPROPERTY(Replicated) bool bTravelling = false;
    UPROPERTY(Replicated) FVector TravelGoal = FVector::ZeroVector;
    /** Units/second while travelling. Tick() does the actual per-frame movement so it glides instead of
     * stepping once per combat tick (10Hz); StepMedium only watches for arrival and fires its side effects. */
    static constexpr float TravelSpeed = 900.f;
    UPROPERTY(Replicated) float TravelRate = TravelSpeed;
    /** Replicated so clients can draw arming and expiry; satchels keep using it for their server-side checks. */
    UPROPERTY(Replicated) int32 ArmedTick = 0;
    /** 0 = persistent. */
    UPROPERTY(Replicated) int32 ExpiresTick = 0;
    /** Stable identity for Dead Ground tags and eviction; never an array index. */
    UPROPERTY(Replicated) int32 Serial = 0;
    bool IsArmed() const { return CurrentTick() >= ArmedTick; }
private:
    void UpdateCircle();
    void UpdateCone();
    void UpdateWire();
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Orb;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Ring;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Label;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> Glow;
    /** The Medium's ghostly-figure mesh, shown instead of Orb for spirits. */
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> GhostBody;
    /** Server-only: where an unbound spirit started, so it wanders around that point rather than drifting away. */
    FVector WanderOrigin = FVector::ZeroVector;
    bool bWanderOriginSet = false;
};
