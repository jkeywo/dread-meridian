#include "DMMechanicFixture.h"
#include "DMShubEncounter.h"
#include "DMCorruption.h"
#include "DMBossArena.h"
#include "DMHealthAttributes.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMShubEncounterTest,"DreadMeridian.Editor.ShubEncounter",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMShubEncounterTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld* W,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        FDMElderSnapshot Selection; Selection.bSelected=true; Selection.Identity=EDMElderOne::Shub; M->ElderOne->Restore(Selection);
        TArray<ADMCombatant*> Roster; for (ADMCombatant* A : M->GetCombatants()) { if (!A->bIsEnemy) { Roster.Add(A); } }
        M->ElderOne->AssignResonance(Roster,0,{1,2,3,4});
        auto* Arena=W->SpawnActor<ADMBossArena>(); Arena->BuildFixture(FVector(-700,0,0));
        auto* Encounter=W->SpawnActor<ADMShubEncounter>(); TestTrue(TEXT("Start Shub in shared arena"),Encounter->Begin(Arena));
        TestFalse(TEXT("Cannot start twice"),Encounter->Begin(Arena));
        auto* Boss=Encounter->Boss.Get(); const int32 Tick=M->GetCombatTick()+1;
        Hero->DealCombatDamage(Boss,800,TEXT("test.mobile_threshold"));
        Encounter->Step(Tick); TestEqual(TEXT("Health transition enters mobile phase"),M->ElderOne->Phase(),EDMBossPhase::Mobile);
        Hero->DealCombatDamage(Boss,500,TEXT("test.frenzy_threshold"));
        Encounter->Step(Tick+1); TestEqual(TEXT("Final phase is birthing frenzy"),M->ElderOne->Phase(),EDMBossPhase::Frenzy);
        auto S=Encounter->Capture(); S.Move=EDMShubMove::BlackMilk; S.Until=Tick+2; S.Targets={Hero->GetActorLocation()};
        TestTrue(TEXT("Restore telegraphed Black Milk"),Encounter->Restore(S)); Encounter->Step(Tick+2);
        bool Corrupted=false; for (TActorIterator<ADMCorruption> It(W);It;++It) { Corrupted|=It->Contains(Hero->GetActorLocation()); }
        TestTrue(TEXT("Black Milk leaves persistent ground"),Corrupted);
        S=Encounter->Capture(); S.Move=EDMShubMove::CallTheBrood; S.Until=Tick+3; Encounter->Restore(S); Encounter->Step(Tick+3);
        TestNotNull(TEXT("Call the Brood creates ecosystem members"),M->FindCombatant(TEXT("shub.add.0")));
        Hero->SetActorLocation(Boss->GetActorLocation()+FVector(200,0,0)); const float Before=Hero->Health()+Hero->Shield();
        S=Encounter->Capture(); S.Move=EDMShubMove::HornedSweep; S.Until=Tick+4; Encounter->Restore(S); Encounter->Step(Tick+4);
        TestTrue(TEXT("Horned Sweep damages nearby investigators"),Hero->Health()+Hero->Shield()<Before);
        S=Encounter->Capture(); S.Move=EDMShubMove::TramplingAdvance; S.Until=Tick+5; S.Destination=Boss->GetActorLocation()+FVector(400,0,0);
        Encounter->Restore(S); Encounter->Step(Tick+5); TestTrue(TEXT("Trample starts movement"),Encounter->Capture().bCharging);
        Boss->Resolve->AddPressure(1000,Hero,TEXT("test.stop_trample")); Encounter->Step(Tick+6); TestFalse(TEXT("Break stops trample"),Encounter->Capture().bCharging);
        Hero->DealCombatDamage(Boss,10000,TEXT("test.boss_defeat")); Encounter->Step(Tick+7);
        TestEqual(TEXT("Boss death resolves victory"),M->ElderOne->Phase(),EDMBossPhase::Victory); TestFalse(TEXT("Encounter ends combat"),M->IsCombatActive());
    })); return true;
}
#endif

