#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMInvestigatorComponent.generated.h"

UENUM(BlueprintType)
enum class EDMInvestigator : uint8 { None, Sapper, Photographer, Medium, Smuggler };

USTRUCT(BlueprintType)
struct FDMSubjectResource
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString Id;
    UPROPERTY(BlueprintReadOnly) float Value = 0;
    UPROPERTY() int32 LastEngagedTick = 0;
    UPROPERTY() FVector Location = FVector::ZeroVector;
};

// Numeric defaults are provisional sandbox tuning. Base Q lives in DMPrimaryComponent.
UCLASS(ClassGroup=(DreadMeridian), meta=(BlueprintSpawnableComponent))
class DREADMERIDIAN_API UDMInvestigatorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMInvestigatorComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void Initialize(EDMInvestigator Value);
    FString DisplayName() const;
    FString PassiveName() const;
    FString ResourceSummary(const FString& TargetId = TEXT("")) const;
    FLinearColor Color() const;
    /** Team colour by kind, so screens without a live component (menu, lobby) match the HUD. */
    static FLinearColor ColorFor(EDMInvestigator Value);
    float Range() const;
    float Damage() const;
    int32 Interval() const;
    void Step(int32 Tick, const FString& EngagedId);
    void Pressure(int32 Tick, float Amount);
    bool OnHit(const FString& TargetId, int32 Tick);
    float DamageMultiplier(const FString& TargetId, bool bSuppressed) const;
    float Resistance() const;
    float Stickiness() const;
    bool CollectComponent();
    // Server-only integration points for the kit abilities and authored phenomena.
    void AddExposure(const FString& TargetId, float Amount, bool bVisibleCommitment, int32 Tick);
    float PeekExposure(const FString& TargetId) const;
    /** Returns the stored Exposure and zeroes it, unless bExposureFrozen (Impossible Photograph) keeps it. */
    float ConsumeExposure(const FString& TargetId);
    void BindSpirit(const FString& SpiritId, const FString& TargetId, FVector Location);
    void ThinPlace(FVector Location, float Strength);
    void UpdateSpiritLocation(const FString& TargetId, FVector Location);
    void UpdateSpiritLocationById(const FString& SpiritId, FVector Location);
    /** A called spirit leaves whatever it was attached to, so Spirit Lash no longer feeds it through that target. */
    void ClearSpiritTarget(const FString& SpiritId);
    /** Intercession normally exhausts the spirit it calls on; Keep is the fraction of Attention left behind. */
    void SpendAttention(const FString& SpiritId, float Keep);
    float PeekAttention(const FString& SpiritId) const;
    /** Compatibility entry for kit pressure; authoritative state lives in MadnessCore. */
    void AddMadness(float Amount, const FString& Reason);
    UPROPERTY(Replicated, BlueprintReadOnly) EDMInvestigator Kind = EDMInvestigator::None;
    UPROPERTY(Replicated, BlueprintReadOnly) float Momentum = 0;
    /** Server-only compatibility projection; never replicated or placed in public summaries. */
    UPROPERTY(BlueprintReadOnly) float Madness = 0;
    /** Smuggler R: Momentum never falls below this while it is set. */
    float MomentumFloor = 0;
    /** Photographer R: Exposure decay paused and Develop does not consume. */
    bool bExposureFrozen = false;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Charges = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Components = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Combo = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) TArray<FDMSubjectResource> Exposure;
    UPROPERTY(Replicated, BlueprintReadOnly) TArray<FDMSubjectResource> Spirits;
    UPROPERTY(Replicated) TArray<FString> SpiritTargets;
    static constexpr int32 ChargeCapacity = 3;
private:
    bool Authority() const;
    int32 LastPressureTick = -100;
    FString LastTarget;
};
