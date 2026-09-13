#include "DMShubEncounter.h"
#include "DMBossArena.h"
#include "DMCorruption.h"
#include "DMGrowthNetwork.h"
#include "DMShubMinion.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMGameState.h"
#include "DMAbilityMarker.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"
ADMShubEncounter::ADMShubEncounter()
{ bReplicates=true; bAlwaysRelevant=true; SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Encounter"))); }
bool ADMShubEncounter::Begin(ADMBossArena* InArena)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); FString Error;
    if (!HasAuthority() || !M || !M->IsCombatActive() || Boss || !IsValid(InArena) || InArena->GetWorld()!=GetWorld()
        || !InArena->Validate(EDMElderOne::Shub,Error) || M->ElderOne->Identity()!=EDMElderOne::Shub || M->ElderOne->Phase()!=EDMBossPhase::Dormant) { return false; }
    Arena=InArena; Boss=M->SpawnEncounterActor(TEXT("elder.shub"),Arena->Locations(TEXT("BossSite"))[0]+FVector(0,0,95),1800,18);
    if (!Boss) { return false; }
    Boss->Tags.Add(TEXT("ShubBoss")); Boss->Tags.Add(TEXT("ShubLinked")); Boss->EncounterLabel=TEXT("Shub-Niggurath | Rooted"); Boss->bCommonEnemy=false; Boss->Resolve->Reset();
    Field=GetWorld()->SpawnActor<ADMCorruption>(); Field->Initialize(Arena); Field->Spread(Boss->GetActorLocation());
    Growths=GetWorld()->SpawnActor<ADMGrowthNetwork>();
    if (!Growths->Initialize(Arena,Boss,Field,M->DrawRandom(EDMRandomStream::BossVariation))) { return false; }
    M->ElderOne->Begin(); M->UseBossOutcome(); State.NextMove=M->GetCombatTick()+20; return true;
}
void ADMShubEncounter::Step(int32 Tick)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !M->IsCombatActive() || !IsValid(Boss) || State.bFinished || Tick<=LastTick) { return; } LastTick=Tick;
    if (auto* G=GetWorld()->GetGameState<ADMGameState>()) { G->SetEncounterObjective(TEXT("Shub-Niggurath | Control genuine growths, interrupt reproduction, escape corruption")); }
    bool Living=false; for (ADMCombatant* A : M->GetCombatants()) { Living|=!A->bIsEnemy && !A->IsDown(); }
    if (Boss->IsDown() || !Living)
    { State.bFinished=true; ClearMarkers(); M->ElderOne->Finish(Boss->IsDown() && Living); M->CompleteBossEncounter(Boss->IsDown() && Living); return; }
    const float Fraction=Boss->Health()/Boss->MaxHealth();
    if (Fraction<=.66f && M->ElderOne->Phase()==EDMBossPhase::Rooted)
    { M->ElderOne->Advance(EDMBossPhase::Mobile); Boss->EncounterLabel=TEXT("Shub-Niggurath | Mobile"); }
    if (Fraction<=.33f && M->ElderOne->Phase()==EDMBossPhase::Mobile)
    { M->ElderOne->Advance(EDMBossPhase::Frenzy); Boss->EncounterLabel=TEXT("Shub-Niggurath | Birthing frenzy"); }
    if (M->ElderOne->Phase()!=EDMBossPhase::Rooted) { Field->Spread(Boss->GetActorLocation()); }
    if (Boss->IsStunned() || Boss->bBreakVulnerable)
    { State.Move=EDMShubMove::Idle; State.bCharging=false; State.NextMove=Tick+20; ClearMarkers(); Boss->StopGoal(); return; }
    if (State.bCharging)
    {
        const FVector Before=Boss->GetActorLocation(); const FVector Next=FMath::VInterpConstantTo(Before,State.Destination,.1f,700.f);
        FHitResult Hit; Boss->SetActorLocation(Next,true,&Hit); Field->Spread(Boss->GetActorLocation());
        for (ADMCombatant* A : M->GetCombatants())
        { if (!A->bIsEnemy && !A->IsDown() && !State.HitIds.Contains(A->EntityId) && FVector::DistSquared2D(A->GetActorLocation(),Boss->GetActorLocation())<FMath::Square(180.f))
          { State.HitIds.Add(A->EntityId); FDMControl C; C.Damage=20; C.Displacement=(Next-Before).GetSafeNormal2D()*200; C.StaggerTicks=3; A->ApplyControl(C,Boss,TEXT("shub.trampling_advance")); } }
        if (Hit.bBlockingHit || FVector::DistSquared2D(Boss->GetActorLocation(),State.Destination)<FMath::Square(30.f) || Tick>=State.Until+30)
        { State.bCharging=false; State.Move=EDMShubMove::Idle; ClearMarkers(); State.NextMove=Tick+15; }
        return;
    }
    if (State.Move!=EDMShubMove::Idle) { if (Tick>=State.Until) { Execute(Tick); } return; }
    ADMCombatant* Target=nullptr; float Best=MAX_flt;
    for (ADMCombatant* A : M->GetCombatants())
    { const float D=FVector::DistSquared2D(A->GetActorLocation(),Boss->GetActorLocation()); if (!A->bIsEnemy && !A->IsDown() && D<Best) { Best=D; Target=A; } }
    Boss->SetAttackTarget(Target); Boss->SetAttackHold(false);
    if (Target && M->ElderOne->Phase()!=EDMBossPhase::Rooted && Arena->Contains(TEXT("MovementLane"),Target->GetActorLocation())) { Boss->MoveToward(Target->GetActorLocation()); } else { Boss->StopGoal(); }
    if (Tick>=State.NextMove) { Windup(Tick); }
}
void ADMShubEncounter::Windup(int32 Tick)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!M || !Boss) { return; }
    State.Move=static_cast<EDMShubMove>(1+M->DrawRandom(EDMRandomStream::BossVariation)%4);
    if (M->ElderOne->Phase()==EDMBossPhase::Rooted && State.Move==EDMShubMove::TramplingAdvance) { State.Move=EDMShubMove::CallTheBrood; }
    State.Until=Tick+15; State.Targets.Reset(); State.HitIds.Reset(); Boss->StopGoal(); Boss->SetAttackHold(true); ClearMarkers();
    auto Mark=[&](FVector P,float Radius,const FString& Label)
    { auto* Marker=GetWorld()->SpawnActor<ADMAbilityMarker>(P,FRotator::ZeroRotator); Marker->bHostile=true; Marker->Radius=Radius; Marker->CustomLabel=Label; Marker->ArmedTick=State.Until; Marker->ExpiresTick=State.Until+40; Markers.Add(Marker); return Marker; };
    if (State.Move==EDMShubMove::BlackMilk)
    { for (ADMCombatant* A : M->GetCombatants())
      { if (!A->bIsEnemy && !A->IsDown() && Arena->Contains(TEXT("CorruptionValid"),A->GetActorLocation())) { State.Targets.Add(A->GetActorLocation()); Mark(A->GetActorLocation(),180,TEXT("Black Milk")); } } }
    else if (State.Move==EDMShubMove::TramplingAdvance)
    {
        const auto Sites=Arena->Locations(TEXT("ManifestationPoint")); State.Destination=Sites[M->DrawRandom(EDMRandomStream::BossVariation)%Sites.Num()]+FVector(0,0,95);
        auto* Marker=Mark(Boss->GetActorLocation(),140,TEXT("Trampling Advance")); Marker->Shape=EDMMarkerShape::Wire; Marker->WireEnd=State.Destination;
    }
    else { Mark(Boss->GetActorLocation(),State.Move==EDMShubMove::HornedSweep ? 350 : 180,State.Move==EDMShubMove::HornedSweep ? TEXT("Horned Sweep") : TEXT("Call the Brood")); }
    auto D=MakeShared<FJsonObject>(); D->SetStringField(TEXT("attack"),StaticEnum<EDMShubMove>()->GetNameStringByValue(static_cast<int64>(State.Move))); M->Emit(TEXT("boss.windup"),D); ForceNetUpdate();
}
void ADMShubEncounter::Execute(int32 Tick)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!M) { return; }
    if (State.Move==EDMShubMove::TramplingAdvance) { State.bCharging=true; return; }
    if (State.Move==EDMShubMove::BlackMilk)
    { for (const FVector& P : State.Targets) { Field->Spread(P); Field->Spread(P+FVector(150,0,0)); Field->Spread(P+FVector(0,150,0)); } }
    else if (State.Move==EDMShubMove::HornedSweep)
    { for (ADMCombatant* A : M->GetCombatants())
      { if (!A->bIsEnemy && !A->IsDown() && FVector::DistSquared2D(A->GetActorLocation(),Boss->GetActorLocation())<FMath::Square(350.f))
        { FDMControl C; C.Damage=15; C.Displacement=(A->GetActorLocation()-Boss->GetActorLocation()).GetSafeNormal2D()*250; C.StaggerTicks=4; A->ApplyControl(C,Boss,TEXT("shub.horned_sweep")); } } }
    else if (State.Move==EDMShubMove::CallTheBrood)
    {
        int32 Live=0; for (ADMCombatant* A : M->GetCombatants()) { Live+=A->ActorHasTag(TEXT("ShubLinked")) && !A->IsDown() ? 1 : 0; }
        const int32 Count=M->ElderOne->Phase()==EDMBossPhase::Frenzy ? 3 : 2;
        const auto Sites=Arena->Locations(TEXT("SpawnRoute"));
        for (int32 I=0; I<Count && Live<24; ++I,++Live)
        {
            const int32 Serial=State.SpawnSerial++;
            const bool Goat=Serial%4==3;
            auto* A=M->SpawnEncounterActor(FString::Printf(TEXT("shub.add.%d"),Serial),Sites[Serial%Sites.Num()]+FVector(0,0,95),Goat ? 220 : 60,Goat ? 12 : 5);
            if (A) { auto* C=NewObject<UDMShubMinion>(A); A->AddInstanceComponent(C); C->RegisterComponent(); C->Initialize(Goat ? EDMShubMinionKind::Goat : EDMShubMinionKind::Broodling); }
        }
    }
    State.Move=EDMShubMove::Idle; State.NextMove=Tick+(M->ElderOne->Phase()==EDMBossPhase::Frenzy ? 15 : 30); ClearMarkers(); ForceNetUpdate();
}
void ADMShubEncounter::ClearMarkers() { for (ADMAbilityMarker* Marker : Markers) { if (IsValid(Marker)) { Marker->Destroy(); } } Markers.Reset(); }
bool ADMShubEncounter::Restore(const FDMShubEncounterSnapshot& S)
{
    if (!HasAuthority() || !Boss || !Arena || S.Version!=1 || S.Move>EDMShubMove::HornedSweep || S.Until<0 || S.NextMove<0 || S.SpawnSerial<0 || S.Destination.ContainsNaN() || (S.bCharging && S.Move!=EDMShubMove::TramplingAdvance)) { return false; }
    for (const auto& P : S.Targets) { if (P.ContainsNaN() || !Arena->Contains(TEXT("CorruptionValid"),P)) { return false; } }
    if (S.Move==EDMShubMove::TramplingAdvance && !Arena->Contains(TEXT("MovementLane"),S.Destination)) { return false; }
    State=S; LastTick=-1; ClearMarkers();
    // Restore tells without drawing randomness again or resolving an attack early.
    if (State.Move!=EDMShubMove::Idle && !State.bFinished)
    {
        TArray<FVector> Places=State.Move==EDMShubMove::BlackMilk ? State.Targets : TArray<FVector>{Boss->GetActorLocation()};
        for (const FVector& P : Places)
        {
            auto* Marker=GetWorld()->SpawnActor<ADMAbilityMarker>(P,FRotator::ZeroRotator); Marker->bHostile=true;
            Marker->Radius=State.Move==EDMShubMove::HornedSweep ? 350 : 180; Marker->ArmedTick=State.Until; Marker->ExpiresTick=State.Until+40;
            Marker->CustomLabel=StaticEnum<EDMShubMove>()->GetNameStringByValue(static_cast<int64>(State.Move));
            if (State.Move==EDMShubMove::TramplingAdvance) { Marker->Shape=EDMMarkerShape::Wire; Marker->WireEnd=State.Destination; }
            Markers.Add(Marker);
        }
    }
    ForceNetUpdate(); return true;
}
void ADMShubEncounter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADMShubEncounter,Boss); DOREPLIFETIME(ADMShubEncounter,State); }
