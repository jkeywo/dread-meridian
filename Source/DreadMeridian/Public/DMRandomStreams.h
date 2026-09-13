#pragma once

#include "CoreMinimal.h"

/** IDs are serialized protocol values: append, never reorder or reuse. */
enum class EDMRandomStream : uint8
{
    RunGeneration, ElderOne, Madness, Relics, HTN, Director, Combat, AIChoice,
    Faction, BossVariation, Injury, Count
};

struct FDMRandomSnapshot
{
    int32 SchemaVersion = 2;
    TArray<int32> Seeds;
};

/** Same-engine reproducibility only; does not promise deterministic physics or navigation. */
class DREADMERIDIAN_API FDMRandomStreams
{
public:
    explicit FDMRandomStreams(int32 RunSeed);
    uint32 Next(EDMRandomStream Stream);
    FDMRandomSnapshot Capture() const;
    bool Restore(const FDMRandomSnapshot& Snapshot);

private:
    static constexpr int32 Count = static_cast<int32>(EDMRandomStream::Count);
    FRandomStream Streams[Count];
};
