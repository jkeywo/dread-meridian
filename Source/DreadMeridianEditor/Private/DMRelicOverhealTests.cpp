#include "DMMechanicFixture.h"
#include "DMRelicComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMOverhealTest,"DreadMeridian.Editor.RelicOverheal",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMOverhealTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        Hero->Relics->Acquire(EDMRelic::Overheal,TEXT("test.overheal")); const float Base=Hero->Shield();
        Hero->HealHealth(100); TestEqual(TEXT("Overheal obeys Shield cap"),Hero->Shield(),Hero->MaxHealth()*.5f);
        const int32 Hold=Hero->Relics->CaptureFull().Runtime.ShieldHoldUntil; const float Full=Hero->Shield();
        Hero->Relics->Step(Hold-1); TestEqual(TEXT("Shield initially holds"),Hero->Shield(),Full);
        for (int32 Tick=Hold;Tick<Hold+100;++Tick) { Hero->Relics->Step(Tick); }
        TestEqual(TEXT("Decay removes only relic Shield"),Hero->Shield(),Base);
        Hero->HealHealth(100); auto* Enemy=M->SpawnEncounterActor(TEXT("test.shield_hit"),Hero->GetActorLocation()+FVector(100,0,0),100,10);
        Enemy->DealCombatDamage(Hero,10,TEXT("test.absorb"));
        const int32 NewHold=Hero->Relics->CaptureFull().Runtime.ShieldHoldUntil;
        auto Saved=Hero->Relics->CaptureFull(); TestTrue(TEXT("Budget restores"),Hero->Relics->RestoreFull(Saved));
        for (int32 Tick=FMath::Max(NewHold,Hold+101);Tick<Hold+220;++Tick) { Hero->Relics->Step(Tick); }
        TestEqual(TEXT("Consumed temporary Shield is not charged twice"),Hero->Shield(),Base);
    })); return true;
}
#endif
