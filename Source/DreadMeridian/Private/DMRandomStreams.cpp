#include "DMRandomStreams.h"

FDMRandomStreams::FDMRandomStreams(int32 RunSeed)
{
    for (int32 Index = 0; Index < Count; ++Index)
    {
        // Fixed integer mixing: independent of FName/process hashes and registration order.
        uint32 Value = static_cast<uint32>(RunSeed) + 0x9e3779b9u * (Index + 1u);
        Value = (Value ^ (Value >> 16)) * 0x85ebca6bu;
        Value = (Value ^ (Value >> 13)) * 0xc2b2ae35u;
        Value ^= Value >> 16;
        Streams[Index].Initialize(static_cast<int32>(Value));
    }
}

uint32 FDMRandomStreams::Next(EDMRandomStream Stream)
{
    const int32 Index = static_cast<int32>(Stream);
    checkf(Index >= 0 && Index < Count, TEXT("Invalid deterministic stream ID"));
    return Streams[Index].GetUnsignedInt();
}

FDMRandomSnapshot FDMRandomStreams::Capture() const
{
    FDMRandomSnapshot Snapshot;
    for (const FRandomStream& Stream : Streams) { Snapshot.Seeds.Add(Stream.GetCurrentSeed()); }
    return Snapshot;
}

bool FDMRandomStreams::Restore(const FDMRandomSnapshot& Snapshot)
{
    if (Snapshot.SchemaVersion != 1 || Snapshot.Seeds.Num() != Count) { return false; }
    for (int32 Index = 0; Index < Count; ++Index) { Streams[Index].Initialize(Snapshot.Seeds[Index]); }
    return true;
}
