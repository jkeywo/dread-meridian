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

    // Sapper. Provisional sandbox tuning, not GDD balance.
    constexpr float ZoneHalfAngle = 25;          // degrees either side of the aim direction
    constexpr float ZoneLength = 600;
    constexpr int32 ZoneTickInterval = 5;        // suppression re-applies on this cadence
    constexpr float ZoneDamage = 4;
    constexpr int32 ZoneSuppressionTicks = 10;
    constexpr float WireMinLength = 60;
    constexpr float WireMaxLength = 500;
    constexpr int32 WireArmDelayTicks = 5;
    constexpr int32 MaxWires = 2;
    constexpr float WireSlack = 35;              // capsule radius plus logical-tick sampling
    constexpr float WireDamage = 15;
    constexpr int32 WireSlowTicks = 20;
    constexpr int32 WireStaggerTicks = 20;
    constexpr float WireBreakPressure = 35;
    constexpr float SatchelDamage = 55;          // mirrors UDMPrimaryComponent's satchel blast
    constexpr float DeadGroundMadness = 30;
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
    // Atomic for bots and probes: a rejected second end must not leave a pending start behind, which the next
    // cast would silently adopt. Both ends are validated before anything is stored.
    ADMCombatGameMode* M = Mode();
    if (!Self()->HasAuthority() || !M || !M->IsCombatActive()) { LastFailure = TEXT("Combat is not active"); return false; }
    const FVector Saved = PendingWireStart;
    const bool bSavedPending = bWirePending;
    bWirePending = false;
    LastFailure = Validate(EDMKitSlot::E, nullptr, A);
    if (LastFailure.IsEmpty())
    {
        bWirePending = true; PendingWireStart = A;
        LastFailure = Validate(EDMKitSlot::E, nullptr, B);
        bWirePending = false;
    }
    if (!LastFailure.IsEmpty()) { bWirePending = bSavedPending; PendingWireStart = Saved; return false; }
    FVector GroundA, GroundB;
    if (!Self()->Primary->Ground(A, GroundA) || !Self()->Primary->Ground(B, GroundB))
    { bWirePending = bSavedPending; PendingWireStart = Saved; LastFailure = TEXT("Choose clear ground in the arena"); return false; }
    if (!PlaceWire(GroundA, GroundB)) { bWirePending = bSavedPending; PendingWireStart = Saved; return false; }
    StartCooldown(EDMKitSlot::E, Spec(Self()->Investigator->Kind, EDMKitSlot::E).CooldownTicks);
    M->NoteKitCast(EDMKitSlot::E);
    Self()->RecordResources(SlotKey(EDMKitSlot::E));
    Self()->ForceNetUpdate();
    return true;
}

bool UDMKitComponent::Sight(const FVector& From, const FVector& To) const
{ return Self()->Smuggler->Sight(From, To); }

