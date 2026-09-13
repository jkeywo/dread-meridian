#include "DMMechanicFixture.h"
#include "DMGrowthNetwork.h"
#include "DMCorruption.h"
#include "DMBossArena.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMGrowthTest,"DreadMeridian.Editor.ShubGrowths",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMGrowthTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld* W,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Arena=W->SpawnActor<ADMBossArena>(); Arena->BuildFixture(FVector(-700,0,0));
        auto* Field=W->SpawnActor<ADMCorruption>(); Field->Initialize(Arena);
        auto* Boss=M->SpawnEncounterActor(TEXT("test.shub"),FVector(-700,0,95),1000,0,true); Boss->bCommonEnemy=false; Boss->Resolve->Reset();
        auto* Growth=W->SpawnActor<ADMGrowthNetwork>(); TestTrue(TEXT("Growth network starts"),Growth->Initialize(Arena,Boss,Field,0));
        const int32 Tick=M->GetCombatTick(); Growth->Step(Tick+1);
        TestEqual(TEXT("One false and two genuine nodes"),Growth->Capture().GenuineIds.Num(),2);
        const int32 Footprint=Field->Capture().Cells.Num(); TestTrue(TEXT("Genuine sources spread"),Footprint>0);
        ADMCombatant* FalseNode=Growth->Nodes[0]; Hero->DealCombatDamage(FalseNode,10000,TEXT("test.false_growth")); Growth->Step(Tick+2);
        TestFalse(TEXT("False node does not expose boss"),Boss->bBreakVulnerable);
        ADMCombatant* RealNode=Growth->Nodes[1]; Hero->DealCombatDamage(RealNode,10000,TEXT("test.real_growth")); Growth->Step(Tick+3);
        TestTrue(TEXT("Genuine destruction exposes boss"),Boss->bBreakVulnerable); TestTrue(TEXT("Ground remains corrupted"),Field->Capture().Cells.Num()>=Footprint);
        for (ADMCombatant* A : M->GetCombatants())
        { if (!A->bIsEnemy) { int32 Clues=0; for (const auto& C : A->MadnessCore->View().Cues) { Clues+=C.Id.StartsWith(TEXT("resonance.")) ? 1 : 0; }
          TestEqual(TEXT("Only entitled investigator receives true-node cues"),Clues,M->ElderOne->IsResonant(A) ? 1 : 0); } }
        const auto Snapshot=Growth->Capture(); TestTrue(TEXT("Restore private node state"),Growth->Restore(Snapshot));
    })); return true;
}
#endif
