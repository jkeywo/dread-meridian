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
    if (A->IsDown()) { Echoes.Reset(); Cues.Reset(); NextFamilyTick = Tick; return; }
    if (Family == EDMMadnessFamily::Dissociation) { StepEchoes(Tick); return; }
    if (bFamilyCrisis != !!State.CrisisUntil)
    { bFamilyCrisis = !!State.CrisisUntil; Cues.Reset(); NextFamilyTick = Tick; }
    if (State.Band(Settings) == 0 && !State.CrisisUntil) { Cues.Reset(); return; }
    if (Family == EDMMadnessFamily::Perception) { StepPerception(Tick); return; }
    if (Family == EDMMadnessFamily::Compulsion) { StepCompulsion(Tick); return; }
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
    if (Authority() && Target && HealthLoss > 0 && Family == EDMMadnessFamily::Compulsion)
    {
        for (int32 I = Cues.Num()-1; I >= 0; --I) { if (Cues[I].TargetId == Target->EntityId) { Indulge(I); } }
        return;
    }
    if (!Authority() || !Target || HealthLoss <= 0 || Family != EDMMadnessFamily::Obsession || !Cues.Num()
        || Cues[0].TargetId != Target->EntityId) { return; }
    ++Cues[0].Progress; NextIgnoreTick = Now() + Settings.IgnoreTicks;
    Recover(Settings.IndulgeRecovery, TEXT("acted_on_fixation"));
    if (Target->IsDown() || Cues[0].Progress >= Cues[0].Goal)
    { FamilyEvent(TEXT("fixation_resolved"), Target->EntityId); Cues.Reset(); NextFamilyTick = Now() + Settings.FamilyInterval; ResolveCrisis(TEXT("fixation_resolved")); }
    Deliver();
}

void UDMMadnessComponent::Indulge(int32 Index)
{
    if (!Cues.IsValidIndex(Index)) { return; }
    const FString Target = Cues[Index].TargetId; Cues.RemoveAt(Index);
    Recover(Settings.IndulgeRecovery * 2, TEXT("indulged_urge"));
    if (State.Band(Settings) >= 3 || State.CrisisUntil) { CastChecked<ADMCombatant>(GetOwner())->AddShield(5); }
    NextFamilyTick = Now() + Settings.FamilyInterval;
    FamilyEvent(TEXT("urge_indulged"), Target); Deliver();
}
void UDMMadnessComponent::StepCompulsion(int32 Tick)
{
    auto* A = CastChecked<ADMCombatant>(GetOwner()); auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    for (int32 I=Cues.Num()-1; I>=0; --I)
    {
        auto& C = Cues[I];
        if (C.TargetId.IsEmpty())
        {
            if (FVector::Dist2D(A->GetActorLocation(), C.Location) <= 65) { Indulge(I); continue; }
        }
        else
        {
            auto* T = M->FindCombatant(C.TargetId);
            if (!T || T->IsDown()) { Cues.RemoveAt(I); continue; }
            if (A->Primary->Sight(T->GetActorLocation(), T)) { C.Location = T->GetActorLocation(); }
        }
        if (Tick >= C.Until)
        { FamilyEvent(TEXT("urge_resisted"), C.TargetId); Cues.RemoveAt(I); Add(Settings.IgnorePressure, TEXT("resisted_urge")); NextFamilyTick = Tick + 10; }
    }
    if (Cues.Num() || Tick < NextFamilyTick) { return; }
    NextFamilyTick = Tick + Settings.FamilyInterval;
    auto Targets = Candidates();
    const int32 Count = State.CrisisUntil ? 3 : 1;
    for (int32 I=0; I<Count; ++I)
    {
        FDMMadnessCue C; C.Id = FString::Printf(TEXT("urge.%d.%d"), Tick, I); C.Until = Tick + Settings.FamilyInterval;
        if (I==0 && Targets.Num())
        {
            auto* T = Targets[M->DrawRandom(EDMRandomStream::Madness) % Targets.Num()];
            C.TargetId = T->EntityId; C.Location = T->GetActorLocation(); C.Label = TEXT("Urge: strike");
        }
        else
        {
            const float Angle = (M->DrawRandom(EDMRandomStream::Madness) % 360) * PI / 180;
            const FVector Offset(FMath::Cos(Angle)*220, FMath::Sin(Angle)*220, 0);
            if (!A->Primary->Ground(A->GetActorLocation()+Offset, C.Location) || !A->Primary->Sight(C.Location)) { continue; }
            C.Label = TEXT("Urge: stand here");
        }
        Cues.Add(C); FamilyEvent(TEXT("urge_started"), C.TargetId);
    }
}
