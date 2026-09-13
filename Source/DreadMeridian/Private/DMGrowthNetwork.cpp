#include "DMGrowthNetwork.h"
#include "DMBossArena.h"
#include "DMCorruption.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "Net/UnrealNetwork.h"
ADMGrowthNetwork::ADMGrowthNetwork() { bReplicates=true; bAlwaysRelevant=true; }
bool ADMGrowthNetwork::Initialize(ADMBossArena* Arena,ADMCombatant* InBoss,ADMCorruption* InField,uint32 Draw)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !IsValid(Arena) || !IsValid(InBoss) || !IsValid(InField) || Boss || Arena->GetWorld()!=GetWorld() || InBoss->GetWorld()!=GetWorld() || InField->GetWorld()!=GetWorld()) { return false; }
    const auto Sites=Arena->Locations(TEXT("GrowthSite")); if (Sites.Num()<2) { return false; }
    Boss=InBoss; Field=InField;
    for (int32 I=0; I<Sites.Num(); ++I)
    {
        const FString Id=GetName()+FString::Printf(TEXT(".growth.%d"),I);
        auto* Node=M->SpawnEncounterActor(Id,Sites[I]+FVector(0,0,80),120,0,true); if (!Node) { return false; }
        Node->Tags.Add(TEXT("ShubLinked")); Nodes.Add(Node);
        if (I!=static_cast<int32>(Draw%Sites.Num())) { State.GenuineIds.Add(Id); }
    }
    return true;
}
void ADMGrowthNetwork::Step(int32 Tick)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !M->IsCombatActive() || Tick<=LastTick || !IsValid(Boss)) { return; } LastTick=Tick;
    TArray<FDMMadnessCue> Cues;
    for (ADMCombatant* Node : Nodes)
    {
        if (!IsValid(Node) || !State.GenuineIds.Contains(Node->EntityId)) { continue; }
        if (Node->IsDown())
        {
            if (!State.ResolvedIds.Contains(Node->EntityId) && !Boss->IsDown())
            {
                State.ResolvedIds.Add(Node->EntityId); Boss->IncomingMultiplier=1.5f; Boss->IncomingUntilTick=Tick+60;
                Boss->Resolve->AddPressure(Boss->Resolve->Settings.MaxResolve,nullptr,TEXT("shub.growth_destroyed"));
                auto D=MakeShared<FJsonObject>(); D->SetStringField(TEXT("node_id"),Node->EntityId); D->SetNumberField(TEXT("until_tick"),Tick+60); M->Emit(TEXT("boss.node_vulnerability"),D);
            }
            continue;
        }
        if (Boss->IsDown()) { continue; }
        if (Node->IsStunned() || Node->IsRestrained()) { Boss->IncomingMultiplier=1.3f; Boss->IncomingUntilTick=FMath::Max(Boss->IncomingUntilTick,Tick+20); }
        FDMMadnessCue Cue; Cue.Id=TEXT("resonance.")+Node->EntityId; Cue.TargetId=Node->EntityId; Cue.Location=Node->GetActorLocation(); Cue.Label=TEXT("Genuine reproductive growth"); Cue.Until=Tick+2; Cues.Add(Cue);
        if (Field && Tick>=State.NextSpread && !Node->IsStunned() && !Node->IsRestrained())
        {
            const float Angle=State.SpreadWave*PI/4; const float Radius=150.f*(1+State.SpreadWave/8);
            Field->Spread(Node->GetActorLocation()+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0));
        }
    }
    if (Tick>=State.NextSpread) { State.NextSpread=Tick+30; ++State.SpreadWave; }
    for (ADMCombatant* A : M->GetCombatants())
    { if (!A->bIsEnemy) { A->MadnessCore->SetResonanceCues(M->ElderOne->IsResonant(A) && !Boss->IsDown() ? Cues : TArray<FDMMadnessCue>()); } }
}
bool ADMGrowthNetwork::Restore(const FDMGrowthSnapshot& S)
{
    if (!HasAuthority() || S.Version!=1 || S.NextSpread<0 || S.SpreadWave<0) { return false; }
    TSet<FString> Known; for (ADMCombatant* Node : Nodes) { if (IsValid(Node)) { Known.Add(Node->EntityId); } }
    TSet<FString> Genuine,Resolved;
    for (const auto& Id : S.GenuineIds) { if (!Known.Contains(Id) || Genuine.Contains(Id)) { return false; } Genuine.Add(Id); }
    for (const auto& Id : S.ResolvedIds) { if (!Genuine.Contains(Id) || Resolved.Contains(Id)) { return false; } Resolved.Add(Id); }
    State=S; LastTick=-1; return true;
}
void ADMGrowthNetwork::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADMGrowthNetwork,Nodes); }
