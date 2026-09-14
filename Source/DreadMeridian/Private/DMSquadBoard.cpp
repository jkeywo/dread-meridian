#include "DMSquadBoard.h"

const TCHAR* FDMSquadBoard::KindName(EDMClaimKind Kind)
{
    switch (Kind)
    {
    case EDMClaimKind::Focus:   return TEXT("focus");
    case EDMClaimKind::Cast:    return TEXT("cast");
    case EDMClaimKind::Rescue:  return TEXT("rescue");
    case EDMClaimKind::Retreat: return TEXT("retreat");
    case EDMClaimKind::Ground:  return TEXT("ground");
    case EDMClaimKind::Peel:    return TEXT("peel");
    }
    return TEXT("unknown");
}

void FDMSquadBoard::Publish(const FDMSquadClaim& Claim)
{
    if (Claim.AuthorId.IsEmpty()) { return; }
    // Ground claims describe placed objects, so one author can hold several at once; they are keyed by serial.
    // Every other kind is a statement of current intent, of which an author has exactly one.
    const int32 Existing = Claims.IndexOfByPredicate([&Claim](const FDMSquadClaim& C)
    {
        return C.AuthorId == Claim.AuthorId && C.Kind == Claim.Kind
            && (C.Kind != EDMClaimKind::Ground || C.Serial == Claim.Serial);
    });
    if (Existing != INDEX_NONE) { Claims[Existing] = Claim; return; }
    Claims.Add(Claim);
}

void FDMSquadBoard::Step(int32 Tick)
{
    Claims.RemoveAll([Tick](const FDMSquadClaim& C) { return C.AgeTicks(Tick) >= LifetimeTicks; });
}

void FDMSquadBoard::ClearAuthor(const FString& AuthorId)
{
    Claims.RemoveAll([&AuthorId](const FDMSquadClaim& C) { return C.AuthorId == AuthorId; });
}

void FDMSquadBoard::Gather(EDMClaimKind Kind, const FString& ExceptAuthor, int32 Tick, TArray<FDMSquadClaim>& Out) const
{
    for (const FDMSquadClaim& C : Claims)
    {
        if (C.Kind != Kind || !Audible(C, ExceptAuthor, Tick)) { continue; }
        Out.Add(C);
    }
}

int32 FDMSquadBoard::FocusCount(int32 TargetIndex, const FString& ExceptAuthor, int32 Tick) const
{
    if (TargetIndex == INDEX_NONE) { return 0; }
    int32 Count = 0;
    for (const FDMSquadClaim& C : Claims)
    {
        if (C.Kind != EDMClaimKind::Focus || C.TargetIndex != TargetIndex) { continue; }
        if (!Audible(C, ExceptAuthor, Tick)) { continue; }
        ++Count;
    }
    return Count;
}

const FDMSquadClaim* FDMSquadBoard::FindOnTarget(EDMClaimKind Kind, int32 TargetIndex, const FString& ExceptAuthor, int32 Tick) const
{
    if (TargetIndex == INDEX_NONE) { return nullptr; }
    for (const FDMSquadClaim& C : Claims)
    {
        if (C.Kind != Kind || C.TargetIndex != TargetIndex) { continue; }
        if (!Audible(C, ExceptAuthor, Tick)) { continue; }
        return &C;
    }
    return nullptr;
}
