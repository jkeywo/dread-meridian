#include "DMSwampThing.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMAbilityMarker.h"
#include "DMVision.h"
#include "Net/UnrealNetwork.h"
UDMSwampThing::UDMSwampThing() { SetIsReplicatedByDefault(true); }
ADMCombatant* UDMSwampThing::Self() const { return CastChecked<ADMCombatant>(GetOwner()); }
bool UDMSwampThing::Initialize(EDMSwampThing Role)
{
    if (!Self()->HasAuthority() || !Self()->bIsEnemy || Role<=EDMSwampThing::None || Role>EDMSwampThing::OldThing || State.Role!=EDMSwampThing::None) { return false; }
    State.Role=Role; State.Home=Self()->GetActorLocation(); State.InterruptSerial=Self()->ControlInterruptSerial;
    Self()->bSwampThing=true; Self()->bRequiresVision=true; Self()->bHumanEnemy=false; Self()->bCommonEnemy=Role!=EDMSwampThing::OldThing; Self()->Resolve->Reset();
    Self()->AttackIntervalTicks=Role==EDMSwampThing::Crawler ? 12 : 20;
    Self()->EncounterLabel=StaticEnum<EDMSwampThing>()->GetNameStringByValue(static_cast<int64>(Role));
    Record(TEXT("initialized")); return true;
}
float UDMSwampThing::Range() const { return State.Role==EDMSwampThing::Spitter ? 600 : State.Role==EDMSwampThing::Grasper ? 350 : 150; }
float UDMSwampThing::Speed() const { return State.Role==EDMSwampThing::Crawler ? 520 : State.Role==EDMSwampThing::OldThing ? 280 : 380; }
bool UDMSwampThing::IsIsolated(const ADMCombatant* Target) const
{
    const auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!M || !Target) { return false; }
    for (ADMCombatant* A : M->GetCombatants())
    { if (A!=Target && !A->IsDown() && A->bIsEnemy==Target->bIsEnemy && !A->IsHostileTo(Target) && FVector::DistSquared2D(A->GetActorLocation(),Target->GetActorLocation())<FMath::Square(400.f)) { return false; } }
    return true;
}
float UDMSwampThing::DamageMultiplier(const ADMCombatant* Target) const
{ return State.Role==EDMSwampThing::Crawler && IsIsolated(Target) ? 1.3f : 1.f; }
bool UDMSwampThing::TrySignature(ADMCombatant* Target)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Self()->HasAuthority() || !M || !M->IsCombatActive() || Self()->IsDown() || Self()->IsStunned() || Self()->IsRestrained() || Self()->bBreakVulnerable
        || !IsValid(Target) || Target->GetWorld()!=GetWorld() || Target->IsDown() || !Self()->IsHostileTo(Target) || State.Role<=EDMSwampThing::Crawler
        || State.ResolveAt>0 || M->GetCombatTick()<State.ReadyAt || !Self()->Smuggler->Sight(Self()->GetActorLocation(),Target->GetActorLocation())) { return false; }
    const float Reach=State.Role==EDMSwampThing::OldThing ? 450 : State.Role==EDMSwampThing::Lurker ? 500 : 750;
    if (FVector::DistSquared2D(Self()->GetActorLocation(),Target->GetActorLocation())>FMath::Square(Reach)
        || FVector::DistSquared2D(Target->GetActorLocation(),State.Home)>FMath::Square(1200.f)) { return false; }
    State.TargetId=Target->EntityId; State.Aim=Target->GetActorLocation(); State.CastOrigin=Self()->GetActorLocation(); State.ResolveAt=M->GetCombatTick()+12;
    State.InterruptSerial=Self()->ControlInterruptSerial; Self()->StopGoal(); Self()->SetAttackHold(true);
    Self()->Resolve->OpenInterruptWindow(12); Project(); Record(TEXT("windup")); return true;
}
void UDMSwampThing::Cancel(int32 Tick)
{ State.ResolveAt=0; State.TargetId.Reset(); State.ReadyAt=Tick+30; Self()->SetAttackHold(false); Project(); Record(TEXT("interrupted")); }
void UDMSwampThing::Stop()
{ if (!Self()->HasAuthority() || State.Role==EDMSwampThing::None) { return; } State.ResolveAt=0; State.TargetId.Reset(); State.PoolUntil=0; Self()->StopGoal(); Self()->SetAttackHold(true); Project(); Self()->ForceNetUpdate(); }
void UDMSwampThing::Resolve(int32 Tick)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    auto* Target=M->FindCombatant(State.TargetId);
    if (State.Role==EDMSwampThing::Spitter)
    { State.Pool=State.Aim; State.PoolUntil=Tick+80; State.NextPoolTick=Tick; }
    else if (State.Role==EDMSwampThing::OldThing)
    {
        for (ADMCombatant* A : M->GetCombatants())
        { if (!A->IsDown() && Self()->IsHostileTo(A) && FVector::DistSquared2D(A->GetActorLocation(),Self()->GetActorLocation())<=FMath::Square(450.f)
            && Self()->Smuggler->Sight(Self()->GetActorLocation(),A->GetActorLocation()))
          { FDMControl C; C.Damage=12; C.Displacement=(A->GetActorLocation()-Self()->GetActorLocation()).GetSafeNormal2D()*(IsIsolated(A) ? 350 : 180); C.StaggerTicks=4; A->ApplyControl(C,Self(),TEXT("swamp.old_thing_sweep")); } }
    }
    else if (Target && !Target->IsDown() && Self()->IsHostileTo(Target) && FVector::DistSquared2D(Target->GetActorLocation(),State.Aim)<=FMath::Square(150.f)
        && FVector::DistSquared2D(Target->GetActorLocation(),Self()->GetActorLocation())<=FMath::Square(750.f)
        && Self()->Smuggler->Sight(Self()->GetActorLocation(),Target->GetActorLocation()))
    {
        FDMControl C;
        if (State.Role==EDMSwampThing::Grasper)
        { const FVector Delta=Self()->GetActorLocation()-Target->GetActorLocation(); C.Displacement=Delta.GetSafeNormal2D()*FMath::Min(400.f,FMath::Max(0.f,Delta.Size2D()-130)); C.StaggerTicks=3; C.Damage=5; }
        else { C.Slow=.4f; C.SlowTicks=25; C.Damage=6; }
        Target->ApplyControl(C,Self(),State.Role==EDMSwampThing::Grasper ? TEXT("swamp.grasp") : TEXT("swamp.ambush"));
    }
    State.ResolveAt=0; State.TargetId.Reset(); State.ReadyAt=Tick+(State.Role==EDMSwampThing::Spitter ? 90 : 60); Self()->SetAttackHold(false); Project(); Record(TEXT("resolved"));
}
void UDMSwampThing::Step(int32 Tick)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Self()->HasAuthority() || !M || !M->IsCombatActive() || State.Role==EDMSwampThing::None || Tick<=LastTick) { return; } LastTick=Tick;
    if (Self()->IsDown()) { if (State.ResolveAt || State.PoolUntil) { Stop(); } return; }
    if (State.PoolUntil && Tick>=State.PoolUntil) { State.PoolUntil=0; Project(); Record(TEXT("pool_expired")); }
    if (State.PoolUntil>Tick && Tick>=State.NextPoolTick)
    {
        State.NextPoolTick=Tick+10;
        for (ADMCombatant* A : M->GetCombatants())
        { if (!A->IsDown() && Self()->IsHostileTo(A) && FVector::DistSquared2D(A->GetActorLocation(),State.Pool)<=FMath::Square(220.f)
            && Self()->Smuggler->Sight(State.Pool,A->GetActorLocation()))
          { Self()->DealCombatDamage(A,4,TEXT("swamp.spit_pool"),false,true); FDMControl C; C.Slow=.3f; C.SlowTicks=12; A->ApplyControl(C,Self(),TEXT("swamp.spit_pool")); } }
    }
    if (Self()->IsStunned() || Self()->IsRestrained() || Self()->bBreakVulnerable || Self()->ControlInterruptSerial!=State.InterruptSerial)
    { State.InterruptSerial=Self()->ControlInterruptSerial; if (State.ResolveAt) { Cancel(Tick); } Self()->StopGoal(); return; }
    if (State.ResolveAt) { if (FVector::DistSquared2D(Self()->GetActorLocation(),State.CastOrigin)>FMath::Square(100.f)) { Cancel(Tick); } else if (Tick>=State.ResolveAt) { Resolve(Tick); } return; }
    Self()->Threat.Cleanup([&](const FString& Id) { auto* A=M->FindCombatant(Id); return A && !A->IsDown() && Self()->IsHostileTo(A); },Tick);
    ADMCombatant* Target=nullptr; float Best=-MAX_flt;
    const FString Forced=Self()->Threat.Forced(Tick);
    for (ADMCombatant* A : M->GetCombatants())
    {
        const float Distance=FVector::Dist2D(A->GetActorLocation(),Self()->GetActorLocation());
        if (A->IsDown() || !Self()->IsHostileTo(A) || Distance>1000 || FVector::DistSquared2D(A->GetActorLocation(),State.Home)>FMath::Square(1200.f)
            || !DMVision::CanSee(Self(),A) || !Self()->Smuggler->Sight(Self()->GetActorLocation(),A->GetActorLocation())) { continue; }
        if (A->EntityId==Forced) { Target=A; break; }
        const float Score=Self()->Threat.FindRef(A->EntityId)+(IsIsolated(A) ? 500 : 0)-Distance;
        if (Score>Best || (Score==Best && Target && A->EntityId<Target->EntityId)) { Target=A; Best=Score; }
    }
    Self()->SetAttackTarget(Target); Self()->SetAttackHold(false);
    if (!Target) { Self()->SetAttackHold(true); if (FVector::DistSquared2D(Self()->GetActorLocation(),State.Home)>FMath::Square(80.f)) { Self()->MoveToward(State.Home); } else { Self()->StopGoal(); } return; }
    if (TrySignature(Target)) { return; }
    if (State.Role==EDMSwampThing::Lurker && Tick>=State.ReadyAt) { Self()->MoveToward(Target->GetActorLocation()); Self()->SetAttackHold(true); return; }
    if (FVector::DistSquared2D(Self()->GetActorLocation(),Target->GetActorLocation())>FMath::Square(Range()*.85f)) { Self()->MoveToward(Target->GetActorLocation()); } else { Self()->StopGoal(); }
}
void UDMSwampThing::Project()
{
    if (Tell) { Tell->Destroy(); Tell=nullptr; } if (PoolMarker) { PoolMarker->Destroy(); PoolMarker=nullptr; }
    if (State.ResolveAt)
    {
        Tell=GetWorld()->SpawnActor<ADMAbilityMarker>(State.Role==EDMSwampThing::OldThing ? Self()->GetActorLocation() : State.Aim,FRotator::ZeroRotator);
        Tell->bHostile=true; Tell->Radius=State.Role==EDMSwampThing::OldThing ? 450 : State.Role==EDMSwampThing::Spitter ? 220 : 150;
        Tell->bVisionFiltered=true; Tell->VisionSubject=Self();
        Tell->ArmedTick=State.ResolveAt; Tell->ExpiresTick=State.ResolveAt+1; Tell->CustomLabel=Self()->EncounterLabel+TEXT(" - interrupt or evade");
        if (State.Role==EDMSwampThing::Grasper) { Tell->SetActorLocation(Self()->GetActorLocation()); Tell->Shape=EDMMarkerShape::Wire; Tell->WireEnd=State.Aim; }
    }
    if (State.PoolUntil)
    { PoolMarker=GetWorld()->SpawnActor<ADMAbilityMarker>(State.Pool,FRotator::ZeroRotator); PoolMarker->bHostile=true; PoolMarker->Radius=220; PoolMarker->ExpiresTick=State.PoolUntil; PoolMarker->CustomLabel=TEXT("Spitter pool"); }
}
bool UDMSwampThing::Restore(const FDMSwampSnapshot& S)
{
    if (!Self()->HasAuthority() || S.Version!=1 || S.Role!=State.Role || S.Role==EDMSwampThing::None || S.Home.ContainsNaN() || S.Aim.ContainsNaN() || S.CastOrigin.ContainsNaN() || S.Pool.ContainsNaN()
        || S.ResolveAt<0 || S.ReadyAt<0 || S.PoolUntil<0 || S.NextPoolTick<0 || S.InterruptSerial<0 || (S.ResolveAt>0 && S.TargetId.IsEmpty()) || (S.PoolUntil>0 && S.Role!=EDMSwampThing::Spitter)) { return false; }
    State=S; LastTick=-1; Self()->SetAttackHold(State.ResolveAt>0); Project(); return true;
}
void UDMSwampThing::Record(const FString& Action)
{ auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!M || !M->FindCombatant(Self()->EntityId)) { return; } auto D=MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"),Self()->EntityId); D->SetStringField(TEXT("role"),Self()->EncounterLabel); D->SetStringField(TEXT("action"),Action); M->Emit(TEXT("swamp.action"),D); Self()->ForceNetUpdate(); }
void UDMSwampThing::EndPlay(const EEndPlayReason::Type Reason)
{ if (Tell) { Tell->Destroy(); } if (PoolMarker) { PoolMarker->Destroy(); } Super::EndPlay(Reason); }
void UDMSwampThing::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UDMSwampThing,State); }
