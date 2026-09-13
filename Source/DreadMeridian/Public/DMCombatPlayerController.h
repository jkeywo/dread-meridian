#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DMPing.h"
#include "DMMadnessRules.h"

#include "DMCombatPlayerController.generated.h"

struct FInputActionValue;
class ADMCombatant;
class ADMScroungePickup;
class UInputAction;
UCLASS()
class DREADMERIDIAN_API ADMCombatPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ADMCombatPlayerController();
    UFUNCTION(Server, Reliable) void ServerEvolve(uint8 Slot, uint8 Node);
    bool EvolutionOpen() const { return bEvolutionOpen; }
    int32 EvolutionCursor() const { return EvolutionSelection; }
    TArray<FIntPoint> EvolutionChoices() const;

    virtual void OnPossess(APawn* InPawn) override;
    UFUNCTION(Client, Reliable) void ClientMadness(const FDMMadnessView& View);
    UFUNCTION(Client, Reliable) void ClientVerifyMadness(const FDMMadnessView& Expected);
    UFUNCTION(Server, Reliable) void ServerGround();
    UFUNCTION(Server, Reliable) void ServerInteractPerception(const FString& CueId);
    FDMMadnessView PrivateMadness() const;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void PawnLeavingGame() override;
    void StartAutoAttack(ADMCombatant* Target = nullptr);
    /** True while any ability is in targeting mode. Slot 0 is Q; 1..3 are W/E/R. */
    bool IsAiming() const { return bAiming; }
    int32 GetAimSlot() const { return AimSlot; }
    /** Range ring for whichever slot is aiming. */
    float AimRange() const;
    void GetAim(ADMCombatant*& Target, FVector& Point) const;
    /** Empty when the aimed cast would be accepted, else the rejection reason. Client-safe. */
    FString AimFailure(ADMCombatant* Target, FVector Point) const;
    FString AimName() const;
    FString QFeedback() const;
    UFUNCTION(Server, Reliable) void ServerCastQ(ADMCombatant* Target, FVector Point, bool bDetonate);
    /** Slot is an EDMKitSlot. Every check happens server-side in UDMKitComponent::Validate. */
    UFUNCTION(Server, Reliable) void ServerCastKit(uint8 Slot, ADMCombatant* Target, FVector Point);
    UFUNCTION(Server, Reliable) void ServerCancelWire();
    UFUNCTION(Server, Reliable) void ServerCancelFrame();
    UFUNCTION(Client, Reliable) void ClientQFeedback(const FString& Message);
    ADMCombatant* GetSelectedTarget() const { return SelectedTarget.Get(); }
    UFUNCTION(Server, Reliable) void ServerSelectTarget(ADMCombatant* Target);
    UFUNCTION(Server, Reliable) void ServerRevive(ADMCombatant* Ally);
    UFUNCTION(Client, Reliable) void ClientVerifyCombatState(const FString& ExpectedJson);

    // ---- Pings (GDD 9.3: tap = contextual, hold = compact radial)
    /**
     * Server entry for every ping gesture. Kind is an EDMPingKind. ExistingPingId >= 1 means the gesture landed on a
     * live ping: the author's own ping is cancelled, anyone else's is acknowledged, and Kind/Location/Target are
     * ignored. Otherwise the server validates and creates the ping through ADMCombatGameMode::CreatePing and replies
     * with ClientQFeedback on rejection. Downed players may still ping Help and Perceive.
     */
    UFUNCTION(Server, Reliable) void ServerPing(uint8 Kind, FVector Location, ADMCombatant* Target, int32 ExistingPingId);
    /** True while the ping key is held past HoldSeconds and the radial is showing. */
    bool IsPingRadialOpen() const { return bPingRadialOpen; }
    /** Screen position where the hold began (radial centre). */
    FVector2D GetPingRadialOrigin() const { return PingRadialOrigin; }
    /** Radial entry under the cursor (index into RadialKinds), or INDEX_NONE inside the dead zone. */
    int32 GetPingRadialHover() const;
    /** Radial entries in clockwise order from the top: GoHere, Defend, Retreat, Help, Focus, Ignore, Perceive. */
    static const TArray<EDMPingKind>& RadialKinds();
    static constexpr float PingHoldSeconds = .3f;
    static constexpr float PingRadialDeadZone = 28.f;
