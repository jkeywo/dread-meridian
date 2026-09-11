#include "DMGameState.h"
#include "Net/UnrealNetwork.h"

namespace
{
bool SamePing(const FDMPing& A, const FDMPing& B)
{
    return A.Id == B.Id && A.Kind == B.Kind && A.AuthorId == B.AuthorId && A.bAuthorBot == B.bAuthorBot && A.Location == B.Location
        && A.TargetId == B.TargetId && A.CreatedTick == B.CreatedTick && A.ExpiresTick == B.ExpiresTick && A.bSubjective == B.bSubjective
        && A.OnIt == B.OnIt && A.Busy == B.Busy && A.Acknowledged == B.Acknowledged;
}
}

void ADMGameState::Publish(const FDMRunState& State)
{
    if (!HasAuthority()) { return; }
    RunState = State;
    ForceNetUpdate();
    OnRep_RunState();
}

void ADMGameState::SetCombatTick(int32 Tick)
{
    if (HasAuthority()) { CombatTick = Tick; }
}

void ADMGameState::OnRep_RunState()
{
    OnRunStateChanged.Broadcast(RunState);
}

void ADMGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADMGameState, EncounterObjective);
    DOREPLIFETIME(ADMGameState, RunState);
    DOREPLIFETIME(ADMGameState, CombatTick);
    DOREPLIFETIME(ADMGameState, Pings);
    DOREPLIFETIME(ADMGameState, ShellPhase);
    DOREPLIFETIME(ADMGameState, bShellVictory);
}

void ADMGameState::SetEncounterObjective(const FString& Text)
{ if (HasAuthority() && EncounterObjective != Text) { EncounterObjective=Text; ForceNetUpdate(); } }

void ADMGameState::SetPings(const TArray<FDMPing>& Live)
{
    if (!HasAuthority()) { return; }
    bool bSame = Live.Num() == Pings.Num();
    for (int32 I = 0; bSame && I < Live.Num(); ++I) { bSame = SamePing(Live[I], Pings[I]); }
    if (bSame) { return; }
    Pings = Live;
    ForceNetUpdate();
}

void ADMGameState::SetShellPhase(EDMShellPhase Phase, bool bVictory)
{
    if (!HasAuthority() || (ShellPhase == Phase && bShellVictory == bVictory)) { return; }
    ShellPhase = Phase;
    bShellVictory = bVictory;
    ForceNetUpdate();
}
