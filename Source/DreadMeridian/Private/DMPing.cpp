#include "DMPing.h"

int32 FDMPingBoard::LifetimeTicks(EDMPingKind Kind)
{
    switch (Kind)
    {
    case EDMPingKind::Enemy: return 150;
    case EDMPingKind::Retreat: return 200;
    case EDMPingKind::Perceive: return 100;
    case EDMPingKind::Focus: case EDMPingKind::Ignore: case EDMPingKind::GoHere: case EDMPingKind::Defend:
    case EDMPingKind::Help: case EDMPingKind::Pickup: return 300;
    default: return 0;
    }
}

bool FDMPingBoard::NeedsTarget(EDMPingKind Kind)
{ return Kind == EDMPingKind::Enemy || Kind == EDMPingKind::Focus || Kind == EDMPingKind::Ignore || Kind == EDMPingKind::Help; }

/** Only Perceive forbids a target (forced subjective). Ground kinds accept a request carrying one but store a location only. */
bool FDMPingBoard::AllowsTarget(EDMPingKind Kind) { return Kind < EDMPingKind::Count && Kind != EDMPingKind::Perceive; }

const TCHAR* FDMPingBoard::KindName(EDMPingKind Kind)
{
    switch (Kind)
    {
    case EDMPingKind::Enemy: return TEXT("enemy");
    case EDMPingKind::Focus: return TEXT("focus");
    case EDMPingKind::Ignore: return TEXT("ignore");
    case EDMPingKind::GoHere: return TEXT("go_here");
    case EDMPingKind::Defend: return TEXT("defend");
    case EDMPingKind::Retreat: return TEXT("retreat");
    case EDMPingKind::Help: return TEXT("help");
    case EDMPingKind::Pickup: return TEXT("pickup");
    case EDMPingKind::Perceive: return TEXT("perceive");
    default: return TEXT("unknown");
    }
}

const TCHAR* FDMPingBoard::EndName(EDMPingEnd Reason)
{
    switch (Reason)
    {
    case EDMPingEnd::Expired: return TEXT("expired");
    case EDMPingEnd::Fulfilled: return TEXT("fulfilled");
    case EDMPingEnd::Cancelled: return TEXT("cancelled");
    case EDMPingEnd::Replaced: return TEXT("replaced");
    default: return TEXT("unknown");
    }
}

FVector FDMPingBoard::MarkerAnchor(EDMPingKind Kind, const FVector& Location)
{
    return Location + FVector(0, 0, NeedsTarget(Kind) ? 260.f : 30.f);
}

const FDMPing* FDMPingBoard::Find(int32 Id) const { return Pings.FindByPredicate([Id](const FDMPing& P) { return P.Id == Id; }); }
FDMPing* FDMPingBoard::Find(int32 Id) { return Pings.FindByPredicate([Id](const FDMPing& P) { return P.Id == Id; }); }

int32 FDMPingBoard::LiveCountFor(const FString& AuthorId) const
{
    int32 Count = 0;
    for (const FDMPing& P : Pings) { if (P.AuthorId == AuthorId) { ++Count; } }
    return Count;
}

int32 FDMPingBoard::Create(EDMPingKind Kind, const FString& AuthorId, bool bAuthorBot, FVector Location, const FString& TargetId, int32 Tick, TArray<FDMPingEnded>& Ended)
{
    if (AuthorId.IsEmpty() || Kind >= EDMPingKind::Count) { return INDEX_NONE; }
    if (NeedsTarget(Kind) && TargetId.IsEmpty()) { return INDEX_NONE; }
    if (!AllowsTarget(Kind) && !TargetId.IsEmpty()) { return INDEX_NONE; }
    const int32* Until = AuthorCooldownUntil.Find(AuthorId);
    if (Until && Tick < *Until) { return INDEX_NONE; }
    // Pings stay in creation order and ids are monotonic, so the first match is the author's oldest.
    int32 Replace = Pings.IndexOfByPredicate([&](const FDMPing& P) { return P.AuthorId == AuthorId && P.Kind == Kind; });
    if (Replace == INDEX_NONE && LiveCountFor(AuthorId) >= MaxPerAuthor)
    { Replace = Pings.IndexOfByPredicate([&](const FDMPing& P) { return P.AuthorId == AuthorId; }); }
    if (Replace != INDEX_NONE) { Ended.Add(FDMPingEnded{Pings[Replace], EDMPingEnd::Replaced}); Pings.RemoveAt(Replace); }
    FDMPing& Ping = Pings.AddDefaulted_GetRef();
    Ping.Id = NextId++;
    Ping.Kind = Kind;
    Ping.AuthorId = AuthorId;
    Ping.bAuthorBot = bAuthorBot;
    Ping.Location = Location;
    Ping.TargetId = NeedsTarget(Kind) ? TargetId : FString();
    Ping.CreatedTick = Tick;
    Ping.ExpiresTick = Tick + LifetimeTicks(Kind);
    Ping.bSubjective = Kind == EDMPingKind::Perceive;
    AuthorCooldownUntil.Add(AuthorId, Tick + AuthorCooldownTicks);
    return Ping.Id;
}

bool FDMPingBoard::Cancel(int32 Id, const FString& AuthorId, TArray<FDMPingEnded>& Ended)
{
    const int32 Index = Pings.IndexOfByPredicate([Id](const FDMPing& P) { return P.Id == Id; });
    if (Index == INDEX_NONE || AuthorId.IsEmpty() || Pings[Index].AuthorId != AuthorId) { return false; }
    Ended.Add(FDMPingEnded{Pings[Index], EDMPingEnd::Cancelled});
    Pings.RemoveAt(Index);
    return true;
}

bool FDMPingBoard::Acknowledge(int32 Id, const FString& WhoId)
{
    FDMPing* P = Find(Id);
    if (!P || WhoId.IsEmpty() || P->AuthorId == WhoId) { return false; }
    P->Acknowledged.AddUnique(WhoId);
    return true;
}

bool FDMPingBoard::Respond(int32 Id, const FString& ResponderId, bool bOnIt)
{
    FDMPing* P = Find(Id);
    if (!P || ResponderId.IsEmpty() || P->OnIt.Contains(ResponderId)) { return false; }
    if (bOnIt) { P->Busy.RemoveSingle(ResponderId); P->OnIt.Add(ResponderId); return true; }
    if (P->Busy.Contains(ResponderId)) { return false; }
    P->Busy.Add(ResponderId);
    return true;
}

bool FDMPingBoard::HasResponse(int32 Id, const FString& ResponderId) const
{
    const FDMPing* P = Find(Id);
    return P && (P->OnIt.Contains(ResponderId) || P->Busy.Contains(ResponderId));
}

void FDMPingBoard::Step(int32 Tick, TFunctionRef<bool(const FDMPing&)> IsFulfilled, TArray<FDMPingEnded>& Ended)
{
    for (int32 I = 0; I < Pings.Num();)
    {
        if (Pings[I].IsLive(Tick)) { ++I; continue; }
        Ended.Add(FDMPingEnded{Pings[I], EDMPingEnd::Expired});
        Pings.RemoveAt(I);
    }
    for (int32 I = 0; I < Pings.Num();)
    {
        if (!IsFulfilled(Pings[I])) { ++I; continue; }
        Ended.Add(FDMPingEnded{Pings[I], EDMPingEnd::Fulfilled});
        Pings.RemoveAt(I);
    }
}
