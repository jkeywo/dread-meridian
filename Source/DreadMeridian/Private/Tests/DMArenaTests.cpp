#include "DMTestArena.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMArenaConfigTest,"DreadMeridian.Foundation.TestArena.Config",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FDMArenaConfigTest::RunTest(const FString& Parameters)
{
    FDMArenaConfig C;
    TestTrue(TEXT("Solo is valid"),C.IsValid());
    C.Companions={EDMInvestigator::Photographer,EDMInvestigator::Medium,EDMInvestigator::Smuggler};
    TestTrue(TEXT("Four unique investigators valid"),C.IsValid());
    C.Companions.Add(EDMInvestigator::Sapper); TestFalse(TEXT("No fifth / duplicated player"),C.IsValid()); C.Companions.Pop();
    C.Companions[1]=EDMInvestigator::Photographer; TestFalse(TEXT("No duplicate companions"),C.IsValid()); C.Companions.Reset();
    C.Player=EDMInvestigator::None; TestFalse(TEXT("Player required"),C.IsValid()); C.Player=EDMInvestigator::Sapper;
    for (int32 I=0;I<32;++I) { FDMArenaPlacement P; P.Position=FVector(0,0,95); C.Placements.Add(P); }
    TestTrue(TEXT("32 is the configuration limit"),C.IsValid()); const FDMArenaPlacement Extra=C.Placements[0]; C.Placements.Add(Extra);
    TestFalse(TEXT("33 exceeds limit"),C.IsValid()); C.Placements.Pop();
    C.Placements[0].Type=EDMArenaEnemy::Count; TestFalse(TEXT("Invalid enemy rejected"),C.IsValid());
    C.Placements[0].Type=EDMArenaEnemy::Gunman; C.Placements[0].Position.X=5000; TestFalse(TEXT("Outside arena rejected"),C.IsValid());
    for (int32 N=1;N<=32;++N)
    {
        for (int32 I=0;I<N;++I) { for (int32 J=0;J<I;++J)
        { TestTrue(TEXT("Formation capsules cannot overlap"),FVector::DistSquared(DMArena::FormationOffset(I,N),DMArena::FormationOffset(J,N))>=FMath::Square(150.)); } }
    }
    const int32 Expected[]={3,3,3,2,5,4};
    for (int32 I=0;I<6;++I) { TestEqual(TEXT("Preset composition size"),DMArena::Preset(I).Num(),Expected[I]); }
    TestTrue(TEXT("Boss posse includes boss"),DMArena::Preset(4).Contains(EDMArenaEnemy::GangBoss));
    TestTrue(TEXT("Swamp group contains grasper"),DMArena::Preset(5).Contains(EDMArenaEnemy::Grasper));
    TestTrue(TEXT("Invalid preset rejected"),DMArena::Preset(6).IsEmpty());
    return true;
}
#endif
