#pragma once
#include "CoreMinimal.h"
/** Authority-owned table; serialized with stable entity IDs, never actor addresses. */
struct FDMThreatSnapshot
{
    int32 Version = 1;
    TMap<FString,float> Values;
    FString OverrideId;
    int32 OverrideUntil = 0;
};
class DREADMERIDIAN_API FDMThreat
{
public:
    void Add(const FString& Id,float Amount)
    { if (!Id.IsEmpty() && FMath::IsFinite(Amount) && Amount>0) { State.Values.FindOrAdd(Id)=FMath::Min(100000000.f,FindRef(Id)+Amount); } }
    void Scale(const FString& Id,float Factor)
    { if (FMath::IsFinite(Factor) && Factor>=0 && State.Values.Contains(Id)) { State.Values[Id]=FMath::Clamp(FindRef(Id)*Factor,0.f,100000000.f); } }
    float FindRef(const FString& Id) const { return State.Values.FindRef(Id); }
    bool IsEmpty() const { return State.Values.IsEmpty(); }
    void Empty() { State = {}; }
    void AtLeastHighest(const FString& Id)
    { if (Id.IsEmpty()) { return; } float Max=0; for (const auto& Pair : State.Values) { Max=FMath::Max(Max,Pair.Value); } Add(Id,FMath::Max(0.f,Max+1-FindRef(Id))); }
    void Override(const FString& Id,int32 Until) { if (!Id.IsEmpty() && Until>0) { State.OverrideId=Id; State.OverrideUntil=Until; } }
    void ClearOverride() { State.OverrideId.Reset(); State.OverrideUntil=0; }
    FString Forced(int32 Tick) const { return Tick<State.OverrideUntil ? State.OverrideId : FString(); }
    FString Highest() const
    { FString Best; float Value=-1; for (const auto& Pair : State.Values) { if (Pair.Value>Value || (Pair.Value==Value && Pair.Key<Best)) { Best=Pair.Key; Value=Pair.Value; } } return Best; }
    void Cleanup(TFunctionRef<bool(const FString&)> Valid,int32 Tick)
    { for (auto It=State.Values.CreateIterator();It;++It) { if (!Valid(It.Key())) { It.RemoveCurrent(); } } if (Tick>=State.OverrideUntil || (!State.OverrideId.IsEmpty() && !Valid(State.OverrideId))) { ClearOverride(); } }
    FDMThreatSnapshot Capture() const { return State; }
    bool Restore(const FDMThreatSnapshot& S)
    {
        if (S.Version!=1 || S.OverrideUntil<0) { return false; }
        for (const auto& Pair : S.Values) { if (Pair.Key.IsEmpty() || !FMath::IsFinite(Pair.Value) || Pair.Value<0 || Pair.Value>100000000.f) { return false; } }
        State=S; return true;
    }
private:
    FDMThreatSnapshot State;
};
