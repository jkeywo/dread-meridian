#include "DMInvestigatorComponent.h"
#include "DMCombatant.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMInvestigatorTest, "DreadMeridian.Foundation.InvestigatorBasics",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMInvestigatorTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ADMCombatant* Hero = World->SpawnActor<ADMCombatant>();
    UDMInvestigatorComponent* R = Hero->Investigator;
    R->Initialize(EDMInvestigator::Sapper);
    TestEqual(TEXT("Correct Sapper name"), R->DisplayName(), FString(TEXT("Trench Raider / Sapper")));
    TestTrue(TEXT("Collect first component"), R->CollectComponent());
    TestEqual(TEXT("Partial components do not refill charge"), R->Charges, 2);
    R->CollectComponent(); TestEqual(TEXT("Two components refill"), R->Charges, 3);
    TestFalse(TEXT("Full stock leaves pickup available"), R->CollectComponent());
    R->OnHit(TEXT("enemy"), 0); TestEqual(TEXT("Carbine does not consume charges"), R->Charges, 3);
    TestEqual(TEXT("Suppression improves carbine"), R->DamageMultiplier(TEXT("enemy"), true), 1.15f);
    R->Initialize(EDMInvestigator::Photographer);
    R->OnHit(TEXT("a"), 0); TestTrue(TEXT("Rifle does not invent Exposure"), R->Exposure.IsEmpty());
    R->AddExposure(TEXT("a"), 20, true, 0); R->AddExposure(TEXT("b"), 20, false, 0);
    TestEqual(TEXT("Perfect Moment amplifies committed subject"), R->Exposure[0].Value, 30.f);
    TestEqual(TEXT("Exposure isolated per subject"), R->Exposure[1].Value, 20.f);
    TestEqual(TEXT("Rifle scales with Exposure"), R->DamageMultiplier(TEXT("a"), false), 1.3f);
    R->Step(21, TEXT("a")); TestEqual(TEXT("Engaged subject retained"), R->Exposure[0].Value, 30.f);
    TestEqual(TEXT("Disengaged subject decays"), R->Exposure[1].Value, 19.5f);
    R->AddExposure(TEXT("a"), 1000, true, 22); TestEqual(TEXT("Exposure capped"), R->Exposure[0].Value, 100.f);
    R->Initialize(EDMInvestigator::Medium);
    R->OnHit(TEXT("a"), 0); TestTrue(TEXT("Lash does not create bound spirits"), R->Spirits.IsEmpty());
    R->BindSpirit(TEXT("near"), TEXT("a"), FVector::ZeroVector);
    R->BindSpirit(TEXT("far"), TEXT("b"), FVector(2000, 0, 0));
    R->OnHit(TEXT("a"), 0); TestEqual(TEXT("Bound target lash gains Attention"), R->Spirits[0].Value, 12.f);
    TestEqual(TEXT("Other spirit independent"), R->Spirits[1].Value, 0.f);
    R->ThinPlace(FVector::ZeroVector, 20); TestEqual(TEXT("Thin Places amplifies nearby event"), R->Spirits[0].Value, 42.f);
    TestEqual(TEXT("Distant spirit unaffected"), R->Spirits[1].Value, 0.f);
    R->Initialize(EDMInvestigator::Smuggler);
    TestFalse(TEXT("First punch"), R->OnHit(TEXT("a"), 0));
    TestFalse(TEXT("Second punch"), R->OnHit(TEXT("a"), 5));
    TestTrue(TEXT("Third punch finisher"), R->OnHit(TEXT("a"), 10));
    TestEqual(TEXT("Each hit builds Momentum"), R->Momentum, 30.f);
    R->Pressure(11, 10); TestEqual(TEXT("Incoming pressure also builds"), R->Momentum, 40.f);
    TestEqual(TEXT("Resistance band"), R->Resistance(), .35f);
    R->Step(41, TEXT("")); TestEqual(TEXT("No premature decay"), R->Momentum, 40.f);
    R->Step(42, TEXT("")); TestEqual(TEXT("Decay after grace period"), R->Momentum, 39.f);
    R->OnHit(TEXT("a"), 43); R->OnHit(TEXT("b"), 44); TestEqual(TEXT("Target change resets cadence"), R->Combo, 1);
    Hero->InitializeCombatant(TEXT("test"), false, 100, 10);
    Hero->ApplySlow(.5f, 100); Hero->StepInvestigator(45); Hero->Tick(.1f);
    TestTrue(TEXT("Keep Your Feet resists actual movement slow"), Hero->GetCharacterMovement()->MaxWalkSpeed > 210);
    Hero->Destroy(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
#endif
