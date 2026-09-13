#include "DMMadnessComponent.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"

void UDMMadnessComponent::StepPerception(int32 Tick)
{
    auto* A = CastChecked<ADMCombatant>(GetOwner()); auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    Cues.RemoveAll([Tick](const FDMMadnessCue& C) { return Tick >= C.Until; });
    const int32 Band = State.CrisisUntil ? 3 : State.Band(Settings);
    // A tier change replaces harmless early anomalies with the newly unlocked contextual pool.
    if (Band >= 2 && Cues.Num() && Cues[0].Id.StartsWith(TEXT("perception.anomaly"))) { Cues.Reset(); NextFamilyTick=Tick; }
    if (!Cues.Num() && Tick >= NextFamilyTick)
    {
        const int32 Count = State.CrisisUntil ? 8 : Band >= 3 ? 4 : Band >= 2 ? 3 : 1;
        for (int32 I=0; I<Count; ++I)
        {
            FVector Ground; bool bFound=false;
            for (int32 Attempt=0; Attempt<12; ++Attempt)
            {
                const float Angle = (M->DrawRandom(EDMRandomStream::Madness)%360)*PI/180;
                const float Radius = 160 + M->DrawRandom(EDMRandomStream::Madness)%220;
                const FVector Point = A->GetActorLocation()+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
                if (!A->Primary->Ground(Point,Ground) || !A->Primary->Sight(Ground)) { continue; }
                bFound = !Cues.ContainsByPredicate([&](const auto& C) { return FVector::Dist2D(C.Location,Ground)<80; });
                if (bFound) { break; }
            }
            if (!bFound) { continue; }
            const FString Kind = Band<2 ? TEXT("anomaly") : I%2 ? TEXT("hazard") : TEXT("wraith");
            FDMMadnessCue C; C.Id=FString::Printf(TEXT("perception.%s.%d.%d"),*Kind,Tick,++PerceptionSerial);
            C.Location=Ground; C.Goal=Band<2 ? 1 : 2; C.Until=Tick+Settings.FamilyInterval*2;
            C.Label=Band<2 ? TEXT("Impossible detail [J]") : I%2 ? TEXT("Private hazard [J]") : TEXT("Wraith [J]");
            Cues.Add(C); FamilyEvent(TEXT("perception_manifested"),C.Id);
        }
        NextFamilyTick=Tick+10; NextPerceptionHazard=Tick+20;
    }
    for (auto& C : Cues)
    {
        if (C.Id.StartsWith(TEXT("perception.wraith")))
        {
            const FVector Next = C.Location+(A->GetActorLocation()-C.Location).GetSafeNormal2D()*6;
            FVector Ground;
            if (A->Primary->Ground(Next,Ground) && A->Smuggler->Sight(C.Location+FVector(0,0,40),Ground+FVector(0,0,40))) { C.Location=Ground; }
        }
        if (Band>=2 && Tick>=NextPerceptionHazard && FVector::Dist2D(A->GetActorLocation(),C.Location)<=65)
        {
            Add(3,TEXT("subjective_contact")); NextPerceptionHazard=Tick+20;
            if (Band>=3 || State.CrisisUntil)
            { FDMControl Control; Control.Slow=.2f; Control.SlowTicks=5; A->ApplyControl(Control,nullptr,TEXT("madness.perception.contact")); }
            FamilyEvent(TEXT("perception_contact"),C.Id);
        }
    }
    // Companions consult only their own layer and use the same validated verb as humans.
    if (!A->IsPlayerControlled() && Tick>=NextPerceptionUse)
    {
        FString Nearest; float Distance=MAX_flt;
        for (const auto& C : Cues)
        { const float D=FVector::DistSquared2D(A->GetActorLocation(),C.Location); if (D<Distance) { Distance=D; Nearest=C.Id; } }
        if (!Nearest.IsEmpty()) { InteractPerception(Nearest); }
    }
}
bool UDMMadnessComponent::InteractPerception(const FString& CueId)
{
    if (!Authority() || Family!=EDMMadnessFamily::Perception || Now()<NextPerceptionUse) { return false; }
    auto* A = CastChecked<ADMCombatant>(GetOwner()); auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!M || !M->IsCombatActive() || A->IsDown() || A->IsStunned() || A->IsRestrained()) { return false; }
    const int32 I=Cues.IndexOfByPredicate([&](const auto& C) { return C.Id==CueId; });
    if (I==INDEX_NONE || Now()>=Cues[I].Until) { return false; }
    const float Reach=State.CrisisUntil ? 320.f : 200.f;
    if (FVector::Dist2D(A->GetActorLocation(),Cues[I].Location)>Reach || !A->Primary->Sight(Cues[I].Location)) { return false; }
    NextPerceptionUse=Now()+5; InterruptGrounding();
    Cues[I].Progress+=State.CrisisUntil ? 2 : 1;
    FamilyEvent(TEXT("perception_interacted"),CueId);
    if (Cues[I].Progress>=Cues[I].Goal)
    {
        const bool bAnomaly=Cues[I].Id.StartsWith(TEXT("perception.anomaly")); Cues.RemoveAt(I);
        if (!bAnomaly && (State.Band(Settings)>=3 || State.CrisisUntil)) { A->AddShield(5); A->HealHealth(3); }
        FamilyEvent(TEXT("perception_cleared"),CueId);
    }
    Deliver(); return true;
}
