#include "DMMadnessComponent.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"

void UDMMadnessComponent::AssignFamily(EDMMadnessFamily Value)
{
    if (!Authority() || Family != EDMMadnessFamily::None || static_cast<int32>(Value) < 1
        || static_cast<int32>(Value) > SupportedFamilies) { return; }
    Family = Value; FamilyEvent(TEXT("assigned")); Deliver();
}
void UDMMadnessComponent::FamilyEvent(const FString& Action, const FString& Target)
{
    if (auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    {
        auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"), View().EntityId);
        D->SetStringField(TEXT("family"), StaticEnum<EDMMadnessFamily>()->GetNameStringByValue(static_cast<int64>(Family)));
        D->SetStringField(TEXT("action"), Action); D->SetStringField(TEXT("target_id"), Target);
        M->Emit(TEXT("madness.family"), D);
    }
}
TArray<ADMCombatant*> UDMMadnessComponent::Candidates() const
{
    TArray<ADMCombatant*> Out; auto* A = CastChecked<ADMCombatant>(GetOwner());
    if (auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    { for (ADMCombatant* T : M->GetCombatants())
      { if (IsValid(T) && T->bIsEnemy && !T->IsDown() && FVector::Dist2D(A->GetActorLocation(), T->GetActorLocation()) <= Settings.FamilyRange
            && A->Primary->Sight(T->GetActorLocation(), T)) { Out.Add(T); } } }
    Out.Sort([](const ADMCombatant& A, const ADMCombatant& B) { return A.EntityId < B.EntityId; }); return Out;
}
void UDMMadnessComponent::ResolveCrisis(const FString& Reason)
{
    if (!Authority() || !State.CrisisUntil) { return; }
    const auto Before = State; State.CrisisUntil = 0; State.Recover(Settings.CrisisRecovery);
    State.RestUntil = Now() + Settings.CrisisRestTicks; Publish(Before, Reason);
}
void UDMMadnessComponent::StepFamily(int32 Tick)
{
    auto* A = CastChecked<ADMCombatant>(GetOwner()); auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!M || Family == EDMMadnessFamily::None) { return; }
    if (A->IsDown()) { Cues.Reset(); NextFamilyTick = Tick; return; }
    if (bFamilyCrisis != !!State.CrisisUntil)
    { bFamilyCrisis = !!State.CrisisUntil; Cues.Reset(); NextFamilyTick = Tick; }
    if (State.Band(Settings) == 0 && !State.CrisisUntil) { Cues.Reset(); return; }
    if (Family == EDMMadnessFamily::Obsession)
    {
        if (Cues.Num())
        {
            auto* T = M->FindCombatant(Cues[0].TargetId);
            if (!T || T->IsDown()) { Cues.Reset(); NextFamilyTick = Tick; }
            else
            {
                // Stop publishing moving target positions when it leaves this investigator's sight.
                if (A->Primary->Sight(T->GetActorLocation(), T)) { Cues[0].Location = T->GetActorLocation(); }
                if (Tick >= NextIgnoreTick) { Add(Settings.IgnorePressure, TEXT("ignored_fixation")); NextIgnoreTick = Tick + Settings.IgnoreTicks; }
                if (Tick >= Cues[0].Until) { FamilyEvent(TEXT("fixation_expired"), T->EntityId); Cues.Reset(); NextFamilyTick = Tick + 10; }
            }
        }
        if (!Cues.Num() && Tick >= NextFamilyTick)
        {
            const auto Targets = Candidates(); NextFamilyTick = Tick + 10;
            if (Targets.Num())
            {
                auto* T = Targets[M->DrawRandom(EDMRandomStream::Madness) % Targets.Num()];
                FDMMadnessCue C; C.Id = TEXT("fixation"); C.Label = State.CrisisUntil ? TEXT("Overwhelming fixation") : TEXT("Fixation");
                C.TargetId = T->EntityId; C.Location = T->GetActorLocation(); C.Goal = State.CrisisUntil ? 5 : 3;
                C.Until = Tick + Settings.FamilyInterval; Cues.Add(C); NextIgnoreTick = Tick + Settings.IgnoreTicks;
                FamilyEvent(TEXT("fixation_started"), T->EntityId);
            }
        }
    }
}
float UDMMadnessComponent::Outgoing(ADMCombatant* Target) const
{
    if (Authority() && Family == EDMMadnessFamily::Obsession && State.CrisisUntil && Cues.Num()
        && Target && Cues[0].TargetId == Target->EntityId) { return 1 + .1f * FMath::Min(Cues[0].Progress, 5); }
    return 1;
}
void UDMMadnessComponent::OnDamage(ADMCombatant* Target, float HealthLoss)
{
    if (!Authority() || !Target || HealthLoss <= 0 || Family != EDMMadnessFamily::Obsession || !Cues.Num()
        || Cues[0].TargetId != Target->EntityId) { return; }
    ++Cues[0].Progress; NextIgnoreTick = Now() + Settings.IgnoreTicks;
    Recover(Settings.IndulgeRecovery, TEXT("acted_on_fixation"));
    if (Target->IsDown() || Cues[0].Progress >= Cues[0].Goal)
    { FamilyEvent(TEXT("fixation_resolved"), Target->EntityId); Cues.Reset(); NextFamilyTick = Now() + Settings.FamilyInterval; ResolveCrisis(TEXT("fixation_resolved")); }
    Deliver();
}
