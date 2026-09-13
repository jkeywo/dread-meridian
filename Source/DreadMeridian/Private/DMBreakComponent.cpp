#include "DMBreakComponent.h"
#include "DMCombatant.h"
#include "DMRelicComponent.h"
#include "DMCombatGameMode.h"
#include "Net/UnrealNetwork.h"

UDMBreakComponent::UDMBreakComponent() { SetIsReplicatedByDefault(true); }
ADMCombatant* UDMBreakComponent::Self() const { return CastChecked<ADMCombatant>(GetOwner()); }
bool UDMBreakComponent::IsProtected() const { return Self()->bIsEnemy && !Self()->bCommonEnemy; }

void UDMBreakComponent::Project(int32 Tick)
{
    const FString Before = ReplicationSummary();
    const bool bEnabled = IsProtected() && !Self()->IsDown();
    MaxResolve = bEnabled ? Meter.Settings.MaxResolve : 0;
    CurrentResolve = bEnabled ? FMath::Max(0.f, MaxResolve - Meter.Value) : 0;
    Self()->Break = bEnabled ? Meter.Value : 0;
    Self()->bBreakVulnerable = bEnabled && Meter.IsBroken(Tick);
    Self()->BrokenUntilTick = Self()->bBreakVulnerable ? Meter.BrokenUntil : 0;
    bResisting = bEnabled && Meter.IsResisting(Tick);
    ResistUntilTick = bResisting ? Meter.ResistUntil : 0;
    if (Before != ReplicationSummary()) { Self()->ForceNetUpdate(); }
}

FString UDMBreakComponent::ReplicationSummary() const
{
    return FString::Printf(TEXT("resolve=%.2f/%.2f broken=%d until=%d resisting=%d until=%d interrupt=%d stun=%d stagger=%d"),
        CurrentResolve, MaxResolve, Self()->bBreakVulnerable ? 1 : 0, Self()->BrokenUntilTick,
        bResisting ? 1 : 0, ResistUntilTick, InterruptUntilTick, Self()->StunnedUntilTick, Self()->StaggeredUntilTick);
}

void UDMBreakComponent::Reset()
{
    if (!Self()->HasAuthority()) { return; }
    Settings.Sanitize(); Meter = FDMBreakMeter(); Meter.Settings = Settings;
    InterruptUntilTick = 0; Project(0);
}

void UDMBreakComponent::Step(int32 Tick)
{
    if (!Self()->HasAuthority()) { return; }
    if (!IsProtected() || Self()->IsDown()) { Reset(); return; }
    const bool bRecovered = Meter.Step(Tick);
    const bool bResistanceEnded = bResisting && !Meter.IsResisting(Tick);
    const bool bInterruptEnded = InterruptUntilTick > 0 && Tick >= InterruptUntilTick;
    if (bInterruptEnded) { InterruptUntilTick = 0; }
    Project(Tick);
    if (bRecovered)
    {
        // A hold accepted late in Broken cannot extend the vulnerability window.
        if (Self()->HeldBy) { Self()->HeldBy->Primary->ReleaseClinch(); }
        Record(TEXT("break.recovered"));
    }
    if (bResistanceEnded) { Record(TEXT("break.resistance_ended")); }
    if (bInterruptEnded) { Record(TEXT("break.interrupt_window_closed")); }
}

void UDMBreakComponent::AddPressure(float Amount, ADMCombatant* Source, const FString& AbilityId)
{
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Self()->HasAuthority() || !Mode || !Mode->IsCombatActive() || !IsProtected() || Self()->IsDown()
        || !FMath::IsFinite(Amount) || Amount <= 0) { return; }
    const int32 Tick = Mode->GetCombatTick(); Step(Tick);
    if (Source) { Amount*=Source->Relics->BreakMultiplierAgainst(Self()); }
    const float Before = Meter.Value;
    const bool bBroke = Meter.Add(Amount, Tick);
    Project(Tick);
    if (Meter.Value > Before)
    { Self()->Relics->AcceptedBreak.Broadcast(Source,Meter.Value-Before,bBroke); Record(TEXT("break.pressure"), Source, AbilityId, Meter.Value - Before); }
    if (bBroke) { Record(TEXT("break.broken"), Source, AbilityId); }
}

void UDMBreakComponent::OpenInterruptWindow(int32 DurationTicks)
{
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Self()->HasAuthority() || !Mode || !Mode->IsCombatActive() || !IsProtected() || Self()->IsDown() || DurationTicks <= 0) { return; }
    Step(Mode->GetCombatTick());
    const int32 Until = Mode->GetCombatTick() + FMath::Min(DurationTicks, 1000000);
    if (Until <= InterruptUntilTick) { return; }
    InterruptUntilTick = Until; Self()->ForceNetUpdate(); Record(TEXT("break.interrupt_window_opened"));
}

void UDMBreakComponent::Record(const TCHAR* Event, ADMCombatant* Source, const FString& AbilityId, float Applied)
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    {
        auto Data = MakeShared<FJsonObject>(); Data->SetStringField(TEXT("entity_id"), Self()->EntityId);
        Data->SetNumberField(TEXT("value"), Self()->Break);
        Data->SetNumberField(TEXT("current_resolve"), CurrentResolve); Data->SetNumberField(TEXT("max_resolve"), MaxResolve);
        Data->SetNumberField(TEXT("broken_until_tick"), Self()->BrokenUntilTick);
        Data->SetNumberField(TEXT("resist_until_tick"), ResistUntilTick);
        Data->SetNumberField(TEXT("interrupt_until_tick"), InterruptUntilTick);
        if (Source) { Data->SetStringField(TEXT("source_id"), Source->EntityId); }
        if (!AbilityId.IsEmpty()) { Data->SetStringField(TEXT("ability_id"), AbilityId); }
        if (Applied > 0) { Data->SetNumberField(TEXT("applied_pressure"), Applied); }
        Mode->Emit(Event, Data);
    }
}

void UDMBreakComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDMBreakComponent, CurrentResolve); DOREPLIFETIME(UDMBreakComponent, MaxResolve);
    DOREPLIFETIME(UDMBreakComponent, bResisting); DOREPLIFETIME(UDMBreakComponent, ResistUntilTick);
    DOREPLIFETIME(UDMBreakComponent, InterruptUntilTick);
}
