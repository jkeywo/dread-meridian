#include "DMMadnessComponent.h"
#include "DMCombatant.h"
#include "DMRelicComponent.h"
#include "DMCombatGameMode.h"
#include "DMCombatPlayerController.h"
bool UDMMadnessComponent::Authority() const
{ const auto* A = Cast<ADMCombatant>(GetOwner()); return A && A->HasAuthority() && !A->bIsEnemy; }
int32 UDMMadnessComponent::Now() const
{ auto* M = GetWorld() ? GetWorld()->GetAuthGameMode<ADMCombatGameMode>() : nullptr; return M ? M->GetCombatTick() : 0; }
void UDMMadnessComponent::Reset()
{
    if (!Authority()) { return; }
    NextPerceptionUse = NextPerceptionHazard = PerceptionSerial = 0;
    Echoes.Reset();
    Family = EDMMadnessFamily::None; Cues.Reset(); NextFamilyTick = NextIgnoreTick = 0; bFamilyCrisis = false;
    ResonanceCues.Reset();
    Settings.Sanitize(); State = FDMMadnessState(); GroundUntil = SymptomUntil = 0;
    SymptomId.Reset(); SymptomText.Reset(); LastSent.Reset(); LastOwner.Reset();
    CastChecked<ADMCombatant>(GetOwner())->Investigator->Madness = 0; Deliver();
}
FDMMadnessView UDMMadnessComponent::View() const
{
    FDMMadnessView V;
    if (!Authority()) { return V; }
    V.Family = Family; V.Cues = Cues; V.Cues.Append(ResonanceCues);
    V.EntityId = CastChecked<ADMCombatant>(GetOwner())->EntityId;
    V.Current = State.Current; V.Floor = State.Floor; V.Band = State.Band(Settings);
    V.CrisisUntil = State.CrisisUntil; V.GroundUntil = GroundUntil;
    V.SymptomId = SymptomId; V.SymptomText = SymptomText; V.SymptomUntil = SymptomUntil; return V;
}
void UDMMadnessComponent::SetResonanceCues(const TArray<FDMMadnessCue>& Value)
{ if (Authority()) { ResonanceCues = Value; Deliver(); } }
void UDMMadnessComponent::Deliver()
{
    if (!Authority()) { return; }
    auto* P = Cast<ADMCombatPlayerController>(CastChecked<ADMCombatant>(GetOwner())->GetController());
    const auto V = View(); const auto Summary = V.Summary();
    if (P && (P != LastOwner.Get() || Summary != LastSent))
    { P->ClientMadness(V); LastSent = Summary; }
    LastOwner = P;
}
void UDMMadnessComponent::Publish(const FDMMadnessState& Before, const FString& Reason)
{
    if (State.Band(Settings)>Before.Band(Settings)) { CastChecked<ADMCombatant>(GetOwner())->Relics->TierEntered.Broadcast(Before.Band(Settings),State.Band(Settings)); }
    CastChecked<ADMCombatant>(GetOwner())->Investigator->Madness = State.Current;
    if (auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    {
        auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"), View().EntityId);
        D->SetStringField(TEXT("reason"), Reason); D->SetNumberField(TEXT("before"), Before.Current);
        D->SetNumberField(TEXT("current"), State.Current); D->SetNumberField(TEXT("floor"), State.Floor);
        D->SetNumberField(TEXT("band_before"), Before.Band(Settings)); D->SetNumberField(TEXT("band"), State.Band(Settings));
        D->SetNumberField(TEXT("crisis_until"), State.CrisisUntil);
        M->Emit(TEXT("madness.changed"), D);
        if (Before.Band(Settings) != State.Band(Settings)) { M->Emit(TEXT("madness.threshold"), D); }
        if (!!Before.CrisisUntil != !!State.CrisisUntil)
        { M->Emit(State.CrisisUntil ? TEXT("madness.crisis_started") : TEXT("madness.crisis_ended"), D); }
    }
    Deliver();
}
bool UDMMadnessComponent::Add(float Amount, const FString& Reason)
{
    if (!Authority()) { return false; }
    const auto Before = State; if (!State.Add(Amount, Now(), Settings)) { return false; }
    Publish(Before, Reason); return true;
}
bool UDMMadnessComponent::Recover(float Amount, const FString& Reason)
{
    if (!Authority()) { return false; }
    const auto Before = State; if (!State.Recover(Amount)) { return false; }
    Publish(Before, Reason); return true;
}
bool UDMMadnessComponent::RaiseFloor(float Value, const FString& Reason)
{
    if (!Authority()) { return false; }
    const auto Before = State; if (!State.RaiseFloor(Value, Now(), Settings)) { return false; }
    Publish(Before, Reason); return true;
}
bool UDMMadnessComponent::BeginGrounding()
{
    if (!Authority()) { return false; }
    auto* A = CastChecked<ADMCombatant>(GetOwner()); auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!M || !M->IsCombatActive() || A->IsDown() || A->IsStunned() || A->IsRestrained()
        || A->Primary->FrameTarget || A->Kit->IsCharging()
        || State.Current <= State.Floor || GroundUntil) { return false; }
    GroundPosition = A->GetActorLocation(); GroundUntil = Now() + Settings.GroundTicks; Deliver(); return true;
}
void UDMMadnessComponent::InterruptGrounding()
{ if (Authority() && GroundUntil) { GroundUntil = 0; Deliver(); } }
void UDMMadnessComponent::Step(int32 Tick)
{
    if (!Authority()) { return; }
    const auto Before = State; State.Step(Tick, Settings);
    if (Before.Current != State.Current || Before.CrisisUntil != State.CrisisUntil) { Publish(Before, TEXT("crisis_clock")); }
    auto* A = CastChecked<ADMCombatant>(GetOwner());
    if (GroundUntil && (A->IsDown() || A->IsStunned() || A->IsRestrained()
        || FVector::DistSquared2D(GroundPosition, A->GetActorLocation()) > FMath::Square(10.f))) { InterruptGrounding(); }
    if (GroundUntil && Tick >= GroundUntil)
    { GroundUntil = 0; Recover(Settings.GroundRecovery, TEXT("grounding")); }
    StepFamily(Tick);
    if (SymptomUntil && Tick >= SymptomUntil) { SymptomUntil = 0; SymptomId.Reset(); SymptomText.Reset(); }
    Deliver();
}
bool UDMMadnessComponent::Manifest(const FString& Id, const FString& Text, int32 RequiredBand, int32 DurationTicks)
{
    if (!Authority() || Id.IsEmpty() || Text.IsEmpty() || RequiredBand < 1 || RequiredBand > 3
        || State.Band(Settings) < RequiredBand || DurationTicks < 1 || SymptomUntil > Now()) { return false; }
    SymptomId = Id; SymptomText = Text; SymptomUntil = Now() + FMath::Min(DurationTicks, 10000);
    if (auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    {
        auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"), View().EntityId);
        D->SetStringField(TEXT("symptom_id"), Id); D->SetNumberField(TEXT("until_tick"), SymptomUntil);
        M->Emit(TEXT("madness.symptom"), D);
    }
    Deliver(); return true;
}
