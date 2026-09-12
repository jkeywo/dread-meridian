#include "DMInvestigatorComponent.h"
#include "DMKitComponent.h"
#include "DMCombatant.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Kit-facing combatant and resource state, without a game mode: the pieces the W/E/R abilities build on.
 * Anything needing combat ticks, casts or replication lives in the PIE tests instead.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMKitComponentTest, "DreadMeridian.Foundation.Kits.Components",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMKitComponentTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ADMCombatant* Hero = World->SpawnActor<ADMCombatant>();
    UDMInvestigatorComponent* R = Hero->Investigator;

    // Madness: a clamped stub the ultimates spike. No decay, because GDD 4.6 describes none.
    R->Initialize(EDMInvestigator::Sapper);
    TestEqual(TEXT("Madness starts clear"), R->Madness, 0.f);
    R->AddMadness(30, TEXT("dead_ground"));
    R->AddMadness(30, TEXT("dead_ground"));
    TestEqual(TEXT("Spikes accumulate"), R->Madness, 60.f);
    R->Step(500, TEXT("")); R->Step(1000, TEXT(""));
    TestEqual(TEXT("Madness does not decay on its own"), R->Madness, 60.f);
    R->AddMadness(-5, TEXT("bad")); R->AddMadness(0, TEXT("bad"));
    TestEqual(TEXT("Madness ignores non-positive spikes"), R->Madness, 60.f);
    R->AddMadness(1000, TEXT("crisis")); TestEqual(TEXT("Madness clamps"), R->Madness, 100.f);
    TestTrue(TEXT("Madness reaches the resource summary"), R->ResourceSummary().Contains(TEXT("Madness")));
    R->Initialize(EDMInvestigator::Sapper); TestEqual(TEXT("Madness resets with the kit"), R->Madness, 0.f);

    // Exposure: Impossible Photograph freezes decay and lets Develop read the same value repeatedly.
    R->Initialize(EDMInvestigator::Photographer);
    R->AddExposure(TEXT("a"), 60, false, 0);
    TestEqual(TEXT("Peek does not consume"), R->PeekExposure(TEXT("a")), 60.f);
    TestEqual(TEXT("Peek of an unknown subject is zero"), R->PeekExposure(TEXT("nobody")), 0.f);
    R->bExposureFrozen = true;
    TestEqual(TEXT("Frozen Develop returns the value"), R->ConsumeExposure(TEXT("a")), 60.f);
    TestEqual(TEXT("Frozen Develop keeps the value"), R->PeekExposure(TEXT("a")), 60.f);
    R->Step(100, TEXT("")); TestEqual(TEXT("Frozen Exposure does not decay"), R->PeekExposure(TEXT("a")), 60.f);
    R->bExposureFrozen = false;
    TestEqual(TEXT("Develop returns the value"), R->ConsumeExposure(TEXT("a")), 60.f);
    TestEqual(TEXT("Develop spends it"), R->PeekExposure(TEXT("a")), 0.f);
    R->Step(200, TEXT("")); TestTrue(TEXT("Thawed Exposure decays again"), R->PeekExposure(TEXT("a")) <= 0.f);

    // Momentum floor (Drowned Man Walking) holds the band up through the decay step.
    R->Initialize(EDMInvestigator::Smuggler);
    R->Pressure(0, 10); TestEqual(TEXT("Momentum from pressure"), R->Momentum, 10.f);
    R->MomentumFloor = 75;
    R->Step(100, TEXT("")); TestEqual(TEXT("Floor lifts Momentum"), R->Momentum, 75.f);
    R->Step(200, TEXT("")); TestEqual(TEXT("Floor holds through decay"), R->Momentum, 75.f);
    TestEqual(TEXT("Floor reaches the top resistance band"), R->Resistance(), .6f);
    R->MomentumFloor = 0;
    R->Step(300, TEXT("")); TestEqual(TEXT("Momentum decays once the floor lifts"), R->Momentum, 74.f);

    // Spirit locations are addressed by spirit id, so several ground spirits keep distinct positions.
    R->Initialize(EDMInvestigator::Medium);
    R->BindSpirit(TEXT("spirit.1"), TEXT(""), FVector::ZeroVector);
    R->BindSpirit(TEXT("spirit.2"), TEXT(""), FVector::ZeroVector);
    R->UpdateSpiritLocationById(TEXT("spirit.1"), FVector(500, 0, 0));
    R->UpdateSpiritLocationById(TEXT("spirit.2"), FVector(-500, 0, 0));
    TestEqual(TEXT("First spirit moved"), static_cast<float>(R->Spirits[0].Location.X), 500.f);
    TestEqual(TEXT("Second spirit independent"), static_cast<float>(R->Spirits[1].Location.X), -500.f);

    // Slows stack by strength through the combatant, so a root survives a weaker refresh.
    Hero->InitializeCombatant(TEXT("hero"), false, 100, 10);
    Hero->Investigator->Initialize(EDMInvestigator::Photographer);
    Hero->ApplySlow(1.f, 40);
    Hero->ApplySlow(.4f, 20);
    Hero->StepInvestigator(10); Hero->Tick(.1f);
    TestTrue(TEXT("A root is not replaced by a weaker slow"), Hero->GetCharacterMovement()->MaxWalkSpeed < 1.f);
    Hero->StepInvestigator(45); Hero->Tick(.1f);
    TestTrue(TEXT("Speed returns once every slow expires"), Hero->GetCharacterMovement()->MaxWalkSpeed > 400.f);

    // Shield rides the same GameplayEffect path as damage, which a bare test world cannot run: actors here are never
    // initialised for play, so the ability system holds no attribute set. The cap rule itself is pure and covered in
    // Kits.Rules; that it reaches Shield is asserted in the PIE tests.

    // The component owns the Broken projection; external writes cannot create an endless control window.
    ADMCombatant* Common = World->SpawnActor<ADMCombatant>();
    Common->InitializeCombatant(TEXT("common"), true, 100, 5);
    Common->bCommonEnemy = true;
    Common->AddBreak(200);
    TestEqual(TEXT("Common enemies carry no Resolve"), Common->Break, 0.f);
    TestFalse(TEXT("Common enemies are never Broken"), Common->bBreakVulnerable);

    ADMCombatant* Elite = World->SpawnActor<ADMCombatant>();
    Elite->InitializeCombatant(TEXT("elite"), true, 750, 9);
    Elite->bCommonEnemy = false;
    Elite->bBreakVulnerable = true;   // set by hand, as the base-Q tests do
    Elite->StepControl(5);
    TestFalse(TEXT("Only the Resolve component can establish a Broken window"), Elite->bBreakVulnerable);
    Elite->bBreakVulnerable = false;

    Hero->Destroy(); Common->Destroy(); Elite->Destroy();
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}

