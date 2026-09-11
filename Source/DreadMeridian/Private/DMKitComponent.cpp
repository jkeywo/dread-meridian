#include "DMKitComponent.h"
#include "DMKitAbility.h"
#include "DMAbilityMarker.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMGameState.h"
#include "DMPrimaryComponent.h"
#include "AbilitySystemComponent.h"
#include "Dom/JsonObject.h"
#include "Net/UnrealNetwork.h"

namespace
{
    // Row = EDMInvestigator (None, Sapper, Photographer, Medium, Smuggler), column = W/E/R. Provisional sandbox tuning.
    const FDMKitSpec Specs[5][3] = {
        { {}, {}, {} },
        { { TEXT("Suppressing Fire"), 550, 120, 30, false, false }, { TEXT("Tripwire"), 650, 80, 0, false, true }, { TEXT("Dead Ground"), 0, 600, 60, true, false } },
        { { TEXT("Flashbulb"), 350, 100, 0, false, false }, { TEXT("Develop"), 850, 60, 0, false, false }, { TEXT("Impossible Photograph"), 1200, 600, 80, true, false } },
        { { TEXT("Beckon"), 650, 80, 0, false, false }, { TEXT("Intercession"), 0, 100, 0, true, false }, { TEXT("Open Seance"), 0, 600, 80, true, false } },
        { { TEXT("Shoulder Through"), 400, 90, 5, false, false }, { TEXT("Dig In"), 0, 120, 20, true, false }, { TEXT("Drowned Man Walking"), 0, 600, 80, true, false } },
    };
    constexpr int32 WirePendingTimeoutTicks = 60;
    constexpr float IntercessionProtection = .3f;
    int32 Index(EDMKitSlot Slot) { return FMath::Clamp(static_cast<int32>(Slot), 0, 2); }
}

UDMKitComponent::UDMKitComponent() { SetIsReplicatedByDefault(true); }
ADMCombatant* UDMKitComponent::Self() const { return CastChecked<ADMCombatant>(GetOwner()); }
ADMCombatGameMode* UDMKitComponent::Mode() const { return GetWorld() ? GetWorld()->GetAuthGameMode<ADMCombatGameMode>() : nullptr; }
int32 UDMKitComponent::Now() const { const ADMCombatGameMode* M = Mode(); return M ? M->GetCombatTick() : 0; }
int32 UDMKitComponent::CurrentTick() const
{
    if (Self()->HasAuthority()) { return Now(); }
    const ADMGameState* State = GetWorld() ? GetWorld()->GetGameState<ADMGameState>() : nullptr;
    return State ? State->GetCombatTick() : 0;
}

const FDMKitSpec& UDMKitComponent::Spec(EDMInvestigator Kind, EDMKitSlot Slot)
{ return Specs[FMath::Clamp(static_cast<int32>(Kind), 0, 4)][Index(Slot)]; }
const TCHAR* UDMKitComponent::SlotKey(EDMKitSlot Slot)
{ switch (Slot) { case EDMKitSlot::W: return TEXT("w"); case EDMKitSlot::E: return TEXT("e"); default: return TEXT("r"); } }

void UDMKitComponent::Initialize()
{ if (Self()->HasAuthority()) { AbilityHandle = Self()->GetAbilitySystemComponent()->GiveAbility(FGameplayAbilitySpec(UDMKitAbility::StaticClass(), 1)); } }
void UDMKitComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Self()->HasAuthority()) { Cancel(true); }
    Super::EndPlay(Reason);
}

FString UDMKitComponent::Name(EDMKitSlot Slot) const { return Spec(Self()->Investigator->Kind, Slot).Name; }
float UDMKitComponent::Range(EDMKitSlot Slot) const { return Spec(Self()->Investigator->Kind, Slot).Range; }
bool UDMKitComponent::IsSelfCast(EDMKitSlot Slot) const { return Spec(Self()->Investigator->Kind, Slot).bSelfCast; }
bool UDMKitComponent::IsTwoPoint(EDMKitSlot Slot) const { return Spec(Self()->Investigator->Kind, Slot).bTwoPoint; }
float UDMKitComponent::CooldownSeconds(EDMKitSlot Slot) const
{ switch (Slot) { case EDMKitSlot::W: return CooldownW; case EDMKitSlot::E: return CooldownE; default: return CooldownR; } }
bool UDMKitComponent::IsReady(EDMKitSlot Slot) const
{ return CooldownSeconds(Slot) <= 0 && !IsCharging() && !Self()->IsDown() && !Self()->IsRestrained(); }
int32 UDMKitComponent::CooldownRemaining(EDMKitSlot Slot, int32 Tick) const { return FMath::Max(0, NextCastTick[Index(Slot)] - Tick); }

