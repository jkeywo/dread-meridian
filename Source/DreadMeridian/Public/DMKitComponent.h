#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "DMKitRules.h"
#include "DMInvestigatorComponent.h"
#include "DMKitComponent.generated.h"
class ADMCombatant;
class ADMAbilityMarker;
class ADMCombatGameMode;

UENUM(BlueprintType)
enum class EDMKitSlot : uint8 { W, E, R, Count UMETA(Hidden) };

/** Static per-(kind, slot) data. Numbers are provisional sandbox tuning, not GDD balance. */
struct DREADMERIDIAN_API FDMKitSpec
{
    const TCHAR* Name = TEXT("");
    float Range = 0;
    int32 CooldownTicks = 0;
    int32 DurationTicks = 0;
    bool bSelfCast = false;
    bool bTwoPoint = false;
};

/**
 * W / E / R for the four investigators (GDD Appendix K, A nodes). Server-authoritative like UDMPrimaryComponent:
 * Request validates and activates the GAS shim, Resolve applies the ability, Step advances persistent effects
 * beside Primary->Step. Validate never touches the game mode so clients can preview it; combat-active is checked
 * in Request/Resolve only. Cooldown floats and the *UntilTick fields are the replicated HUD projections.
 */
UCLASS()
class DREADMERIDIAN_API UDMKitComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDMKitComponent();
    void Initialize();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    static const FDMKitSpec& Spec(EDMInvestigator Kind, EDMKitSlot Slot);
    static const TCHAR* SlotKey(EDMKitSlot Slot);

    /** "" when the cast may proceed, else the reason. Client-safe; spends nothing. */
    FString Validate(EDMKitSlot Slot, ADMCombatant* Target, FVector Point) const;
    bool Request(EDMKitSlot Slot, ADMCombatant* Target, FVector Point);
    /** Sapper Tripwire for bots and probes: both points validated before anything is stored. */
    bool RequestWire(FVector A, FVector B);
    bool Resolve();
    void Step(int32 Tick);
    /** Ends charge, brace, R window, pending wire and protection; bDestroyMarkers also removes zones and wires. */
    void Cancel(bool bDestroyMarkers = false);
    void CancelWire();

    FString Name(EDMKitSlot Slot) const;
    FString Status(EDMKitSlot Slot) const;
    float Range(EDMKitSlot Slot) const;
    bool IsSelfCast(EDMKitSlot Slot) const;
    bool IsTwoPoint(EDMKitSlot Slot) const;
    float CooldownSeconds(EDMKitSlot Slot) const;
    /** Replicated cooldown elapsed and no blocking state (client-safe). */
    bool IsReady(EDMKitSlot Slot) const;
    /** Ticks until the slot may cast again (authority). */
    int32 CooldownRemaining(EDMKitSlot Slot, int32 Tick) const;
    /** Authority: game mode tick; clients: the GameState projection. */
    int32 CurrentTick() const;
    bool IsCharging() const { return ChargeUntilTick > CurrentTick(); }
    /**
     * Authority: carries an active Shoulder Through forward by DeltaSeconds and stops it on world geometry.
     * The charge advances per frame so it glides; the rules tick still owns contacts and when it ends.
     */
    void AdvanceCharge(float DeltaSeconds);
    bool IsBraced() const { return BracedUntilTick > CurrentTick(); }
    bool IsRActive() const { return RActiveUntilTick > CurrentTick(); }
    /** Integers and ids only, so the network probe can compare it. */
    FString ReplicationSummary() const;

    // ---- Dead Ground (used by UDMPrimaryComponent while the Sapper's R is active)
    bool IsDeadGroundActive() const;
    /** Records a trigger instead of resolving it. Returns false when that trap already tagged that enemy. */
    bool TagTrap(uint8 Trap, int32 Serial, ADMCombatant* Enemy);
    void DropTrap(uint8 Trap, int32 Serial);

    UPROPERTY(Replicated) float CooldownW = 0;
    UPROPERTY(Replicated) float CooldownE = 0;
    UPROPERTY(Replicated) float CooldownR = 0;
    UPROPERTY(Replicated) int32 RActiveUntilTick = 0;
    UPROPERTY(Replicated) int32 BracedUntilTick = 0;
    UPROPERTY(Replicated) int32 ChargeUntilTick = 0;
    UPROPERTY(Replicated) FVector PendingWireStart = FVector::ZeroVector;
    UPROPERTY(Replicated) bool bWirePending = false;
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMAbilityMarker>> Zones;
    UPROPERTY(Replicated) TArray<TObjectPtr<ADMAbilityMarker>> Wires;
    FString LastFailure;
    FDMDeadGroundLedger Ledger;

