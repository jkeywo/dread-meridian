#include "DMInjuryComponent.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMGameState.h"
#include "Net/UnrealNetwork.h"

UDMInjuryComponent::UDMInjuryComponent() { SetIsReplicatedByDefault(true); }
ADMCombatant* UDMInjuryComponent::Self() const { return CastChecked<ADMCombatant>(GetOwner()); }
int32 UDMInjuryComponent::Now() const
{
    if (auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>()) { return M->GetCombatTick(); }
    const auto* G = GetWorld()->GetGameState<ADMGameState>(); return G ? G->GetCombatTick() : 0;
}
bool UDMInjuryComponent::CanCast() const { return Now() >= CastUntilTick; }
bool UDMInjuryComponent::CanAttack() const { return Now() >= AttackUntilTick; }
void UDMInjuryComponent::Reset()
{ if (!Self()->HasAuthority()) { return; } Settings.Sanitize(); State = FDMInjuryState(); bHasPosition = false; HealingPulses = 0; Project(); }
void UDMInjuryComponent::Project()
{
    Specific = State.Specific; Self()->InjuryCount = Specific.Num(); Self()->GrievousCount = State.Grievous;
    CastUntilTick = State.Has(EDMInjury::Concussion) && State.CastUntil > Now() ? State.CastUntil : 0;
    AttackUntilTick = State.Has(EDMInjury::WoundedArm) && State.AttackUntil > Now() ? State.AttackUntil : 0;
    MovementFactor = State.Has(EDMInjury::TwistedKnee) && State.LimpUntil > Now() ? 1 - Settings.LimpFraction : 1;
    HealingFactor = State.Has(EDMInjury::DeepCut) && State.RecoveryUntil > Now() ? Settings.DeepCutHealingFactor : 1;
    Self()->ForceNetUpdate();
}
void UDMInjuryComponent::Step(int32 Tick)
{
    if (!Self()->HasAuthority() || Self()->bIsEnemy) { return; }
    State.RecentLoss(Tick, Settings.WindowTicks);
    if (!Self()->IsDown() && bHasPosition) { State.Move(FVector::Dist2D(LastPosition, Self()->GetActorLocation()), Tick, Settings); }
    LastPosition = Self()->GetActorLocation(); bHasPosition = true; Project();
    if (Self()->IsDown()) { HealingPulses = 0; }
    if (HealingPulses > 0 && Tick >= NextHealingTick)
    { --HealingPulses; NextHealingTick = Tick + Settings.FoodIntervalTicks; Self()->HealHealth(Settings.FoodPulseHealth); }
}
void UDMInjuryComponent::StartFoodHealing()
{ if (Self()->HasAuthority()) { HealingPulses = Settings.FoodPulses; NextHealingTick = Now() + Settings.FoodIntervalTicks; Self()->ForceNetUpdate(); } }
float UDMInjuryComponent::Incoming(float Unshielded, bool bHazard) const
{ return Self()->bIsEnemy ? 1 : State.Incoming(Unshielded, Self()->MaxHealth(), Now(), bHazard, Settings); }
void UDMInjuryComponent::RecordLoss(float Loss, bool bDown, bool bHazard, ADMCombatant* Source, const FString& AbilityId)
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Self()->HasAuthority() || Self()->bIsEnemy || !M || !M->IsCombatActive()) { return; }
    if (bDown) { HealingPulses = 0; }
    const float Recent = State.RecentLoss(Now(), Settings.WindowTicks) + Loss;
    if (State.RecordLoss(Loss, Self()->MaxHealth(), Now(), bDown, bHazard, Settings))
    {
        const uint32 Draw = State.Specific.Num() < FDMInjuryState::Capacity ? M->DrawRandom(EDMRandomStream::Injury) : 0;
        const EDMInjury Kind = State.Gain(Draw); Project();
        auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"), Self()->EntityId);
        D->SetStringField(TEXT("injury_id"), DMInjuryRules::Id(Kind)); D->SetStringField(TEXT("ability_id"), AbilityId);
        if (Source) { D->SetStringField(TEXT("source_id"), Source->EntityId); }
        D->SetNumberField(TEXT("recent_health_loss"), Recent); D->SetBoolField(TEXT("downed"), bDown);
        D->SetNumberField(TEXT("grievous"), State.Grievous); D->SetStringField(TEXT("effect"), DMInjuryRules::Effect(Kind));
        M->Emit(TEXT("injury.gained"), D);
    }
    Project();
}
void UDMInjuryComponent::OnCast() { if (Self()->HasAuthority() && !Self()->bIsEnemy) { State.Cast(Now(), Settings); Project(); } }
void UDMInjuryComponent::OnAttack() { if (Self()->HasAuthority() && !Self()->bIsEnemy) { State.Attack(Now(), Settings); Project(); } }
void UDMInjuryComponent::OnDisplacement(float Distance)
{
    if (!Self()->HasAuthority() || Self()->bIsEnemy || Distance <= 0 || !State.Has(EDMInjury::TwistedKnee)) { return; }
    State.LimpUntil = Now() + Settings.LimpTicks; LastPosition = Self()->GetActorLocation(); bHasPosition = true; Project();
}
bool UDMInjuryComponent::Treat()
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Self()->HasAuthority() || !M || !M->IsCombatActive() || Self()->bIsEnemy || Self()->IsDown()) { return false; }
    const EDMInjury Removed = State.Grievous > 0 ? EDMInjury::Count : State.Specific.Num() ? State.Specific[0] : EDMInjury::Count;
    if (!State.Treat()) { return false; }
    Project(); auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"), Self()->EntityId);
    D->SetStringField(TEXT("injury_id"), DMInjuryRules::Id(Removed)); D->SetNumberField(TEXT("grievous"), State.Grievous);
    M->Emit(TEXT("injury.treated"), D); return true;
}
FString UDMInjuryComponent::Summary() const
{
    FString Text; for (auto K : Specific) { Text += FString(DMInjuryRules::Name(K)) + TEXT("; "); }
    return Text + FString::Printf(TEXT("Grievous %d | cast=%d attack=%d move=%.2f heal=%.2f"), Self()->GrievousCount, CastUntilTick, AttackUntilTick, MovementFactor, HealingFactor);
}
void UDMInjuryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDMInjuryComponent, Specific); DOREPLIFETIME(UDMInjuryComponent, CastUntilTick);
    DOREPLIFETIME(UDMInjuryComponent, AttackUntilTick); DOREPLIFETIME(UDMInjuryComponent, MovementFactor); DOREPLIFETIME(UDMInjuryComponent, HealingFactor);
    DOREPLIFETIME(UDMInjuryComponent, HealingPulses);
}