FString UDMKitComponent::Status(EDMKitSlot Slot) const
{
    const FString Label = Name(Slot);
    if (Label.IsEmpty()) { return TEXT(""); }
    const int32 Tick = CurrentTick();
    if (Slot == EDMKitSlot::R && RActiveUntilTick > Tick) { return FString::Printf(TEXT("%s active %.1fs"), *Label, (RActiveUntilTick - Tick) * .1f); }
    if (Slot == EDMKitSlot::E && BracedUntilTick > Tick) { return FString::Printf(TEXT("Braced %.1fs - E to shove"), (BracedUntilTick - Tick) * .1f); }
    if (Slot == EDMKitSlot::E && bWirePending) { return TEXT("Tripwire - place the second end"); }
    if (Slot == EDMKitSlot::W && ChargeUntilTick > Tick) { return TEXT("Charging"); }
    const float Seconds = CooldownSeconds(Slot);
    if (Seconds > 0) { return FString::Printf(TEXT("%s %.1fs"), *Label, Seconds); }
    return Label + TEXT(" ready");
}

FString UDMKitComponent::Validate(EDMKitSlot Slot, ADMCombatant* Target, FVector Point) const
{
    const ADMCombatant* Actor = Self();
    if (Slot >= EDMKitSlot::Count) { return TEXT("Invalid slot"); }
    if (Point.ContainsNaN()) { return TEXT("Invalid aim point"); }
    if (Target && (!IsValid(Target) || Target->GetWorld() != GetWorld())) { return TEXT("Invalid target"); }
    if (Actor->IsDown() || Actor->IsRestrained() || Actor->bIsEnemy) { return TEXT("Cannot cast in this state"); }
    if (Actor->Investigator->Kind == EDMInvestigator::None || Name(Slot).IsEmpty()) { return TEXT("No ability available"); }
    if (IsCharging()) { return TEXT("Charging"); }
    if (CooldownSeconds(Slot) > 0 || (Actor->HasAuthority() && NextCastTick[Index(Slot)] > Now()))
    { return FString::Printf(TEXT("%s is cooling down"), *FString(SlotKey(Slot)).ToUpper()); }
    return ValidateAbility(Slot, Target, Point);
}

bool UDMKitComponent::Request(EDMKitSlot Slot, ADMCombatant* Target, FVector Point)
{
    ADMCombatGameMode* M = Mode();
    if (!Self()->HasAuthority() || !M || !M->IsCombatActive()) { LastFailure = TEXT("Combat is not active"); return false; }
    LastFailure = Validate(Slot, Target, Point);
    if (!LastFailure.IsEmpty()) { return false; }
    RequestedSlot = Slot; RequestedTarget = Target; RequestedPoint = Point; bResolved = false;
    Self()->GetAbilitySystemComponent()->TryActivateAbility(AbilityHandle);
    return bResolved;
}

bool UDMKitComponent::Resolve()
{
    ADMCombatant* Actor = Self();
    ADMCombatGameMode* M = Mode();
    if (!Actor->HasAuthority() || !M || !M->IsCombatActive()) { return false; }
    ADMCombatant* Target = RequestedTarget.Get();
    LastFailure = Validate(RequestedSlot, Target, RequestedPoint);
    if (!LastFailure.IsEmpty()) { return false; }
    if (!ResolveAbility(RequestedSlot, Target, RequestedPoint)) { return false; }
    M->NoteKitCast(RequestedSlot);
    Actor->RecordResources(SlotKey(RequestedSlot));
    RefreshCooldowns();
    Actor->ForceNetUpdate();
    bResolved = true;
    return true;
}

bool UDMKitComponent::RequestWire(FVector A, FVector B)
{
    // Filled in by the Sapper slice; until then no wire can be placed.
    LastFailure = TEXT("Tripwire is not implemented yet");
    return false;
}

