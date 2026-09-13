#include "DMMechanicFixture.h"
#include "DMRelicComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMCoinTest,"DreadMeridian.Editor.RelicCoin",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMCoinTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        Hero->Relics->Acquire(EDMRelic::Coin,TEXT("test.coin")); int32 Tick=M->GetCombatTick()+1; Hero->Relics->Step(Tick++);
        Hero->SetActorLocation(Hero->GetActorLocation()+FVector(400,0,0)); Hero->Relics->Step(Tick++);
        TestEqual(TEXT("Significant movement creates wake"),Hero->Relics->CaptureFull().Runtime.Wakes.Num(),1);
        ADMCombatant* Ally=nullptr; for (ADMCombatant* A : M->GetCombatants()) { if (!A->bIsEnemy && A!=Hero) { Ally=A; break; } }
        Ally->SetActorLocation(Hero->GetActorLocation()+FVector(0,80,0));
        auto* Enemy=M->SpawnEncounterActor(TEXT("test.coin_enemy"),Hero->GetActorLocation()+FVector(80,0,0),500,1); Enemy->bCommonEnemy=false; Enemy->Resolve->Reset();
        Hero->Relics->Step(Tick++); TestTrue(TEXT("Ally gains movement advantage"),Ally->Relics->MovementMultiplier()>1);
        TestTrue(TEXT("Enemy is slowed"),!Enemy->Slows.IsEmpty()); Enemy->Resolve->AddPressure(10,Hero,TEXT("test.coin_break")); TestTrue(TEXT("Enemy more susceptible to Break"),Enemy->Break>10);
        auto Snapshot=Hero->Relics->CaptureFull(); TestTrue(TEXT("Wake state restores"),Hero->Relics->RestoreFull(Snapshot));
        Hero->Relics->Step(Tick+100); TestTrue(TEXT("Wakes expire"),Hero->Relics->CaptureFull().Runtime.Wakes.IsEmpty());
    })); return true;
}
#endif
