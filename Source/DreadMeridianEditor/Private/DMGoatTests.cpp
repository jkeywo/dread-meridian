#include "DMMechanicFixture.h"
#include "DMShubMinion.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMGoatTest,"DreadMeridian.Editor.BlackGoatCharge",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMGoatTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld* W,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Goat=M->SpawnEncounterActor(TEXT("test.goat"),Hero->GetActorLocation()+FVector(500,0,0),220,12);
        auto* C=NewObject<UDMShubMinion>(Goat); Goat->AddInstanceComponent(C); C->RegisterComponent(); C->Initialize(EDMShubMinionKind::Goat);
        int32 Tick=M->GetCombatTick()+1; C->Step(Tick++);
        TestEqual(TEXT("Charge has a windup"),C->Capture().Action,EDMShubMinionAction::ChargeWindup);
        Goat->Resolve->AddPressure(1000,Hero,TEXT("test.break_charge")); C->Step(Tick++);
        TestEqual(TEXT("Break cancels committed charge"),C->Capture().Action,EDMShubMinionAction::Hunting);
        Goat->Resolve->Reset(); auto Ready=C->Capture(); Ready.Until=0; C->Restore(Ready); C->Step(Tick++);
        Tick+=20; C->Step(Tick++); TestEqual(TEXT("Charge executes after tell"),C->Capture().Action,EDMShubMinionAction::Charging);
        const FVector Before=Hero->GetActorLocation(); const float Pool=Hero->Health()+Hero->Shield();
        for (int32 I=0; I<20; ++I) { C->Step(Tick++); }
        TestTrue(TEXT("Charge deals damage"),Hero->Health()+Hero->Shield()<Pool);
        TestTrue(TEXT("Charge displaces formation"),FVector::DistSquared2D(Before,Hero->GetActorLocation())>1);
        TestEqual(TEXT("Charge creates no additional enemies"),M->FindCombatant(TEXT("test.goat.child.0")),static_cast<ADMCombatant*>(nullptr));
    })); return true;
}
#endif
