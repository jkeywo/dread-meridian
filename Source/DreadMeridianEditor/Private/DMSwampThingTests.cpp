#include "DMMechanicFixture.h"
#include "DMSwampThing.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
ADMCombatant* SpawnSwamp(ADMCombatGameMode* M,ADMCombatant* Hero,EDMSwampThing Role,float Distance=350)
{
    auto* A=M->SpawnEncounterActor(TEXT("test.swamp"),Hero->GetActorLocation()+FVector(Distance,0,0),500,8);
    A->Swamp->Initialize(Role); A->GetCharacterMovement()->DisableMovement(); return A;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMCrawlerTest,"DreadMeridian.Editor.Swamp.Crawler",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMCrawlerTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* A=SpawnSwamp(M,Hero,EDMSwampThing::Crawler,100);
        TestTrue(TEXT("Isolation increases pressure"),A->Swamp->IsIsolated(Hero));
        float Before=(Hero->Health()+Hero->Shield()); A->DealCombatDamage(Hero,10,TEXT("test.crawler"),true); const float Alone=Before-(Hero->Health()+Hero->Shield());
        for (ADMCombatant* Ally : M->GetCombatants()) { if (!Ally->bIsEnemy && Ally!=Hero) { Ally->SetActorLocation(Hero->GetActorLocation()+FVector(0,120,0)); break; } }
        Before=(Hero->Health()+Hero->Shield()); A->DealCombatDamage(Hero,10,TEXT("test.crawler"),true); TestTrue(TEXT("Grouping reduces accepted damage"),Before-(Hero->Health()+Hero->Shield())<Alone);
        TestTrue(TEXT("Fast movement projects"),A->Swamp->Speed()>420); TestEqual(TEXT("Melee range"),A->GetAttackRange(),150.f);
        auto* Human=M->SpawnEncounterActor(TEXT("test.human"),A->GetActorLocation()+FVector(100,0,0),500,1); Human->bHumanEnemy=true;
        TestTrue(TEXT("Mutual swamp/native-human hostility"),A->IsHostileTo(Human) && Human->IsHostileTo(A));
        TestTrue(TEXT("Human can damage swamp"),Human->DealCombatDamage(A,5,TEXT("test.factions")));
        TestTrue(TEXT("Swamp can damage human"),A->DealCombatDamage(Human,5,TEXT("test.factions")));
        A->Threat.Add(Human->EntityId,100000000); A->Threat.Override(Hero->EntityId,M->GetCombatTick()+20); A->Swamp->Step(M->GetCombatTick()+1);
        TestEqual(TEXT("Explicit override outranks even maximum threat"),A->GetAttackTarget(),Hero);
        auto* Kin=M->SpawnEncounterActor(TEXT("test.kin"),A->GetActorLocation(),100,1); Kin->Swamp->Initialize(EDMSwampThing::Crawler);
        TestFalse(TEXT("No swamp friendly fire"),A->DealCombatDamage(Kin,5,TEXT("test.factions")));
        Hero->SetActorLocation(A->GetActorLocation()+FVector(2000,0,0)); A->Swamp->Step(M->GetCombatTick()+2);
        TestTrue(TEXT("Territory prevents distant investigator pursuit"),A->GetAttackTarget()!=Hero);
    })); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMLurkerTest,"DreadMeridian.Editor.Swamp.Lurker",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMLurkerTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* A=SpawnSwamp(M,Hero,EDMSwampThing::Lurker); TestTrue(TEXT("Ambush commits"),A->Swamp->TrySignature(Hero));
        TestTrue(TEXT("Committing ambusher is targetable"),Hero->DealCombatDamage(A,5,TEXT("test.counter_ambush")));
        auto S=A->Swamp->Capture(); const float Before=(Hero->Health()+Hero->Shield()); A->Swamp->Step(S.ResolveAt);
        TestTrue(TEXT("Opening attack slows"),!Hero->Slows.IsEmpty()); TestTrue(TEXT("Opener is modest damage"),Before-(Hero->Health()+Hero->Shield())<=6.1f);
        S.ReadyAt=0; TestTrue(TEXT("Windup restoration"),A->Swamp->Restore(S)); Hero->SetActorLocation(Hero->GetActorLocation()+FVector(0,300,0));
        const float Evaded=(Hero->Health()+Hero->Shield()); A->Swamp->Step(S.ResolveAt); TestEqual(TEXT("Leaving tell evades opener"),(Hero->Health()+Hero->Shield()),Evaded);
    })); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMSpitterTest,"DreadMeridian.Editor.Swamp.Spitter",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMSpitterTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* A=SpawnSwamp(M,Hero,EDMSwampThing::Spitter); TestTrue(TEXT("Spit telegraphs"),A->Swamp->TrySignature(Hero));
        const int32 Tick=A->Swamp->Capture().ResolveAt; A->Swamp->Step(Tick); TestTrue(TEXT("Persistent pool created"),A->Swamp->Capture().PoolUntil>Tick);
        float Before=(Hero->Health()+Hero->Shield()); A->Swamp->Step(Tick+1); TestTrue(TEXT("Standing in pool damages and slows"),(Hero->Health()+Hero->Shield())<Before && !Hero->Slows.IsEmpty());
        Before=(Hero->Health()+Hero->Shield()); A->Swamp->Step(Tick+1); TestEqual(TEXT("Repeated tick cannot double damage"),(Hero->Health()+Hero->Shield()),Before);
        const auto S=A->Swamp->Capture(); TestTrue(TEXT("Pool restores"),A->Swamp->Restore(S));
        Hero->SetActorLocation(Hero->GetActorLocation()+FVector(0,400,0)); A->Swamp->Step(Tick+12); TestEqual(TEXT("Leaving pool stops damage"),(Hero->Health()+Hero->Shield()),Before);
        A->Swamp->Step(S.PoolUntil); TestEqual(TEXT("Terrain expires"),A->Swamp->Capture().PoolUntil,0);
        auto Bad=S; Bad.PoolUntil=-1; TestFalse(TEXT("Reject malformed restoration"),A->Swamp->Restore(Bad));
    })); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMGrasperTest,"DreadMeridian.Editor.Swamp.Grasper",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMGrasperTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* A=SpawnSwamp(M,Hero,EDMSwampThing::Grasper,450); TestTrue(TEXT("Pull telegraphs"),A->Swamp->TrySignature(Hero)); const auto S=A->Swamp->Capture();
        const FVector Start=Hero->GetActorLocation(); A->Swamp->Step(S.ResolveAt); const float Pull=FVector::Dist2D(Start,Hero->GetActorLocation()); TestTrue(TEXT("Pull moves target toward caster"),Pull>50);
        Hero->SetActorLocation(Start); Hero->BraceResistance=.8f; A->Swamp->Restore(S); A->Swamp->Step(S.ResolveAt);
        TestTrue(TEXT("Resistance reduces pull"),FVector::Dist2D(Start,Hero->GetActorLocation())<Pull); Hero->BraceResistance=0;
        Hero->SetActorLocation(Start); A->Swamp->Restore(S); FDMControl C; C.bInterrupt=true; A->ApplyControl(C,Hero,TEXT("test.interrupt_grasp")); A->Swamp->Step(S.ResolveAt);
        TestEqual(TEXT("Interrupted pull does not move target"),Hero->GetActorLocation(),Start); TestEqual(TEXT("Interrupted windup clears"),A->Swamp->Capture().ResolveAt,0);
        auto Moved=S; Moved.InterruptSerial=A->ControlInterruptSerial; A->Swamp->Restore(Moved); A->SetActorLocation(A->GetActorLocation()+FVector(0,200,0)); A->Swamp->Step(S.ResolveAt);
        TestEqual(TEXT("Moving caster out of tell cancels pull"),A->Swamp->Capture().ResolveAt,0); TestEqual(TEXT("Displaced caster cannot pull from new origin"),Hero->GetActorLocation(),Start);
    })); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMOldThingTest,"DreadMeridian.Editor.Swamp.OldThing",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMOldThingTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* A=SpawnSwamp(M,Hero,EDMSwampThing::OldThing,300); TestTrue(TEXT("Elite has protected Resolve"),A->Resolve->IsProtected());
        TestTrue(TEXT("Territorial sweep telegraphs"),A->Swamp->TrySignature(Hero)); const auto S=A->Swamp->Capture(); const FVector Start=Hero->GetActorLocation();
        A->Swamp->Step(S.ResolveAt); TestTrue(TEXT("Sweep displaces isolated target"),FVector::Dist2D(Start,Hero->GetActorLocation())>100);
        Hero->SetActorLocation(Start); A->Swamp->Restore(S); A->Resolve->AddPressure(1000,Hero,TEXT("test.break_old_thing")); A->Swamp->Step(S.ResolveAt);
        TestTrue(TEXT("Break cancels elite signature"),A->bBreakVulnerable && A->Swamp->Capture().ResolveAt==0); TestEqual(TEXT("Broken sweep causes no displacement"),Hero->GetActorLocation(),Start);
    })); return true;
}
#endif
