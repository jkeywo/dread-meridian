#include "DMMadnessComponent.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"

void UDMMadnessComponent::ScheduleEcho(uint8 Slot, ADMCombatant* Target, FVector Point, float Strength, FVector WireStart)
{
    if (!Authority() || Family != EDMMadnessFamily::Dissociation || Slot > 2 || (State.Band(Settings) == 0 && !State.CrisisUntil) || Echoes.Num() >= 8) { return; }
    // Predictable pools: early Q, mid Q/W, high and Crisis Q/W/E. No random hidden proc rate.
    if (!State.CrisisUntil && Slot >= State.Band(Settings)) { return; }
    auto* A = CastChecked<ADMCombatant>(GetOwner());
    FEcho E; E.Slot = Slot; E.Kind = static_cast<uint8>(A->Investigator->Kind);
    E.Origin = A->GetActorLocation(); E.Point = Target ? Target->GetActorLocation() : Point; E.WireStart = WireStart;
    E.Strength = FMath::IsFinite(Strength) ? FMath::Max(0.f, Strength) : 0;
    E.Due = Now() + 20; E.bAnchored = State.Band(Settings) >= 3 || State.CrisisUntil;
    Echoes.Add(E); FamilyEvent(TEXT("echo_scheduled")); StepEchoes(Now()); Deliver();
}
void UDMMadnessComponent::StepEchoes(int32 Tick)
{
    auto* A = CastChecked<ADMCombatant>(GetOwner());
    for (int32 I=0; I<Echoes.Num();)
    {
        if (Tick < Echoes[I].Due) { ++I; continue; }
        const auto E = Echoes[I]; Echoes.RemoveAt(I); ResolveEcho(E, Tick);
    }
    Cues.Reset();
    for (int32 I=0; I<Echoes.Num(); ++I)
    {
        const auto& E = Echoes[I]; FDMMadnessCue C; C.Id = FString::Printf(TEXT("echo.%d.%d"), E.Due,I);
        C.Location = E.Point + (E.bAnchored ? FVector::ZeroVector : A->GetActorLocation()-E.Origin);
        C.Label = FString::Printf(TEXT("%s echo in %.1fs"), E.Slot==0 ? TEXT("Q") : E.Slot==1 ? TEXT("W") : TEXT("E"), FMath::Max(0,E.Due-Tick)*.1f);
        C.Until = E.Due; Cues.Add(C);
    }
}
void UDMMadnessComponent::ResolveEcho(const FEcho& E, int32 Tick)
{
    auto* A = CastChecked<ADMCombatant>(GetOwner()); auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!M || !M->IsCombatActive() || A->IsDown()) { return; }
    const FVector Shift = E.bAnchored ? FVector::ZeroVector : A->GetActorLocation()-E.Origin;
    const FVector Origin = E.Origin+Shift, Point = E.Point+Shift;
    const FVector Direction = (Point-Origin).GetSafeNormal2D();
    const auto Kind = static_cast<EDMInvestigator>(E.Kind);
    const FString Id = FString::Printf(TEXT("ability.echo.%d.%d"), E.Kind, E.Slot);
    // Authored weak signatures, not recursive ability activation: no second resource spend,
    // cooldown reset, movement of the caster, persistent summon or echo of an echo.
    for (ADMCombatant* T : M->GetCombatants())
    {
        if (!T || T->IsDown()) { continue; }
        FDMControl Control; float Damage = 0, Shield = 0, Exposure = 0;
        FVector Centre = Point; float Radius = 120; bool bInside = false;
        if (Kind == EDMInvestigator::Sapper)
        {
            if (E.Slot==0) { Radius=150; Damage=19.25f; }
            if (E.Slot==1) { bInside=DMKitRules::PointInCone(Origin,Direction,25,600,T->GetActorLocation()); Damage=1.4f; Control.Slow=.15f; Control.SlowTicks=4; }
            if (E.Slot==2) { bInside=DMKitRules::DistanceToSegment2D(E.WireStart+Shift,Point,T->GetActorLocation())<=35; Damage=5.25f; Control.Slow=.2f; Control.SlowTicks=7; Control.StaggerTicks=7; }
        }
        else if (Kind == EDMInvestigator::Photographer)
        {
            if (E.Slot==0) { Radius=60; Exposure=8; }
            if (E.Slot==1) { bInside=DMKitRules::PointInCone(Origin,Direction,35,350,T->GetActorLocation()); Exposure=8.75f; Control.Slow=.175f; Control.SlowTicks=4; Control.StaggerTicks=3; }
            if (E.Slot==2) { Radius=60; Damage=E.Strength*.7f*.35f; }
        }
        else if (Kind == EDMInvestigator::Medium)
        {
            Shield=E.Slot==0 ? 2.f : 3.f; Control.Slow=.14f; Control.SlowTicks=5;
            if (E.Slot==2) { Centre=Origin; Radius=200; Damage=7; }
        }
        else if (Kind == EDMInvestigator::Smuggler)
        {
            Centre=Origin; Radius=200;
            if (E.Slot==0) { Control.Displacement=Direction*80; Control.StaggerTicks=2; }
            if (E.Slot==1) { bInside=DMKitRules::DistanceToSegment2D(Origin,Point,T->GetActorLocation())<=50; Damage=5.25f; Control.Displacement=Direction*60; }
            if (E.Slot==2) { Damage=3.5f; Control.Displacement=(T->GetActorLocation()-Origin).GetSafeNormal2D()*70; }
        }
        else { continue; }
        const bool bCustomShape = (Kind==EDMInvestigator::Sapper && E.Slot>0)
            || (Kind==EDMInvestigator::Photographer && E.Slot==1) || (Kind==EDMInvestigator::Smuggler && E.Slot==1);
        if (!bCustomShape) { bInside=FVector::Dist2D(Centre,T->GetActorLocation())<=Radius; }
        if (!bInside || !A->Smuggler->Sight(Origin,T->GetActorLocation())) { continue; }
        if (T->bIsEnemy)
        {
            if (Damage>0) { A->DealCombatDamage(T,Damage,Id); }
            if (Exposure>0) { A->Investigator->AddExposure(T->EntityId,Exposure,false,Tick); A->RecordResources(TEXT("echo_exposure")); }
            if (Control.Slow>0 || Control.StaggerTicks>0 || !Control.Displacement.IsNearlyZero()) { T->ApplyControl(Control,A,Id); }
        }
        else if (Shield>0) { T->AddShield(Shield); }
    }
    FamilyEvent(TEXT("echo_resolved"));
}