private:
    ADMCombatant* Self() const;
    ADMCombatGameMode* Mode() const;
    int32 Now() const;
    void Emit(EDMKitSlot Slot, const FString& Action, ADMCombatant* Target = nullptr, const TSharedPtr<FJsonObject>& Extra = nullptr);
    void StartCooldown(EDMKitSlot Slot, int32 Ticks);
    /** Brings a running cooldown forward, never back: an altered state answers now, not when the old wait expires. */
    void ShortenCooldown(EDMKitSlot Slot, int32 Ticks);
    void RefreshCooldowns();
    /** Per-kind rules after the shared checks. */
    FString ValidateAbility(EDMKitSlot Slot, ADMCombatant* Target, FVector Point) const;
    bool ResolveAbility(EDMKitSlot Slot, ADMCombatant* Target, FVector Point);
    FString ValidateSapper(EDMKitSlot Slot, FVector Point) const;
    bool ResolveSapper(EDMKitSlot Slot, FVector Point);
    FString ValidatePhotographer(EDMKitSlot Slot, ADMCombatant* Target, FVector Point) const;
    bool ResolvePhotographer(EDMKitSlot Slot, ADMCombatant* Target, FVector Point);
    FString ValidateMedium(EDMKitSlot Slot, FVector Point) const;
    bool ResolveMedium(EDMKitSlot Slot, FVector Point);
    /** Beckoned spirits travelling to their destination, and the pulse each one makes when it lands. */
    void StepMedium(int32 Tick);
    /** Highest-Attention bound spirit, or nullptr. Ties break on the lowest serial so the choice is stable. */
    ADMAbilityMarker* BestSpirit() const;
    FString ValidateSmuggler(EDMKitSlot Slot, FVector Point) const;
    bool ResolveSmuggler(EDMKitSlot Slot, FVector Point);
    /** Advances a Shoulder Through: one step per tick, hitting whatever it reaches once each. */
    void StepSmuggler(int32 Tick);
    /** Suppression ticks, wire crossings and the Dead Ground batch. */
    void StepSapper(int32 Tick);
    /** Spawns a wire between two validated ground points, evicting the oldest when a third is placed. */
    bool PlaceWire(FVector A, FVector B);
    /** The wire's control effect, applied to whoever crossed it. */
    void TriggerWire(ADMAbilityMarker* Wire, ADMCombatant* Enemy);
    /** Resolves every tag the ledger holds, then consumes the traps that carried one. */
    bool bResolvingLedger = false;
    void ResolveDeadGround();
    /** Line of sight between two world points, ignoring combatant bodies. */
    bool Sight(const FVector& From, const FVector& To) const;
    void EndR();
    void EndBrace(bool bShove);
    int32 NextCastTick[3] = { 0, 0, 0 };
    int32 WirePendingSinceTick = 0;
    FVector ChargeDirection = FVector::ZeroVector;
    TArray<TWeakObjectPtr<ADMCombatant>> ChargeHits;
    TWeakObjectPtr<ADMCombatant> ProtectionTarget;
    int32 ProtectionUntilTick = 0;
    TMap<TWeakObjectPtr<ADMCombatant>, FVector> LastPositions;
    int32 MarkerSerial = 0;
    FGameplayAbilitySpecHandle AbilityHandle;
    EDMKitSlot RequestedSlot = EDMKitSlot::W;
    TWeakObjectPtr<ADMCombatant> RequestedTarget;
    FVector RequestedPoint = FVector::ZeroVector;
    bool bResolved = false;
};
