#include "DMSmugglerWave.h"
#include "DMEncounterLayout.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMSmugglerWaveTest,"DreadMeridian.Foundation.SmugglerWave",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FDMSmugglerWaveTest::RunTest(const FString& Parameters)
{
    FDMSmugglerWave W;
    TestTrue(TEXT("Any surviving camp or patrol blocks finale"),W.Advance(true,true,100)==EDMWaveAction::None);
    TestTrue(TEXT("Last occupation kill announces boss, not victory"),W.Advance(true,false,101)==EDMWaveAction::Announce);
    TestTrue(TEXT("Arrival has a readable three-second delay"),W.Advance(true,false,130)==EDMWaveAction::None);
    TestTrue(TEXT("Boss and posse spawn at deadline"),W.Advance(true,false,131)==EDMWaveAction::SpawnPosse);
    TestTrue(TEXT("Any surviving posse member blocks victory"),W.Advance(true,true,132)==EDMWaveAction::None);
    TestTrue(TEXT("Defeating whole finale grants victory"),W.Advance(true,false,133)==EDMWaveAction::Victory);
    TestTrue(TEXT("Finale cannot spawn twice"),W.Advance(true,false,134)==EDMWaveAction::None);
    FDMSmugglerWave Loss; Loss.Advance(true,false,1);
    TestTrue(TEXT("Wipe during arrival loses immediately"),Loss.Advance(false,false,31)==EDMWaveAction::Defeat);
    FDMSmugglerWave Simultaneous;
    TestTrue(TEXT("Simultaneous last enemy and squad death is defeat"),Simultaneous.Advance(false,false,1)==EDMWaveAction::Defeat);
    TSet<EDMSmuggler> Roles;
    for(int32 I=0;I<DMEncounterLayout::EnemyCount;++I) { Roles.Add(DMEncounterLayout::EnemyRole(I)); }
    TestEqual(TEXT("Occupation mixes all four common roles"),Roles.Num(),4);
    TestFalse(TEXT("Boss is reserved for the finale"),Roles.Contains(EDMSmuggler::GangBoss));
    Roles.Empty();for(int32 I=0;I<DMEncounterLayout::PosseCount;++I) { Roles.Add(DMEncounterLayout::PosseRole(I)); }
    TestEqual(TEXT("Finale includes boss and four complementary posse roles"),Roles.Num(),5);
    return true;
}
#endif