bool UDMKitComponent::PlaceWire(FVector A, FVector B)
{
    ADMCombatant* Actor = Self();
    // A third wire evicts the oldest; its Dead Ground tags go with it so nothing resolves against a dead trap.
    Wires.RemoveAll([](const ADMAbilityMarker* M) { return !IsValid(M); });
    while (Wires.Num() >= MaxWires)
    {
        if (ADMAbilityMarker* Old = Wires[0].Get(); IsValid(Old)) { Ledger.DropTrap(FDMDeadGroundLedger::Wire, Old->Serial); Old->Destroy(); }
        Wires.RemoveAt(0);
    }
    ADMAbilityMarker* Wire = GetWorld()->SpawnActor<ADMAbilityMarker>(A, FRotator::ZeroRotator);
    if (!Wire) { LastFailure = TEXT("Placement failed"); return false; }
    Wire->SetOwner(Actor);
    Wire->Shape = EDMMarkerShape::Wire; Wire->WireEnd = B;
    Wire->ArmedTick = Now() + WireArmDelayTicks; Wire->Serial = ++MarkerSerial;
    Wires.Add(Wire);
    bWirePending = false; PendingWireStart = FVector::ZeroVector; WirePendingSinceTick = 0;
    TSharedPtr<FJsonObject> Extra = MakeShared<FJsonObject>();
    Extra->SetNumberField(TEXT("serial"), Wire->Serial);
    Extra->SetNumberField(TEXT("length"), FVector::Dist2D(A, B));
    Emit(EDMKitSlot::E, TEXT("place_wire"), nullptr, Extra);
    Actor->MulticastPresentation(11, A);
    return true;
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
    if (Actor->Investigator->Kind == EDMInvestigator::Sapper) { StepSapper(Tick); }
    // Ends after StepSapper so the last tick of the window still defers, and any tags left open resolve at once.
    if (RActiveUntilTick > 0 && Tick >= RActiveUntilTick)
    {
        if (!Ledger.Tags.IsEmpty()) { ResolveDeadGround(); }
        EndR();
    }
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
{
    if (Self()->Investigator->Kind == EDMInvestigator::Sapper) { return ValidateSapper(Slot, Point); }
    return Name(Slot) + TEXT(" is not implemented yet");
}

bool UDMKitComponent::ResolveAbility(EDMKitSlot Slot, ADMCombatant* Target, FVector Point)
{
    if (Self()->Investigator->Kind == EDMInvestigator::Sapper) { return ResolveSapper(Slot, Point); }
    LastFailure = Name(Slot) + TEXT(" is not implemented yet");
    return false;
}

// ------------------------------------------------------------------------------------------- Sapper

FString UDMKitComponent::ValidateSapper(EDMKitSlot Slot, FVector Point) const
{
    const ADMCombatant* Actor = Self();
    const FVector Origin = Actor->GetActorLocation();
    const FDMKitSpec& Kit = Spec(EDMInvestigator::Sapper, Slot);
    switch (Slot)
    {
    case EDMKitSlot::W:
        // A direction cast: the point only says which way the cone faces.
        if (FVector::DistSquared2D(Origin, Point) <= 1) { return TEXT("Choose a direction to fire"); }
        if (FVector::DistSquared2D(Origin, Point) > FMath::Square(Kit.Range)) { return TEXT("Out of range"); }
        if (!Sight(Origin, Point)) { return TEXT("Line of sight blocked"); }
        return TEXT("");
    case EDMKitSlot::E:
    {
        if (FVector::DistSquared2D(Origin, Point) > FMath::Square(Kit.Range)) { return TEXT("Out of Tripwire range"); }
        FVector Ground;
        if (!Actor->Primary->Ground(Point, Ground)) { return TEXT("Choose clear ground in the arena"); }
        if (!bWirePending) { return TEXT(""); }
        const float Length = FVector::Dist2D(PendingWireStart, Ground);
        if (Length < WireMinLength) { return TEXT("Wire is too short"); }
        if (Length > WireMaxLength) { return TEXT("Wire is too long"); }
        if (!Sight(PendingWireStart + FVector(0, 0, 40), Ground + FVector(0, 0, 40))) { return TEXT("Wire is obstructed"); }
        return TEXT("");
    }
    default:
        // Dead Ground defers triggers that would have happened anyway, so it needs something armed to defer.
        for (const ADMAbilityMarker* M : Actor->Primary->Satchels) { if (IsValid(M) && M->ArmedTick <= CurrentTick()) { return TEXT(""); } }
        for (const ADMAbilityMarker* M : Wires) { if (IsValid(M) && M->ArmedTick <= CurrentTick()) { return TEXT(""); } }
        return TEXT("No traps prepared");
    }
}

bool UDMKitComponent::ResolveSapper(EDMKitSlot Slot, FVector Point)
{
    ADMCombatant* Actor = Self();
    const int32 Tick = Now();
    const FDMKitSpec& Kit = Spec(EDMInvestigator::Sapper, Slot);
    switch (Slot)
    {
    case EDMKitSlot::W:
    {
        // Fire-and-forget: the cone stays where it was cast and the Sapper keeps moving and shooting.
        for (ADMAbilityMarker* Old : Zones) { if (IsValid(Old)) { Old->Destroy(); } }
        Zones.Reset();
        const FVector Origin = Actor->GetActorLocation();
        ADMAbilityMarker* Zone = GetWorld()->SpawnActor<ADMAbilityMarker>(Origin - FVector(0, 0, 70), FRotator::ZeroRotator);
        if (!Zone) { LastFailure = TEXT("Placement failed"); return false; }
        Zone->SetOwner(Actor);
        Zone->Shape = EDMMarkerShape::Cone;
        Zone->Direction = (Point - Origin).GetSafeNormal2D();
        Zone->HalfAngle = ZoneHalfAngle; Zone->Length = ZoneLength;
        Zone->ArmedTick = Tick; Zone->ExpiresTick = Tick + Kit.DurationTicks; Zone->Serial = ++MarkerSerial;
        Zones.Add(Zone);
        StartCooldown(Slot, Kit.CooldownTicks);
        TSharedPtr<FJsonObject> Extra = MakeShared<FJsonObject>();
        Extra->SetNumberField(TEXT("direction_x"), Zone->Direction.X);
        Extra->SetNumberField(TEXT("direction_y"), Zone->Direction.Y);
        Emit(Slot, TEXT("suppressing_fire"), nullptr, Extra);
        Actor->MulticastPresentation(9, Point);
        return true;
    }
    case EDMKitSlot::E:
    {
        FVector Ground;
        if (!Actor->Primary->Ground(Point, Ground)) { LastFailure = TEXT("Choose clear ground in the arena"); return false; }
        if (!bWirePending)
        {
            // First end only: no cooldown yet, and the cast is not finished until the second end lands.
            bWirePending = true; PendingWireStart = Ground; WirePendingSinceTick = Tick;
            Emit(Slot, TEXT("wire_start"));
            Actor->ForceNetUpdate();
            return true;
        }
        if (!PlaceWire(PendingWireStart, Ground)) { return false; }
        StartCooldown(Slot, Kit.CooldownTicks);
        return true;
    }
    default:
        RActiveUntilTick = Tick + Kit.DurationTicks;
        StartCooldown(Slot, Kit.CooldownTicks);
        Actor->Investigator->AddMadness(DeadGroundMadness, TEXT("dead_ground"));
        Emit(Slot, TEXT("dead_ground_started"));
        Actor->MulticastPresentation(13, Actor->GetActorLocation());
        return true;
    }
}

void UDMKitComponent::TriggerWire(ADMAbilityMarker* Wire, ADMCombatant* Enemy)
{
    FDMControl Control;
    Control.Damage = WireDamage; Control.Slow = 1; Control.SlowTicks = WireSlowTicks;
    Control.StaggerTicks = WireStaggerTicks; Control.bInterrupt = true; Control.BreakPressure = WireBreakPressure;
    Enemy->ApplyControl(Control, Self(), TEXT("ability.e.tripwire"));
    Emit(EDMKitSlot::E, TEXT("wire_triggered"), Enemy);
    Enemy->MulticastPresentation(12, Enemy->GetActorLocation());
}

void UDMKitComponent::ResolveDeadGround()
{
    ADMCombatant* Actor = Self();
    ADMCombatGameMode* M = Mode();
    if (!M) { Ledger.Clear(); return; }
    const TArray<TObjectPtr<ADMCombatant>>& Roster = M->GetCombatants();
    int32 Resolved = 0;
    for (const FDMDeadGroundLedger::FTag& Tag : Ledger.Tags)
    {
        ADMCombatant* Enemy = Roster.IsValidIndex(Tag.Enemy) ? Roster[Tag.Enemy].Get() : nullptr;
        if (!IsValid(Enemy) || Enemy->IsDown()) { continue; }
        if (Tag.Trap == FDMDeadGroundLedger::Wire)
        {
            TObjectPtr<ADMAbilityMarker>* Wire = Wires.FindByPredicate([&](const TObjectPtr<ADMAbilityMarker>& W) { return IsValid(W) && W->Serial == Tag.Serial; });
            if (Wire) { TriggerWire(Wire->Get(), Enemy); ++Resolved; }
        }
        else
        {
            // The tag was the legitimate trigger; the blast resolves against the enemy that earned it.
            Actor->DealCombatDamage(Enemy, SatchelDamage, TEXT("ability.q.satchel"));
            Actor->MulticastPresentation(4, Enemy->GetActorLocation());
            ++Resolved;
        }
    }
    // Only traps that actually tagged something are spent; the rest stay armed (GDD K.2).
    TSet<int32> SpentWires, SpentSatchels;
    for (const FDMDeadGroundLedger::FTag& Tag : Ledger.Tags)
    { (Tag.Trap == FDMDeadGroundLedger::Wire ? SpentWires : SpentSatchels).Add(Tag.Serial); }
    for (int32 I = Wires.Num() - 1; I >= 0; --I)
    {
        ADMAbilityMarker* Wire = Wires[I].Get();
        if (!IsValid(Wire) || SpentWires.Contains(Wire->Serial)) { if (IsValid(Wire)) { Wire->Destroy(); } Wires.RemoveAt(I); }
    }
    Actor->Primary->ConsumeSatchels(SpentSatchels);
    TSharedPtr<FJsonObject> Extra = MakeShared<FJsonObject>();
    Extra->SetNumberField(TEXT("count"), Resolved);
    Emit(EDMKitSlot::R, TEXT("dead_ground_resolved"), nullptr, Extra);
    Ledger.Clear();
    Actor->ForceNetUpdate();
}

void UDMKitComponent::StepSapper(int32 Tick)
{
    ADMCombatant* Actor = Self();
    ADMCombatGameMode* M = Mode();
    if (!M) { return; }

    for (int32 I = Zones.Num() - 1; I >= 0; --I)
    {
        ADMAbilityMarker* Zone = Zones[I].Get();
        if (!IsValid(Zone)) { Zones.RemoveAt(I); continue; }
        if (Tick >= Zone->ExpiresTick)
        {
            Zone->Destroy(); Zones.RemoveAt(I);
            Emit(EDMKitSlot::W, TEXT("suppression_ended"));
            continue;
        }
        if ((Tick - Zone->ArmedTick) % ZoneTickInterval != 0) { continue; }
        const FVector Origin = Zone->GetActorLocation() + FVector(0, 0, 70);
        for (ADMCombatant* Enemy : M->GetCombatants())
        {
            if (!IsValid(Enemy) || !Enemy->bIsEnemy || Enemy->IsDown()) { continue; }
            if (!DMKitRules::PointInCone(Origin, Zone->Direction, Zone->HalfAngle, Zone->Length, Enemy->GetActorLocation())) { continue; }
            if (!Sight(Origin, Enemy->GetActorLocation())) { continue; }
            Actor->DealCombatDamage(Enemy, ZoneDamage, TEXT("ability.w.suppressing_fire"));
            if (Enemy->IsDown()) { continue; }
            Enemy->ApplySuppression(Tick + ZoneSuppressionTicks, Actor);
            Actor->MulticastAttackFX(Origin, Enemy->GetActorLocation(), Actor->Investigator->Color(), 1);
            Enemy->MulticastPresentation(10, Enemy->GetActorLocation());
        }
    }

    const bool bDeferring = IsDeadGroundActive();
    for (int32 I = Wires.Num() - 1; I >= 0; --I)
    {
        ADMAbilityMarker* Wire = Wires[I].Get();
        if (!IsValid(Wire)) { Wires.RemoveAt(I); continue; }
        if (Tick < Wire->ArmedTick) { continue; }
        // A wire is spent by its first crossing (repeat triggers belong to the Resetting Fuse node), so under
        // Dead Ground it tags only the first crosser too and stays visibly armed until the batch resolves.
        if (bDeferring && Ledger.HasTag(FDMDeadGroundLedger::Wire, Wire->Serial)) { continue; }
        for (ADMCombatant* Enemy : M->GetCombatants())
        {
            if (!IsValid(Enemy) || !Enemy->bIsEnemy || Enemy->IsDown()) { continue; }
            const FVector* Last = LastPositions.Find(Enemy);
            if (!Last) { continue; }
            if (!DMKitRules::CrossesWire(Wire->GetActorLocation(), Wire->WireEnd, *Last, Enemy->GetActorLocation(), WireSlack)) { continue; }
            if (bDeferring)
            {
                if (TagTrap(FDMDeadGroundLedger::Wire, Wire->Serial, Enemy)) { Wire->CustomLabel = TEXT("TAGGED"); Wire->ForceNetUpdate(); }
                break;
            }
            TriggerWire(Wire, Enemy);
            Wire->Destroy(); Wires.RemoveAt(I);
            break;
        }
    }

    if (Ledger.Due(Tick)) { ResolveDeadGround(); }
}

void UDMKitComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDMKitComponent, CooldownW); DOREPLIFETIME(UDMKitComponent, CooldownE); DOREPLIFETIME(UDMKitComponent, CooldownR);
    DOREPLIFETIME(UDMKitComponent, RActiveUntilTick); DOREPLIFETIME(UDMKitComponent, BracedUntilTick); DOREPLIFETIME(UDMKitComponent, ChargeUntilTick);
    DOREPLIFETIME(UDMKitComponent, PendingWireStart); DOREPLIFETIME(UDMKitComponent, bWirePending);
    DOREPLIFETIME(UDMKitComponent, Zones); DOREPLIFETIME(UDMKitComponent, Wires);
}
