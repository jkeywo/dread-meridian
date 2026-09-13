#include "DMObjectiveCatalogue.h"
TArray<FString> DMObjectiveCatalogue::Ids()
{
    return {TEXT("missing_fisherman"),TEXT("impossible_catch"),TEXT("bell_network"),TEXT("waterworks"),TEXT("main_pump"),TEXT("emergency_sluices"),TEXT("counter_sigil"),TEXT("ritual_components"),TEXT("bell_sequence"),TEXT("counter_ritualist"),TEXT("marsh_idols"),TEXT("smuggler_cache"),TEXT("lost_curio"),TEXT("surgery"),TEXT("doctor"),TEXT("lighthouse")};
}
bool DMObjectiveCatalogue::Build(const FString& Id, FVector Origin, int32 Difficulty, FDMObjectiveDefinition& Out)
{
    if (!Ids().Contains(Id) || Origin.ContainsNaN() || Difficulty < 0 || Difficulty > 4) { return false; }
    Out = {}; Out.Id = Id;
    auto Add = [&](EDMObjectiveVerb Verb, FVector Offset, const TCHAR* Text, int32 Duration = 20, FVector Destination = FVector::ZeroVector)
    {
        FDMObjectiveStep S; S.Verb = Verb; S.Location = Origin + Offset; S.Destination = Origin + Destination;
        S.Instruction = Text; S.WorkTicks = Duration; Out.Steps.Add(S);
    };
    const FVector A(0,0,0), B(500,0,0), C(250,450,0);
    if (Id == TEXT("missing_fisherman"))
    {
        Out.Title = TEXT("Find the Missing Fisherman"); Out.ChainStage = 0;
        Add(EDMObjectiveVerb::Inspect,A,TEXT("Inspect the tracks")); Add(EDMObjectiveVerb::Inspect,B,TEXT("Search the abandoned hut"));
        Add(EDMObjectiveVerb::Escort,C,TEXT("Escort the fisherman to the hut"),20,B);
    }
    else if (Id == TEXT("impossible_catch"))
    {
        Out.Title = TEXT("Examine the Impossible Catch"); Out.ChainStage = 0;
        Add(EDMObjectiveVerb::Inspect,A,TEXT("Inspect the abandoned boat")); Add(EDMObjectiveVerb::Operate,B,TEXT("Open the fish hold"),35);
        Add(EDMObjectiveVerb::Inspect,B,TEXT("Examine the anomalous catch"));
    }
    else if (Id == TEXT("bell_network") || Id == TEXT("waterworks"))
    {
        const bool Bell = Id == TEXT("bell_network"); Out.Title = Bell ? TEXT("Inspect the Bell Network") : TEXT("Trace the Waterworks"); Out.ChainStage = 1;
        for (const auto& P : {A,B,C}) { Add(Bell ? EDMObjectiveVerb::Inspect : EDMObjectiveVerb::Operate,P,Bell ? TEXT("Inspect the signal site") : TEXT("Trace the altered flow"),25); }
    }
    else if (Id == TEXT("main_pump"))
    {
        Out.Title = TEXT("Restore the Main Pump"); Out.ChainStage = 2; Out.Reward = EDMObjectiveReward::DrainBasin;
        Add(EDMObjectiveVerb::Operate,A,TEXT("Repair the main pump"),50); Add(EDMObjectiveVerb::Defend,A,TEXT("Defend the running pump"),100);
    }
    else if (Id == TEXT("emergency_sluices"))
    {
        Out.Title = TEXT("Open the Emergency Sluices"); Out.ChainStage = 2; Out.Reward = EDMObjectiveReward::DrainBasin;
        for (const auto& P : {A,B,C}) { Add(EDMObjectiveVerb::Operate,P,TEXT("Open the remote sluice"),30); }
    }
    else if (Id == TEXT("counter_sigil"))
    {
        Out.Title = TEXT("Charge the Counter-Sigil"); Out.ChainStage = 3;
        for (const auto& P : {A,B,C}) { Add(EDMObjectiveVerb::Operate,P,TEXT("Activate the sigil point")); Add(EDMObjectiveVerb::Defend,P,TEXT("Hold the charged sigil"),40); }
    }
    else if (Id == TEXT("ritual_components"))
    {
        Out.Title = TEXT("Assemble Ritual Components"); Out.ChainStage = 3;
        Add(EDMObjectiveVerb::Carry,B,TEXT("Carry the component to its socket"),20,A);
        Add(EDMObjectiveVerb::Carry,C,TEXT("Carry the second component to its socket"),20,A);
        Add(EDMObjectiveVerb::Defend,A,TEXT("Defend the assembled components"),50);
    }
    else if (Id == TEXT("bell_sequence"))
    {
        Out.Title = Difficulty == 4 ? TEXT("Bells of Manifestation") : TEXT("Break the Bell Sequence"); Out.bDisruption = true;
        const int32 Sites = Difficulty >= 3 ? 3 : 1;
        for (int32 Site = 0; Site < Sites; ++Site)
        {
            Add(EDMObjectiveVerb::Sequence,Site == 0 ? A : Site == 1 ? B : C,TEXT("Observe the bells, then repeat their sequence"));
            Out.Steps.Last().Sequence = Difficulty >= 2 ? TArray<int32>{1,3,2,1,2,3} : TArray<int32>{1,3,2};
        }
    }
    else if (Id == TEXT("counter_ritualist") || Id == TEXT("doctor"))
    {
        const bool Doctor = Id == TEXT("doctor"); Out.Title = Doctor ? TEXT("Rescue the Doctor") : Difficulty == 4 ? TEXT("Last Counter-Rite") : TEXT("Protect the Counter-Ritualist");
        Out.bDisruption = !Doctor; Out.Reward = Doctor ? EDMObjectiveReward::Treatment : EDMObjectiveReward::None;
        Add(EDMObjectiveVerb::Escort,A,TEXT("Stay near and defend the moving escort"),20,B);
        Add(EDMObjectiveVerb::Defend,B,TEXT("Defend while the escort works"),40);
        if (!Doctor && Difficulty >= 2)
        { Add(EDMObjectiveVerb::Escort,B,TEXT("Escort to the next ward"),20,C); Add(EDMObjectiveVerb::Defend,C,TEXT("Defend the warding stop"),60); }
        if (!Doctor && Difficulty >= 3)
        { Add(EDMObjectiveVerb::Escort,C,TEXT("Escort to the final ward"),20,A); Add(EDMObjectiveVerb::Defend,A,TEXT("Protect the final rite"),80); }
    }
    else if (Id == TEXT("marsh_idols"))
    {
        Out.Title = Difficulty == 4 ? TEXT("Anchors of the Apocalypse") : TEXT("Shatter the Marsh Idols"); Out.bDisruption = true;
        for (const auto& P : {A,B,C}) { Add(EDMObjectiveVerb::Destroy,P,TEXT("Destroy the static marsh idol")); }
    }
    else if (Id == TEXT("smuggler_cache") || Id == TEXT("surgery"))
    {
        const bool Cache = Id == TEXT("smuggler_cache"); Out.Title = Cache ? TEXT("Smuggler Cache") : TEXT("Open the Surgery");
        Out.Reward = Cache ? EDMObjectiveReward::Relic : EDMObjectiveReward::Treatment;
        Add(EDMObjectiveVerb::Defend,A,TEXT("Clear and hold the site"),80); Add(EDMObjectiveVerb::Operate,A,Cache ? TEXT("Open the cache") : TEXT("Open the treatment station"));
    }
    else if (Id == TEXT("lost_curio"))
    {
        Out.Title = TEXT("Lost Curio"); Out.Reward = EDMObjectiveReward::Relic;
        Add(EDMObjectiveVerb::Carry,A,TEXT("Carry the curio to the inspection point"),20,B);
        Add(EDMObjectiveVerb::Inspect,B,TEXT("Inspect the recovered curio"));
    }
    else
    {
        Out.Title = TEXT("Restore the Lighthouse"); Out.Reward = EDMObjectiveReward::Vision;
        Add(EDMObjectiveVerb::Operate,A,TEXT("Repair the lighthouse machinery"),60); Add(EDMObjectiveVerb::Operate,B,TEXT("Light the beacon"),30);
    }
    return true;
}