void UDMKitComponent::CancelWire()
{
    if (!Self()->HasAuthority() || !bWirePending) { return; }
    bWirePending = false; PendingWireStart = FVector::ZeroVector; WirePendingSinceTick = 0;
    Emit(EDMKitSlot::E, TEXT("wire_cancelled"));
    Self()->ForceNetUpdate();
}

void UDMKitComponent::StartCooldown(EDMKitSlot Slot, int32 Ticks)
{ NextCastTick[Index(Slot)] = Now() + FMath::Max(0, Ticks); RefreshCooldowns(); }

void UDMKitComponent::RefreshCooldowns()
{
    const int32 Tick = Now();
    CooldownW = FMath::Max(0, NextCastTick[0] - Tick) * .1f;
    CooldownE = FMath::Max(0, NextCastTick[1] - Tick) * .1f;
    CooldownR = FMath::Max(0, NextCastTick[2] - Tick) * .1f;
}

void UDMKitComponent::Emit(EDMKitSlot Slot, const FString& Action, ADMCombatant* Target, const TSharedPtr<FJsonObject>& Extra)
{
    ADMCombatGameMode* M = Mode();
    if (!M) { return; }
    TSharedPtr<FJsonObject> D = Extra.IsValid() ? Extra : MakeShared<FJsonObject>();
    D->SetStringField(TEXT("actor_id"), Self()->EntityId);
    D->SetStringField(TEXT("ability"), Name(Slot));
    D->SetStringField(TEXT("action"), Action);
    if (Target) { D->SetStringField(TEXT("target_id"), Target->EntityId); }
    M->Emit(FString(TEXT("ability.")) + SlotKey(Slot), D.ToSharedRef());
}

void UDMKitComponent::EndR()
{
    if (RActiveUntilTick <= 0) { return; }
    ADMCombatant* Actor = Self();
    RActiveUntilTick = 0;
    Actor->Investigator->MomentumFloor = 0;
    Actor->Investigator->bExposureFrozen = false;
    Actor->ReachBonus = 0;
    Emit(EDMKitSlot::R, TEXT("ended"));
    Actor->ForceNetUpdate();
}

void UDMKitComponent::EndBrace(bool bShove)
{
    if (BracedUntilTick <= 0) { return; }
    ADMCombatant* Actor = Self();
    BracedUntilTick = 0;
    Actor->IncomingMultiplier = 1; Actor->IncomingUntilTick = 0; Actor->BraceResistance = 0;
    StartCooldown(EDMKitSlot::E, Spec(Actor->Investigator->Kind, EDMKitSlot::E).CooldownTicks);
    Emit(EDMKitSlot::E, TEXT("dig_in_ended"));
    Actor->ForceNetUpdate();
}

void UDMKitComponent::Cancel(bool bDestroyMarkers)
{
    if (!Self()->HasAuthority()) { return; }
    ADMCombatant* Actor = Self();
    EndR();
    if (BracedUntilTick > 0)
    { BracedUntilTick = 0; Actor->IncomingMultiplier = 1; Actor->IncomingUntilTick = 0; Actor->BraceResistance = 0; }
    ChargeUntilTick = 0; ChargeHits.Reset();
    if (bWirePending) { bWirePending = false; PendingWireStart = FVector::ZeroVector; WirePendingSinceTick = 0; }
    ProtectionUntilTick = 0; ProtectionTarget = nullptr;
    if (bDestroyMarkers)
    {
        for (ADMAbilityMarker* Marker : Zones) { if (IsValid(Marker)) { Marker->Destroy(); } }
        for (ADMAbilityMarker* Marker : Wires) { if (IsValid(Marker)) { Marker->Destroy(); } }
        Zones.Reset(); Wires.Reset(); Ledger.Clear();
    }
    Actor->ForceNetUpdate();
}

