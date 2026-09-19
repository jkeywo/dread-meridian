#include "DMMechanicFixture.h"
#include "DMFishingVillage.h"
#include "DMObjective.h"
#include "DMGameState.h"
#include "DMShubEncounter.h"
#include "DMElderOne.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMFishingVillageTest,"DreadMeridian.Editor.FishingVillage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMFishingVillageTest::RunTest(const FString&)
{
    for (bool Premature : {false,true})
    {
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this,Premature](UWorld* W,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* V=M->Village.Get(); if (!TestNotNull(TEXT("Real village map starts its scenario"),V)) { return; }
        TestTrue(TEXT("Village mode selected"),M->IsFishingVillage());
        TestEqual(TEXT("Objective phase precedes manifestation"),W->GetGameState<ADMGameState>()->GetRunState().Phase,EDMRunPhase::Expedition);
        TestFalse(TEXT("Expedition cannot win before boss phase"),M->FinishRun(true));
        TestEqual(TEXT("Fixed scenario selects Shub before resonance"),M->ElderOne->Identity(),EDMElderOne::Shub);
        TestEqual(TEXT("Three required disruptions"),V->Disruptions.Num(),3);
        TestEqual(TEXT("Three optional power-ups"),V->Optional.Num(),3);
        TestEqual(TEXT("Fixed roster includes nine enemies and three idols"),M->GetCombatants().Num(),16);
        const FVector From(400,600,95),To(1600,600,95);
        TestFalse(TEXT("Flood blocks basin route"),V->RouteClear(From,To));
        FHitResult Hit; FCollisionQueryParams Q; Q.AddIgnoredActor(Hero);
        TestTrue(TEXT("Flood has actual blocking collision"),W->LineTraceSingleByChannel(Hit,From,To,ECC_Visibility,Q));
        const FVector CornerFrom(-2300,-700,95),CornerTo(-1600,-700,95);
        const FVector Around=V->Waypoint(CornerFrom,CornerTo);
        TestTrue(TEXT("Building route finds a reachable detour"),!Around.Equals(CornerFrom) && !Around.Equals(CornerTo) && V->RouteClear(CornerFrom,Around));
        TestTrue(TEXT("Pawn can leave building clearance margin"),V->RouteClear(FVector(-2085,-700,95),CornerFrom));
        const int32 StartTick=M->GetCombatTick();
        V->StepRitual(StartTick+29);
        TestEqual(TEXT("No partial point"),W->GetGameState<ADMGameState>()->GetRunState().RitualProgress,0);
        V->StepRitual(StartTick+30); V->StepRitual(StartTick+30); V->StepRitual(StartTick+10);
        TestEqual(TEXT("Clock ignores duplicate and old ticks"),W->GetGameState<ADMGameState>()->GetRunState().RitualProgress,1);
        if (Premature)
        {
            const int32 InitialWork=V->Core->Steps[0].WorkTicks;
            V->StepRitual(StartTick+3000);
            TestEqual(TEXT("Stirring clock boundary"),W->GetGameState<ADMGameState>()->GetRunState().RitualStage,EDMRitualStage::Stirring);
            TestTrue(TEXT("Stirring increases mandatory work"),V->Core->Steps[0].WorkTicks>InitialWork);
            const int32 Work=V->Core->Steps[0].WorkTicks;
            V->StepRitual(StartTick+3000);
            TestEqual(TEXT("Stage effects cannot apply twice"),V->Core->Steps[0].WorkTicks,Work);
            V->StepRitual(StartTick+6000);
            TestEqual(TEXT("Intrusion clock boundary"),W->GetGameState<ADMGameState>()->GetRunState().RitualStage,EDMRitualStage::Intrusion);
            TestEqual(TEXT("Intrusion adds escort ward"),V->Disruptions[1]->Steps.Num(),4);
            V->StepRitual(StartTick+9000);
            TestEqual(TEXT("Convergence clock boundary"),W->GetGameState<ADMGameState>()->GetRunState().RitualStage,EDMRitualStage::Convergence);
            TestEqual(TEXT("Convergence requires three bell sites"),V->Disruptions[0]->Steps.Num(),3);
            V->StepRitual(StartTick+12000); V->StepScenario();
            TestTrue(TEXT("Clock manifests while objectives outstanding"),V->bManifested && V->Encounter && V->Encounter->Boss);
            TestFalse(TEXT("Premature manifestation preserves pump requirement"),V->bDrained);
            TestTrue(TEXT("Current core converted"),V->Core->PublicState.bApocalypse);
            for (ADMObjective* O : V->Disruptions)
            { TestTrue(TEXT("Disruption converted"),O->PublicState.bApocalypse); TestEqual(TEXT("Actual harder mechanics"),O->Difficulty,4); }
            if (!V->Encounter || !V->Encounter->Boss) { return; }
            Hero->DealCombatDamage(V->Encounter->Boss,100000,TEXT("test.ritual_gate"));
            TestFalse(TEXT("Boss cannot die before mandatory work"),V->Encounter->Boss->IsDown());
            // This fixture drives objectives without live AI; keep the boss away from ward sites.
            V->Encounter->Boss->SetActorLocation(FVector(4000,4500,95));
        }
        else
        {
            const int32 Work=V->Core->Steps[0].WorkTicks;
            V->Core->Fail(); V->StepScenario();
            TestEqual(TEXT("Failed mandatory objective repairs"),V->Core->PublicState.State,EDMObjectiveState::Repaired);
            TestTrue(TEXT("Repair requires more work"),V->Core->Steps[0].WorkTicks>Work);
            TestEqual(TEXT("Failure adds pressure once"),W->GetGameState<ADMGameState>()->GetRunState().RitualProgress,11);
            V->StepScenario();
            TestEqual(TEXT("Repair cannot double count"),W->GetGameState<ADMGameState>()->GetRunState().RitualProgress,11);
        }
        int32 Tick=StartTick+5;
        auto Complete=[&](ADMObjective* O)
        {
            if (!TestNotNull(TEXT("Objective exists"),O)) { return; }
            for (int32 N=0;N<2500 && !O->IsTerminal();++N)
            {
                const auto* S=O->Current(); if (!S) { break; }
                Hero->SetActorLocation(O->PublicState.PayloadLocation+FVector(0,-70,75));
                if (S->Verb==EDMObjectiveVerb::Destroy) { Hero->DealCombatDamage(O->Destructible,10000,TEXT("test.village_idol")); }
                else if (S->Verb==EDMObjectiveVerb::Sequence)
                { if (O->bSequenceReady && S->Sequence.IsValidIndex(O->PublicState.Progress)) { O->Interact(Hero,S->Sequence[O->PublicState.Progress]); } }
                else if (!O->IsInteracting(Hero)) { O->Interact(Hero); }
                O->Step(++Tick);
            }
            TestEqual(FString::Printf(TEXT("Accepted interactions complete %s step %d"),*O->PublicState.Id,O->PublicState.Step),O->PublicState.State,EDMObjectiveState::Completed);
        };
        Complete(V->Optional[0]); V->StepScenario();
        TestEqual(TEXT("Optional lighthouse cannot change manifestation"),V->bManifested,Premature);
        TestFalse(TEXT("Optional reward does not drain basin"),V->bDrained);
        for (int32 Stage=0;Stage<4;++Stage)
        {
            TestEqual(TEXT("Core progresses sequentially"),V->CoreStage,Stage);
            Complete(V->Core); V->StepScenario();
            TestEqual(TEXT("Core chain alone cannot bypass disruptions"),V->bManifested,Premature);
            if (Premature && Stage<3) { TestTrue(TEXT("Future core also converts"),V->Core->PublicState.bApocalypse); }
            if (Stage==2) { TestTrue(TEXT("Pump drains basin"),V->bDrained); TestTrue(TEXT("Drain changes route topology"),V->RouteClear(From,To)); TestFalse(TEXT("Drain removes actual collision"),W->LineTraceSingleByChannel(Hit,From,To,ECC_Visibility,Q)); }
        }
        for (int32 I=0;I<3;++I) { Complete(V->Disruptions[I]); V->StepScenario(); if (I<2) { TestEqual(TEXT("Every disruption is required"),V->bManifested,Premature); } }
        if (!Premature)
        {
            TestFalse(TEXT("Completion allows preparation instead of automatic summon"),V->bManifested);
            TestNotNull(TEXT("Summon interaction becomes available"),V->Manifestation.Get());
            Complete(V->Manifestation); V->StepScenario();
        }
        TestTrue(TEXT("Scenario summons actual encounter"),V->bManifested && V->Encounter && V->Encounter->Boss);
        if (!V->Encounter || !V->Encounter->Boss) { return; }
        TestEqual(TEXT("Manifestation advances public run phase"),W->GetGameState<ADMGameState>()->GetRunState().Phase,EDMRunPhase::Apocalypse);
        const int32 Count=M->GetCombatants().Num(); V->StepScenario(); TestEqual(TEXT("Boss cannot spawn twice"),M->GetCombatants().Num(),Count);
        Hero->DealCombatDamage(V->Encounter->Boss,10000,TEXT("test.village_boss_defeat")); V->Encounter->Step(++Tick);
        TestEqual(TEXT("Boss defeat completes scenario victory"),W->GetGameState<ADMGameState>()->GetRunState().Phase,EDMRunPhase::Victory);
        V->StepRitual(StartTick+24000); V->StepScenario();
        TestEqual(TEXT("Terminal run stays terminal"),W->GetGameState<ADMGameState>()->GetRunState().Phase,EDMRunPhase::Victory);
    },TEXT("/Game/DreadMeridian/Maps/L_FishingVillage")));
    }
    return true;
}
#endif
