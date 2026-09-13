#include "DMMechanicFixture.h"
#include "DMCorpse.h"
#include "DMShubMinion.h"
#include "EngineUtils.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMBroodTest,"DreadMeridian.Editor.BroodLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMBroodTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld* W,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Victim=M->SpawnEncounterActor(TEXT("test.food"),Hero->GetActorLocation()+FVector(100,0,0),30,0);
        Hero->DealCombatDamage(Victim,1000,TEXT("test.corpse")); ADMCorpse* Corpse=nullptr;
        for (TActorIterator<ADMCorpse> It(W);It;++It) { if (It->State.SourceId==Victim->EntityId) { Corpse=*It; } }
        if (!TestNotNull(TEXT("Accepted kill creates gameplay corpse"),Corpse)) { return; }
        auto* Brood=M->SpawnEncounterActor(TEXT("test.brood"),Victim->GetActorLocation()+FVector(0,60,0),60,5);
        auto* C=NewObject<UDMShubMinion>(Brood); Brood->AddInstanceComponent(C); C->RegisterComponent(); C->Initialize(EDMShubMinionKind::Broodling);
        int32 Tick=M->GetCombatTick()+1; C->Step(Tick++); TestEqual(TEXT("Feeding windup"),C->Capture().Action,EDMShubMinionAction::Feeding);
        FDMControl Interrupt; Interrupt.bInterrupt=true; Brood->ApplyControl(Interrupt,Hero,TEXT("test.interrupt")); C->Step(Tick++);
        TestFalse(TEXT("Interrupted feeding leaves corpse unconsumed"),Corpse->State.bConsumed);
        C->Step(Tick++); Tick+=21; C->Step(Tick++); TestTrue(TEXT("Completed feeding consumes corpse"),Corpse->State.bConsumed);
        TestEqual(TEXT("Corpse cannot be consumed twice"),Corpse->Consume(),0.f);
        C->Step(Tick++); TestEqual(TEXT("Split windup"),C->Capture().Action,EDMShubMinionAction::Splitting);
        Brood->ApplyControl(Interrupt,Hero,TEXT("test.split_interrupt")); C->Step(Tick++); TestFalse(TEXT("Interrupted split keeps parent"),Brood->IsDown());
        C->Step(Tick++); Tick+=26; const int32 Kills=M->GetMetrics().EnemyKills; C->Step(Tick++);
        TestTrue(TEXT("Split replaces parent"),Brood->IsDown()); TestEqual(TEXT("Replacement grants no kill"),M->GetMetrics().EnemyKills,Kills);
        auto* Child=M->FindCombatant(TEXT("test.brood.child.0")); auto* Other=M->FindCombatant(TEXT("test.brood.child.1"));
        if (!TestNotNull(TEXT("First offspring"),Child) || !TestNotNull(TEXT("Second offspring"),Other)) { return; }
        TestTrue(TEXT("Offspring injured"),Child->Health()<Child->MaxHealth()); auto* ChildState=Child->FindComponentByClass<UDMShubMinion>();
        ChildState->Step(Tick++); const float Healed=Child->Health(); TestTrue(TEXT("Fresh offspring heals"),Healed>30);
        Hero->DealCombatDamage(Child,3,TEXT("test.tag_offspring")); const float Tagged=Child->Health(); ChildState->Step(Tick++);
        TestEqual(TEXT("Damage stops healing"),Child->Health(),Tagged);
        Hero->DealCombatDamage(Child,1000,TEXT("test.brood_corpse"));
        for (TActorIterator<ADMCorpse> It(W);It;++It) { if (It->State.SourceId==Child->EntityId) { TestEqual(TEXT("Broodling corpse has low value"),It->State.Value,.1f); } }
        const auto S=C->Capture(); TestTrue(TEXT("Restore minion state"),C->Restore(S));
    })); return true;
}
#endif