void UDMKitComponent::Step(int32 Tick)
{
    ADMCombatant* Actor = Self();
    if (!Actor->HasAuthority()) { return; }
    RefreshCooldowns();
    ADMCombatGameMode* M = Mode();
    if (!M || !M->IsCombatActive()) { return; }
    if (Actor->IsDown())
    {
        if (RActiveUntilTick > 0 || BracedUntilTick > 0 || ChargeUntilTick > 0 || bWirePending || ProtectionUntilTick > 0) { Cancel(false); }
        return;
    }
    if (bWirePending && Tick - WirePendingSinceTick > WirePendingTimeoutTicks) { CancelWire(); }
    if (ProtectionUntilTick > Tick)
    {
        // Re-applied every step because StepCombat zeroes SpiritProtection before any attack resolves.
        if (ADMCombatant* Ally = ProtectionTarget.Get(); Ally && !Ally->IsDown()) { Ally->SpiritProtection = FMath::Max(Ally->SpiritProtection, IntercessionProtection); }
    }
    else { ProtectionTarget = nullptr; }
    if (RActiveUntilTick > 0 && Tick >= RActiveUntilTick) { EndR(); }
    if (BracedUntilTick > 0 && Tick >= BracedUntilTick) { EndBrace(false); }
    // Wire crossings compare each actor's previous sampled position with this one, so the cache only matters to a
    // Sapper holding live wire. An actor's first sample only seeds: a wire never triggers on a position it never saw.
    if (Wires.IsEmpty()) { LastPositions.Reset(); }
    else { for (ADMCombatant* Other : M->GetCombatants()) { if (IsValid(Other)) { LastPositions.Add(Other, Other->GetActorLocation()); } } }
}

bool UDMKitComponent::IsDeadGroundActive() const
{ return Self()->Investigator->Kind == EDMInvestigator::Sapper && RActiveUntilTick > Now(); }

bool UDMKitComponent::TagTrap(uint8 Trap, int32 Serial, ADMCombatant* Enemy)
{
    ADMCombatGameMode* M = Mode();
    if (!M || !IsValid(Enemy)) { return false; }
    const int32 EnemyIndex = M->GetCombatants().IndexOfByKey(Enemy);
    if (EnemyIndex == INDEX_NONE || !Ledger.Tag(EnemyIndex, Trap, Serial, Now())) { return false; }
    TSharedPtr<FJsonObject> Extra = MakeShared<FJsonObject>();
    Extra->SetStringField(TEXT("trap"), Trap == FDMDeadGroundLedger::Wire ? TEXT("wire") : TEXT("satchel"));
    Extra->SetNumberField(TEXT("serial"), Serial);
    Emit(EDMKitSlot::R, TEXT("dead_ground_tag"), Enemy, Extra);
    Enemy->MulticastPresentation(14, Enemy->GetActorLocation());
    return true;
}

void UDMKitComponent::DropTrap(uint8 Trap, int32 Serial) { Ledger.DropTrap(Trap, Serial); }

FString UDMKitComponent::ReplicationSummary() const
{
    int32 LiveZones = 0, LiveWires = 0;
    for (const ADMAbilityMarker* Marker : Zones) { if (IsValid(Marker)) { ++LiveZones; } }
    for (const ADMAbilityMarker* Marker : Wires) { if (IsValid(Marker)) { ++LiveWires; } }
    return FString::Printf(TEXT("zones=%d wires=%d pending=%d r_until=%d brace_until=%d charge_until=%d"),
        LiveZones, LiveWires, bWirePending ? 1 : 0, RActiveUntilTick, BracedUntilTick, ChargeUntilTick);
}

// ---- Per-kind rules. Each investigator slice replaces its branch; until then the slot reports itself missing.

FString UDMKitComponent::ValidateAbility(EDMKitSlot Slot, ADMCombatant* Target, FVector Point) const
{ return Name(Slot) + TEXT(" is not implemented yet"); }

bool UDMKitComponent::ResolveAbility(EDMKitSlot Slot, ADMCombatant* Target, FVector Point)
{ LastFailure = Name(Slot) + TEXT(" is not implemented yet"); return false; }

void UDMKitComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDMKitComponent, CooldownW); DOREPLIFETIME(UDMKitComponent, CooldownE); DOREPLIFETIME(UDMKitComponent, CooldownR);
    DOREPLIFETIME(UDMKitComponent, RActiveUntilTick); DOREPLIFETIME(UDMKitComponent, BracedUntilTick); DOREPLIFETIME(UDMKitComponent, ChargeUntilTick);
    DOREPLIFETIME(UDMKitComponent, PendingWireStart); DOREPLIFETIME(UDMKitComponent, bWirePending);
    DOREPLIFETIME(UDMKitComponent, Zones); DOREPLIFETIME(UDMKitComponent, Wires);
}