/** The static per-(kind, slot) table: every investigator has three named abilities and the ranges the HUD reads. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMKitSpecTest, "DreadMeridian.Foundation.Kits.Specs",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMKitSpecTest::RunTest(const FString& Parameters)
{
    const EDMInvestigator Kinds[] = { EDMInvestigator::Sapper, EDMInvestigator::Photographer, EDMInvestigator::Medium, EDMInvestigator::Smuggler };
    for (EDMInvestigator Kind : Kinds)
    {
        for (int32 Index = 0; Index < static_cast<int32>(EDMKitSlot::Count); ++Index)
        {
            const FDMKitSpec& Spec = UDMKitComponent::Spec(Kind, static_cast<EDMKitSlot>(Index));
            TestTrue(TEXT("Every slot is named"), FCString::Strlen(Spec.Name) > 0);
            TestTrue(TEXT("Every slot has a cooldown"), Spec.CooldownTicks > 0);
            TestTrue(TEXT("Targeted slots have a range"), Spec.bSelfCast || Spec.Range > 0);
        }
        // R is the strongly Madness-dependent ultimate: always a self-cast on the longest cooldown.
        const FDMKitSpec& Ultimate = UDMKitComponent::Spec(Kind, EDMKitSlot::R);
        TestTrue(TEXT("R is self-cast"), Ultimate.bSelfCast);
        TestEqual(TEXT("R shares the ultimate cooldown"), Ultimate.CooldownTicks, 600);
        TestTrue(TEXT("R runs for a window"), Ultimate.DurationTicks > 0);
    }
    // None has no kit at all, so an unassigned pawn advertises nothing.
    TestEqual(TEXT("No kit without an investigator"), FString(UDMKitComponent::Spec(EDMInvestigator::None, EDMKitSlot::W).Name), FString());
    // Only the Sapper's Tripwire takes two points.
    TestTrue(TEXT("Tripwire is two-point"), UDMKitComponent::Spec(EDMInvestigator::Sapper, EDMKitSlot::E).bTwoPoint);
    TestFalse(TEXT("Develop is single-point"), UDMKitComponent::Spec(EDMInvestigator::Photographer, EDMKitSlot::E).bTwoPoint);
    return true;
}

#endif
