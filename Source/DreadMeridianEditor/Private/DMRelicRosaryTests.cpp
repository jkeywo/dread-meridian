#include "DMMechanicFixture.h"
#include "DMRelicComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMRosaryTest,"DreadMeridian.Editor.RelicRosary",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMRosaryTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld*,ADMCombatGameMode* M,ADMCombatant*)
    {
        for (ADMCombatant* A : M->GetCombatants())
        {
            if (A->bIsEnemy) { continue; } A->MadnessCore->Reset(); A->Relics->Acquire(EDMRelic::Rosary,TEXT("test.rosary.")+A->EntityId);
            A->MadnessCore->Add(25,TEXT("test.tier")); TestTrue(TEXT("Tier crossing boosts resource"),A->Relics->ResourceMultiplier()>1);
            switch (A->Investigator->Kind)
            {
            case EDMInvestigator::Sapper: A->Investigator->Charges=0; A->Investigator->Components=0; A->Investigator->CollectComponent(); TestEqual(TEXT("Sapper component effectiveness"),A->Investigator->Charges,1); break;
            case EDMInvestigator::Photographer: A->Investigator->AddExposure(TEXT("subject"),10,false,M->GetCombatTick()); TestEqual(TEXT("Photographer Exposure generation"),A->Investigator->PeekExposure(TEXT("subject")),20.f); break;
            case EDMInvestigator::Medium: A->Investigator->BindSpirit(TEXT("spirit"),A->EntityId,A->GetActorLocation()); A->Investigator->AddAttention(TEXT("spirit"),10); TestEqual(TEXT("Medium Attention generation"),A->Investigator->PeekAttention(TEXT("spirit")),20.f); break;
            case EDMInvestigator::Smuggler: A->Investigator->Momentum=0; A->Investigator->Pressure(M->GetCombatTick(),5); TestEqual(TEXT("Smuggler Momentum generation"),A->Investigator->Momentum,10.f); break;
            default: break;
            }
            TestEqual(TEXT("Relic does not reduce Madness"),A->MadnessCore->View().Current,25.f);
            A->Relics->Step(A->Relics->CaptureFull().Runtime.RosaryUntil); TestEqual(TEXT("Boost expires"),A->Relics->ResourceMultiplier(),1.f);
        }
    })); return true;
}
#endif
