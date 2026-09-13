#include "DMRelicDrop.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
ADMRelicDrop::ADMRelicDrop() { bReplicates = true; bAlwaysRelevant = true; }
bool ADMRelicDrop::Initialize(const FString& Id)
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !M->IsCombatActive() || !Roll.AwardId.IsEmpty() || Id.IsEmpty()) { return false; }
    for (TActorIterator<ADMRelicDrop> It(GetWorld());It;++It) { if (*It!=this && It->Roll.AwardId==Id) { return false; } }
    for (ADMCombatant* A : M->GetCombatants())
    { if (auto* I=A->FindComponentByClass<UDMRelicComponent>(); I && I->Capture().AwardIds.Contains(Id)) { return false; } }
    Roll.AwardId = Id; Roll.Relic = static_cast<EDMRelic>(M->DrawRandom(EDMRandomStream::Relics)%8); Roll.Deadline = M->GetCombatTick()+200;
    for (ADMCombatant* A : M->GetCombatants()) { if (!A->bIsEnemy) { FDMRelicVote V; V.EntityId = A->EntityId; Roll.Votes.Add(V); } }
    auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("award_id"),Id); D->SetStringField(TEXT("relic"),UDMRelicComponent::Name(Roll.Relic)); M->Emit(TEXT("relic.offered"),D);
    ForceNetUpdate(); return true;
}
bool ADMRelicDrop::Vote(ADMCombatant* A, EDMRelicChoice C)
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !M->IsCombatActive() || Roll.bResolved || M->GetCombatTick()>=Roll.Deadline || !IsValid(A) || A->GetWorld()!=GetWorld() || A->bIsEnemy || C<EDMRelicChoice::Pass || C>EDMRelicChoice::Need) { return false; }
    auto* Inventory = A->FindComponentByClass<UDMRelicComponent>();
    if (!Inventory || (C!=EDMRelicChoice::Pass && !Inventory->CanAcquire(Roll.Relic))) { return false; }
    auto* V = Roll.Votes.FindByPredicate([&](const FDMRelicVote& X) { return X.EntityId==A->EntityId; });
    if (!V || V->Choice!=EDMRelicChoice::Pending) { return false; }
    V->Choice = C; ForceNetUpdate(); return true;
}
void ADMRelicDrop::Step(int32 Tick)
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !M->IsCombatActive() || Roll.AwardId.IsEmpty() || Roll.bResolved) { return; }
    bool Pending = false;
    for (auto& V : Roll.Votes)
    {
        if (V.Choice!=EDMRelicChoice::Pending) { continue; }
        ADMCombatant* A = M->FindCombatant(V.EntityId); auto* Inventory = A ? A->FindComponentByClass<UDMRelicComponent>() : nullptr;
        if (!A || !Inventory || !Inventory->CanAcquire(Roll.Relic) || Tick>=Roll.Deadline) { V.Choice = EDMRelicChoice::Pass; }
        else if (!A->IsPlayerControlled())
        { const float Fit = Inventory->Value(Roll.Relic); V.Choice = Fit>=2 ? EDMRelicChoice::Need : Fit>0 ? EDMRelicChoice::Greed : EDMRelicChoice::Pass; }
        Pending |= V.Choice==EDMRelicChoice::Pending;
    }
    if (!Pending) { Resolve(); }
}
void ADMRelicDrop::Resolve()
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!M || Roll.bResolved) { return; }
    TArray<ADMCombatant*> Candidates; EDMRelicChoice Rank = EDMRelicChoice::Pass;
    for (const auto& V : Roll.Votes)
    {
        ADMCombatant* A = M->FindCombatant(V.EntityId); auto* I = A ? A->FindComponentByClass<UDMRelicComponent>() : nullptr;
        if (!I || !I->CanAcquire(Roll.Relic) || V.Choice<=EDMRelicChoice::Pass || V.Choice<Rank) { continue; }
        if (V.Choice>Rank) { Candidates.Reset(); Rank = V.Choice; }
        Candidates.Add(A);
    }
    Candidates.Sort([](const ADMCombatant& A,const ADMCombatant& B) { return A.EntityId<B.EntityId; });
    if (!Candidates.IsEmpty())
    {
        auto* Winner = Candidates[M->DrawRandom(EDMRandomStream::Relics)%Candidates.Num()];
        if (Winner->FindComponentByClass<UDMRelicComponent>()->Acquire(Roll.Relic,Roll.AwardId)) { Roll.Winner = Winner->EntityId; }
    }
    Roll.bResolved = true; ForceNetUpdate();
    auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("award_id"),Roll.AwardId); D->SetStringField(TEXT("winner"),Roll.Winner); M->Emit(TEXT("relic.allocated"),D);
}
bool ADMRelicDrop::Restore(const FDMRelicRollSnapshot& S)
{
    if (!HasAuthority() || S.Version!=1 || S.AwardId.IsEmpty() || static_cast<uint8>(S.Relic)>=8 || S.Votes.Num()>4 || S.Deadline<0 || (!S.bResolved && !S.Winner.IsEmpty())) { return false; }
    TSet<FString> Ids;
    for (const auto& V : S.Votes) { if (V.EntityId.IsEmpty() || Ids.Contains(V.EntityId) || V.Choice>EDMRelicChoice::Need) { return false; } Ids.Add(V.EntityId); }
    if (!S.Winner.IsEmpty() && !Ids.Contains(S.Winner)) { return false; }
    Roll = S; ForceNetUpdate(); return true;
}
void ADMRelicDrop::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADMRelicDrop,Roll); }
