#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMRecoverySupply.h"
#include "DMHudModel.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS
class FDMVerifyInjury : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyInjury(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyInjury() { if (Settings) { Settings->RemoveFromRoot(); } }
    bool Update() override
    {
        if (Stage == 0)
        {
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone); Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Injury verification"))).ClientSize(FVector2D(640,480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams P; P.EditorPlaySettings = Settings; P.CustomPIEWindow = Window;
            P.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox"); P.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(P); GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 90; Stage = 1; return false;
        }
        UWorld* W = GEditor->PlayWorld;
        auto* M = W ? W->GetAuthGameMode<ADMCombatGameMode>() : nullptr;
        if (Stage == 1 && M && M->GetCombatants().Num() >= 5)
        {
            int32 N = 0;
            for (ADMCombatant* A : M->GetCombatants())
            {
                if (A->GetController()) { A->GetController()->UnPossess(); }
                A->StopGoal(); A->SetAttackTarget(nullptr); A->NextAttackTick = 100000;
                A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(1500, -650 + N++ * 100, 95));
                if (!A->bIsEnemy && !Hero.IsValid()) { Hero = A; }
                else if (!A->bIsEnemy && !Ally.IsValid()) { Ally = A; }
                if (A->bIsEnemy && !Enemy.IsValid()) { Enemy = A; }
            }
            Hero->InitializeInvestigator(EDMInvestigator::Photographer, false);
            Hero->bProfileRange = true; Hero->SetActorLocation(FVector(-1200,0,95));
            Enemy->SetActorLocation(FVector(-1080,0,95)); Ally->SetActorLocation(FVector(-1200,100,95));
            Hero->AddShield(50);
            const float HP = Hero->Health(); Enemy->DealCombatDamage(Hero.Get(), 10, TEXT("test.shield"));
            Test->TestEqual(TEXT("Absorbed damage causes no Health loss"), Hero->Health(), HP);
            Test->TestEqual(TEXT("Absorbed damage grants no Injury"), Hero->InjuryCount, 0);
            if (!Acquire(EDMInjury::Concussion)) { return Finish(); }
            Test->TestTrue(TEXT("HUD exposes Injury name"), FDMHudModel::Read(*Hero, TEXT("")).SpecificInjuries.Contains(EDMInjury::Concussion));
            Test->TestTrue(TEXT("First cast accepted"), Hero->Kit->Request(EDMKitSlot::W, Enemy.Get(), Enemy->GetActorLocation()));
            Test->TestTrue(TEXT("Rapid second cast accepted"), Hero->Primary->Request(Enemy.Get(), Enemy->GetActorLocation()));
            Test->TestFalse(TEXT("Concussion blocks third cast"), Hero->Kit->Request(EDMKitSlot::E, Enemy.Get(), Enemy->GetActorLocation()));
            Test->TestTrue(TEXT("Refusal leaves E cooldown unspent"), Hero->Kit->CooldownSeconds(EDMKitSlot::E) <= 0);
            Hero->Primary->CancelChannel(); Due = Hero->Injuries->CastUntilTick; Stage = 2; return false;
        }
        if (Stage == 2 && M && M->GetCombatTick() >= Due)
        {
            Test->TestTrue(TEXT("Concussion pause ends"), Hero->Injuries->CanCast());
            if (!Acquire(EDMInjury::WoundedArm)) { return Finish(); }
            for (int32 I = 0; I < 3; ++I)
            { Hero->NextAttackTick = 0; Test->TestTrue(TEXT("Basic in chain accepted"), Hero->TryAttack(Enemy.Get())); }
            Hero->NextAttackTick = 0;
            Test->TestFalse(TEXT("Wounded Arm blocks another basic"), Hero->TryAttack(Enemy.Get()));
            Due = Hero->Injuries->AttackUntilTick; Stage = 3; return false;
        }
        if (Stage == 3 && M && M->GetCombatTick() >= Due)
        {
            Test->TestTrue(TEXT("Arm recovery ends"), Hero->Injuries->CanAttack());
            if (!Acquire(EDMInjury::TwistedKnee)) { return Finish(); }
            Hero->ApplyDisplacement(FVector(0,50,0)); Hero->StepInvestigator(M->GetCombatTick()); Hero->Tick(.1f);
            Test->TestTrue(TEXT("Actual displacement produces limp"), Hero->Injuries->MovementFactor < 1);
            Test->TestTrue(TEXT("Limp affects movement speed"), Hero->GetCharacterMovement()->MaxWalkSpeed < 420);
            if (!Acquire(EDMInjury::BrokenRibs)) { return Finish(); }
            Hero->HealHealth(1000); float Before = Hero->Health();
            Enemy->DealCombatDamage(Hero.Get(), 20, TEXT("test.heavy"));
            Test->TestTrue(TEXT("Repeated heavy impact invokes ribs"), FMath::IsNearlyEqual(Before - Hero->Health(), 25.f));
            if (!Acquire(EDMInjury::Burns)) { return Finish(); }
            Hero->HealHealth(1000); Enemy->DealCombatDamage(Hero.Get(), 4, TEXT("test.hazard"), false, true); Before = Hero->Health();
            Enemy->DealCombatDamage(Hero.Get(), 4, TEXT("test.hazard"), false, true);
            Test->TestTrue(TEXT("Repeated hazard exposure invokes Burns"), FMath::IsNearlyEqual(Before - Hero->Health(), 6.f));
            if (!Acquire(EDMInjury::DeepCut)) { return Finish(); }
            Before = Hero->Health(); Hero->HealHealth(10);
            Test->TestTrue(TEXT("Deep Cut weakens burst recovery"), FMath::IsNearlyEqual(Hero->Health() - Before, 5.f));
            Test->TestEqual(TEXT("Healing retains specific Injury"), Hero->InjuryCount, 1);
            Clear(); Hero->HealHealth(1000);
            for (int32 I = 0; I < 3; ++I)
            {
                Enemy->DealCombatDamage(Hero.Get(), 10000, TEXT("test.down"));
                Test->TestTrue(TEXT("Lethal hit downs"), Hero->IsDown());
                Test->TestEqual(TEXT("A lethal burst grants exactly one Injury event"), Hero->InjuryCount + Hero->GrievousCount, I + 1);
                Test->TestEqual(TEXT("Ordinary healing cannot revive"), Hero->HealHealth(1000), 0.f);
                Test->TestTrue(TEXT("Ally can revive without clearing Injuries"), Ally->Revive(Hero.Get()));
            }
            Test->TestEqual(TEXT("Specific cap preserved"), Hero->InjuryCount, 2);
            Test->TestEqual(TEXT("Third event is Grievous"), Hero->GrievousCount, 1);
            auto* Supply = W->SpawnActor<ADMRecoverySupply>(Hero->GetActorLocation() + FVector(600,0,0), FRotator::ZeroRotator);
            Test->TestFalse(TEXT("Distant treatment refused"), Supply->TryUse(Hero.Get()));
            Test->TestEqual(TEXT("Refused use keeps charges"), Supply->Charges, 2);
            Supply->SetActorLocation(Hero->GetActorLocation());
            Test->TestTrue(TEXT("Nearby treatment accepted"), Supply->TryUse(Hero.Get()));
            Test->TestEqual(TEXT("Grievous treated first"), Hero->GrievousCount, 0);
            Test->TestEqual(TEXT("Specific effects remain until later treatment"), Hero->InjuryCount, 2);
            Test->TestTrue(TEXT("Next treatment clears one specific"), Supply->TryUse(Hero.Get()));
            Test->TestEqual(TEXT("One specific remains"), Hero->InjuryCount, 1);
            Test->TestFalse(TEXT("Exhausted supply refuses use"), Supply->TryUse(Hero.Get()));
            auto* Food = W->SpawnActor<ADMRecoverySupply>(Hero->GetActorLocation(), FRotator::ZeroRotator);
            Food->bFood = true; Food->Charges = 1;
            BeforeFood = Hero->Health();
            Test->TestTrue(TEXT("Food accepted by hurt investigator"), Food->TryUse(Hero.Get()));
            Test->TestEqual(TEXT("Food does not heal instantly"), Hero->Health(), BeforeFood);
            Due = M->GetCombatTick() + Hero->Injuries->Settings.FoodIntervalTicks + 1; Stage = 4; return false;
        }
        if (Stage == 4 && M && M->GetCombatTick() >= Due)
        {
            Test->TestTrue(TEXT("Food heals on logical pulses"), Hero->Health() > BeforeFood);
            Test->TestEqual(TEXT("Food does not remove Injury"), Hero->InjuryCount, 1);
            return Finish();
        }
        if (Stage == 5 && !W) { if (Window) { Window->RequestDestroyWindow(); } return true; }
        if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("Injury test timed out")); GEditor->RequestEndPlayMap(); return true; }
        return false;
    }
private:
    void Clear() { while (Hero->Injuries->Treat()) {} }
    bool Acquire(EDMInjury Kind)
    {
        // Use the real random selection and damage API; no writable test-only Injury state.
        for (int32 I = 0; I < 128; ++I)
        {
            Clear(); Hero->HealHealth(10000);
            Enemy->DealCombatDamage(Hero.Get(), Hero->Shield() + 35, TEXT("test.burst"));
            if (Hero->Injuries->Specific.Contains(Kind)) { return true; }
        }
        Test->AddError(FString(TEXT("Seeded fixture never selected ")) + DMInjuryRules::Name(Kind)); return false;
    }
    bool Finish() { GEditor->RequestEndPlayMap(); Stage = 5; return false; }
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    TWeakObjectPtr<ADMCombatant> Hero, Ally, Enemy;
    int32 Stage = 0, Due = 0;
    float BeforeFood = 0;
    double Deadline = 0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMInjuryIntegrationTest, "DreadMeridian.Editor.Injuries.Control",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMInjuryIntegrationTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyInjury(this)); return true; }
#endif
