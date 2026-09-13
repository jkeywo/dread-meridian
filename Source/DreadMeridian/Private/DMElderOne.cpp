#include "DMElderOne.h"
#include "DMCombatant.h"
TArray<EDMMadnessFamily> UDMElderOne::Families(EDMElderOne Boss,uint32 SeatDraw,const TArray<uint32>& Draws)
{
    if (Draws.Num()!=4 || static_cast<uint8>(Boss)>1) { return {}; }
    const auto Match = Boss==EDMElderOne::Shub ? EDMMadnessFamily::Obsession : EDMMadnessFamily::Perception;
    TArray<EDMMadnessFamily> Others, Result;
    for (uint8 I=1; I<=4; ++I) { if (static_cast<EDMMadnessFamily>(I)!=Match) { Others.Add(static_cast<EDMMadnessFamily>(I)); } }
    for (int32 I=0; I<4; ++I) { Result.Add(I==static_cast<int32>(SeatDraw%4) ? Match : Others[Draws[I]%3]); }
    return Result;
}
bool UDMElderOne::AssignResonance(const TArray<ADMCombatant*>& Roster,uint32 SeatDraw,const TArray<uint32>& Draws)
{
    if (!Authority() || !State.bSelected || State.Phase!=EDMBossPhase::Dormant || !State.ResonantId.IsEmpty() || Roster.Num()!=4) { return false; }
    TSet<FString> Ids;
    for (const auto* A : Roster) { if (!IsValid(A) || A->GetWorld()!=GetWorld() || A->bIsEnemy || A->EntityId.IsEmpty() || Ids.Contains(A->EntityId)) { return false; } Ids.Add(A->EntityId); }
    const auto Assignment = Families(State.Identity,SeatDraw,Draws); if (Assignment.Num()!=4) { return false; }
    for (int32 I=0; I<4; ++I) { Roster[I]->MadnessCore->AssignFamily(Assignment[I]); }
    State.ResonantId = Roster[SeatDraw%4]->EntityId; return true;
}
bool UDMElderOne::IsResonant(const ADMCombatant* A) const
{ return Authority() && IsValid(A) && A->GetWorld()==GetWorld() && !A->bIsEnemy && !State.ResonantId.IsEmpty() && A->EntityId==State.ResonantId; }
bool UDMElderOne::Select(uint32 Draw)
{
    if (!Authority() || State.bSelected) { return false; }
    State.bSelected = true; State.Identity = static_cast<EDMElderOne>(Draw%2); return true;
}
bool UDMElderOne::Begin()
{
    if (!Authority() || !State.bSelected || State.Phase != EDMBossPhase::Dormant) { return false; }
    State.Phase = EDMBossPhase::Rooted; return true;
}
bool UDMElderOne::Advance(EDMBossPhase Phase)
{
    if (!Authority() || !((State.Phase == EDMBossPhase::Rooted && Phase == EDMBossPhase::Mobile) || (State.Phase == EDMBossPhase::Mobile && Phase == EDMBossPhase::Frenzy))) { return false; }
    State.Phase = Phase; return true;
}
bool UDMElderOne::Finish(bool bVictory)
{
    if (!Authority() || State.Phase < EDMBossPhase::Rooted || State.Phase > EDMBossPhase::Frenzy) { return false; }
    State.Phase = bVictory ? EDMBossPhase::Victory : EDMBossPhase::Defeat; return true;
}
bool UDMElderOne::Restore(const FDMElderSnapshot& S)
{
    if (!Authority() || S.Version != 1 || static_cast<uint8>(S.Identity)>1 || static_cast<uint8>(S.Phase)>static_cast<uint8>(EDMBossPhase::Defeat)
        || (!S.bSelected && S.Phase != EDMBossPhase::Dormant)) { return false; }
    State = S; return true;
}