private:
    bool bEvolutionOpen = false;
    int32 EvolutionSelection = 0;
    void ToggleEvolution();
    void ConfirmEvolution();
    UPROPERTY() TObjectPtr<UInputAction> EvolutionAction;
    FDMMadnessView MadnessView;
    void StartGrounding();
    void StartPerceptionInteraction();
    UPROPERTY() TObjectPtr<UInputAction> PerceptionAction;
    UPROPERTY() TObjectPtr<UInputAction> GroundAction;
    UPROPERTY() TObjectPtr<class UInputMappingContext> Mapping;
    UPROPERTY() TObjectPtr<UInputAction> MoveAction;
    UPROPERTY() TObjectPtr<UInputAction> ClickAction;
    UPROPERTY() TObjectPtr<UInputAction> AttackAction;
    UPROPERTY() TObjectPtr<UInputAction> CycleAction;
    UPROPERTY() TObjectPtr<UInputAction> ReviveAction;
    UPROPERTY() TObjectPtr<UInputAction> QAction;
    UPROPERTY() TObjectPtr<UInputAction> QPadAction;
    UPROPERTY() TObjectPtr<UInputAction> WAction;
    UPROPERTY() TObjectPtr<UInputAction> EAction;
    UPROPERTY() TObjectPtr<UInputAction> RAction;
    UPROPERTY() TObjectPtr<UInputAction> ConfirmQAction;
    UPROPERTY() TObjectPtr<UInputAction> CancelQAction;
    UPROPERTY() TObjectPtr<UInputAction> DetonateAction;
    UPROPERTY() TObjectPtr<UInputAction> PingAction;
    bool bAiming = false;
    /** 0 = Q (UDMPrimaryComponent), 1..3 = W/E/R (UDMKitComponent). */
    int32 AimSlot = 0;
    bool bQGamepad = false;
    FVector PadAimOffset = FVector::ZeroVector;
    FString LastQFeedback;
    float FeedbackUntil = 0;
    float NextProbeQTime = 0;
    int32 ProbeCastIndex = 0;
    bool bEvolutionProbeSent = false;
    // Ping gesture state (client side)
    bool bPingHeld = false;
    bool bPingRadialOpen = false;
    float PingPressedAt = 0;
    FVector2D PingRadialOrigin = FVector2D::ZeroVector;
    void PingPressed();
    void PingReleased();
    /** Cursor context -> (kind, location, target, existing ping id). Returns false when nothing pingable is under the cursor. */
    bool ResolveContextPing(EDMPingKind& OutKind, FVector& OutLocation, ADMCombatant*& OutTarget, int32& OutExistingPingId) const;
    /** Nearest live ping whose projected marker is within PingRadialDeadZone of the cursor, or INDEX_NONE. */
    int32 PingUnderCursor() const;
    void BeginQ();
    void BeginQPad();
    /** Self-cast slots fire immediately; the rest enter targeting. Recasting the same slot cancels. */
    void BeginSlot(int32 Slot);
    void BeginW(); void BeginE(); void BeginR();
    void ConfirmQ();
    void CancelQ();
    void Detonate();
    TWeakObjectPtr<ADMCombatant> SelectedTarget;
    bool bAutoAttack = false;
    bool bDisconnectScheduled = false;
    bool bVisualScheduled = false;
    void Move(const FInputActionValue& Value);
    void Click();
    void Attack();
    void Cycle();
    void StartRevive();
    void StartTreatment();
    UFUNCTION(Server, Reliable) void ServerTreatment();
    UPROPERTY() TObjectPtr<UInputAction> TreatmentAction;
};
