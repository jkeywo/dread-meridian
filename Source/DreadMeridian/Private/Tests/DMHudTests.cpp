#include "DMHudModel.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMHudModelTest, "DreadMeridian.Foundation.HudModel",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMHudModelTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

    ADMCombatant* Self = World->SpawnActor<ADMCombatant>();
    Self->InitializeCombatant(TEXT("inv_1"), false, 200, 10, 40);
    Self->InitializeInvestigator(EDMInvestigator::Sapper, false);
    ADMCombatant* Ally = World->SpawnActor<ADMCombatant>();
    Ally->InitializeCombatant(TEXT("inv_2"), false, 0, 10);
    Ally->InitializeInvestigator(EDMInvestigator::Medium, false);
    Ally->GrievousCount = 2;
    ADMCombatant* Hunter = World->SpawnActor<ADMCombatant>();
    Hunter->InitializeCombatant(TEXT("enemy_1"), true, 150, 7);
    ADMCombatant* Idle = World->SpawnActor<ADMCombatant>();
    Idle->InitializeCombatant(TEXT("enemy_2"), true, 0, 7);

    Hunter->SetAttackTarget(Self);
    Ally->SetReviveChannel(Self->EntityId, .5f);
    Self->NextAttackTick = 5;
    Self->AttackIntervalTicks = 10;

    const FDMHudModel Model = FDMHudModel::Build(World, Self, Hunter);

    TestTrue(TEXT("Local investigator resolved"), Model.bHasSelf);
    TestEqual(TEXT("Self is not listed as an ally"), Model.Allies.Num(), 1);
    TestEqual(TEXT("Enemies collected"), Model.Enemies.Num(), 2);
    TestEqual(TEXT("Downed enemies are not counted as standing"), Model.EnemiesStanding, 1);
    TestEqual(TEXT("Downed investigators are not counted as up"), Model.InvestigatorsStanding, 1);
    TestEqual(TEXT("Health fraction"), Model.Self.HealthFraction(), 1.f);
    TestEqual(TEXT("Shield projects against max health"), Model.Self.ShieldFraction(), .2f);
    TestFalse(TEXT("Zero max health does not divide"), FMath::IsNaN(Model.Allies[0].HealthFraction()));
    TestTrue(TEXT("Downed ally reported down"), Model.Allies[0].bDown);
    TestTrue(TEXT("Revive channel is a public projection"), Model.Allies[0].IsReviving());
    TestEqual(TEXT("Revive progress passes through"), Model.Allies[0].ReviveProgress, .5f);
    TestEqual(TEXT("Reviver identified"), Model.Allies[0].ReviverId, FString(TEXT("inv_1")));

    const FDMHudUnit* Attacker = Model.Enemies.FindByPredicate(
        [](const FDMHudUnit& Unit) { return Unit.EntityId == TEXT("enemy_1"); });
    TestNotNull(TEXT("Attacking enemy present"), Attacker);
    TestTrue(TEXT("Aggro on the local investigator is visible"), Attacker->bTargetingLocal);
    TestFalse(TEXT("Idle enemy carries no aggro cue"), Model.Enemies[1].bTargetingLocal);

    TestTrue(TEXT("Selected target resolved"), Model.bHasTarget);
    TestEqual(TEXT("Selected target identity"), Model.Target.EntityId, FString(TEXT("enemy_1")));
    TestEqual(TEXT("Basic attack cooldown fraction"), Model.BasicCooldown, .5f);
    TestEqual(TEXT("Basic attack cooldown seconds"), Model.BasicCooldownSeconds, .5f);

    TestEqual(TEXT("Grievous stacks lengthen the revive channel"),
        ADMCombatGameMode::ReviveDurationTicks(2), 40);
    TestTrue(TEXT("Revive curve is nonlinear"),
        ADMCombatGameMode::ReviveDurationTicks(3) - ADMCombatGameMode::ReviveDurationTicks(2)
            > ADMCombatGameMode::ReviveDurationTicks(2) - ADMCombatGameMode::ReviveDurationTicks(1));

    const FDMHudModel Empty = FDMHudModel::Build(nullptr, nullptr, nullptr);
    TestFalse(TEXT("No world yields no investigator"), Empty.bHasSelf);

    Self->Destroy(); Ally->Destroy(); Hunter->Destroy(); Idle->Destroy();
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
