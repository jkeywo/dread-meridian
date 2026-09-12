#include "DMCombatant.h"
#include "DMCombatGameMode.h"
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
class FDMVerifyBreak : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyBreak(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifyBreak() { if (Settings) { Settings->RemoveFromRoot(); } }
    bool Update() override
    {
        if (Stage == 0)
        {
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone);
            Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Break verification"))).ClientSize(FVector2D(640, 480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams Params; Params.EditorPlaySettings = Settings;
            Params.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox");
            Params.CustomPIEWindow = Window; Params.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(Params); GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 90; Stage = 1; return false;
        }
        if (Stage == 1 && GEditor->PlayWorld)
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            if (!Mode || Mode->GetCombatants().Num() < 5) { return CheckTimeout(); }
            int32 N = 0;
            for (ADMCombatant* A : Mode->GetCombatants())
            {
                // Exercise production control APIs while preventing unrelated bot decisions from changing this fixture.
                if (A->GetController()) { A->GetController()->UnPossess(); }
                A->StopGoal(); A->SetAttackTarget(nullptr); A->NextAttackTick = 100000;
                A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(1200, -650 + 150 * N++, 95));
                if (!A->bIsEnemy && !Hero.IsValid()) { Hero = A; }
                if (A->bIsEnemy && !Enemy.IsValid()) { Enemy = A; }
            }
            if (!Test->TestTrue(TEXT("Fixture has hero and enemy"), Hero.IsValid() && Enemy.IsValid())) { return Finish(); }
            Hero->InitializeInvestigator(EDMInvestigator::Smuggler, false);
            Hero->SetActorLocation(FVector(-1200, 0, 95)); Enemy->SetActorLocation(FVector(-1080, 0, 95));
            Enemy->Smuggler->Initialize(EDMSmuggler::GangBoss);
            Enemy->Resolve->Settings.MaxResolve = 80; Enemy->Resolve->Settings.BrokenTicks = 12;
            Enemy->Resolve->Settings.ResistTicks = 20; Enemy->Resolve->Settings.ResistFactor = .5f;
            Enemy->Resolve->Reset();
            Test->TestEqual(TEXT("Public Resolve starts full"), Enemy->Resolve->CurrentResolve, 80.f);
            FDMControl Invalid; Invalid.BreakPressure = NAN;
            Enemy->ApplyControl(Invalid, Hero.Get(), TEXT("test.break.invalid"));
            Test->TestEqual(TEXT("Invalid pressure cannot alter Resolve"), Enemy->Resolve->CurrentResolve, 80.f);
            Test->TestEqual(TEXT("HUD reads authored Resolve fraction"), FDMHudModel::Read(*Enemy, TEXT("")).BreakFraction, 1.f);
            Test->TestTrue(TEXT("Elite command starts"), Enemy->Smuggler->TrySignature(*Mode, Hero.Get()));
            FDMControl C; C.bInterrupt = true; C.StunTicks = 30; C.Slow = 1; C.SlowTicks = 30; C.BreakPressure = 40;
            Enemy->ApplyControl(C, Hero.Get(), TEXT("test.break.control"));
            Test->TestTrue(TEXT("Protected command survives"), Enemy->Smuggler->OrderTarget != nullptr);
            Test->TestFalse(TEXT("Protected enemy cannot be stunned"), Enemy->IsStunned());
            Test->TestEqual(TEXT("Pressure depletes Resolve"), Enemy->Resolve->CurrentResolve, 40.f);
            Test->TestEqual(TEXT("Protected root is partial slow"), DMKitRules::EffectiveSlow(Enemy->Slows, Mode->GetCombatTick()), .5f);
            Enemy->ApplyControl(C, Hero.Get(), TEXT("test.break.control"));
            Test->TestTrue(TEXT("Threshold breaks"), Enemy->bBreakVulnerable);
            Test->TestTrue(TEXT("Breaking hit retains pre-hit protection"), Enemy->Smuggler->OrderTarget != nullptr);
            Enemy->ApplyControl(C, Hero.Get(), TEXT("test.break.control"));
            Test->TestFalse(TEXT("Broken interrupt clears command"), Enemy->Smuggler->OrderTarget != nullptr);
            Test->TestEqual(TEXT("Stun capped at recovery"), Enemy->StunnedUntilTick, Enemy->BrokenUntilTick);
            Enemy->Tick(.1f);
            Test->TestEqual(TEXT("Stun removes movement speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 0.f);
            Test->TestFalse(TEXT("Stunned signature refused"), Enemy->Smuggler->TrySignature(*Mode, Hero.Get()));
            Test->TestTrue(TEXT("Broken elite can be clinched"), Hero->Primary->Request(Enemy.Get(), Enemy->GetActorLocation()));
            Test->TestTrue(TEXT("Clinch established"), Enemy->IsRestrained());
            Due = Enemy->BrokenUntilTick; Stage = 2; return false;
        }
        if (Stage == 2 && GEditor->PlayWorld && Enemy.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            if (Mode->GetCombatTick() < Due) { return CheckTimeout(); }
            Test->TestFalse(TEXT("Recovery closes Broken"), Enemy->bBreakVulnerable);
            Test->TestFalse(TEXT("Recovery releases clinch"), Enemy->IsRestrained());
            Test->TestFalse(TEXT("Recovery ends stun"), Enemy->IsStunned());
            Test->TestTrue(TEXT("Recovery restores Resolve"), Enemy->Resolve->CurrentResolve == 80);
            Test->TestTrue(TEXT("Recovery resistance visible"), FDMHudModel::Read(*Enemy, TEXT("")).bResisting);
            Enemy->AddBreak(40); Test->TestEqual(TEXT("Resistance halves accepted pressure"), Enemy->Resolve->CurrentResolve, 60.f);
            Enemy->Smuggler->NextSignatureTick = 0;
            Test->TestTrue(TEXT("Recovered enemy can command"), Enemy->Smuggler->TrySignature(*Mode, Hero.Get()));
            Enemy->Resolve->OpenInterruptWindow(3);
            FDMControl C; C.bInterrupt = true; C.StunTicks = 10;
            Enemy->ApplyControl(C, Hero.Get(), TEXT("test.break.window"));
            Test->TestFalse(TEXT("Independent window permits interrupt"), Enemy->Smuggler->OrderTarget != nullptr);
            Test->TestFalse(TEXT("Independent window does not permit stun"), Enemy->IsStunned());
            Due = Enemy->Resolve->InterruptUntilTick; Stage = 3; return false;
        }
        if (Stage == 3 && GEditor->PlayWorld && Enemy.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            if (Mode->GetCombatTick() < Due) { return CheckTimeout(); }
            Test->TestEqual(TEXT("Independent window expires"), Enemy->Resolve->InterruptUntilTick, 0);
            Enemy->Smuggler->Initialize(EDMSmuggler::Bomber); Enemy->Smuggler->NextSignatureTick = 0;
            Test->TestTrue(TEXT("Common bomber begins cast"), Enemy->Smuggler->TrySignature(*Mode, Hero.Get()));
            FDMControl C; C.StunTicks = 4; C.Slow = 1; C.SlowTicks = 5;
            Enemy->ApplyControl(C, Hero.Get(), TEXT("test.break.common"));
            Test->TestFalse(TEXT("Common stun interrupts pending cast"), Enemy->Smuggler->IsCasting());
            Test->TestTrue(TEXT("Common takes full stun"), Enemy->IsStunned());
            Test->TestEqual(TEXT("Common root is full"), DMKitRules::EffectiveSlow(Enemy->Slows, Mode->GetCombatTick()), 1.f);
            Test->TestEqual(TEXT("Common has no Resolve"), Enemy->Resolve->MaxResolve, 0.f);
            Due = Mode->GetCombatTick() + 5; Stage = 4; return false;
        }
        if (Stage == 4 && GEditor->PlayWorld && Enemy.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            if (Mode->GetCombatTick() < Due) { return CheckTimeout(); }
            Enemy->Smuggler->NextSignatureTick = 0;
            Test->TestTrue(TEXT("Common can cast after stun expires"), Enemy->Smuggler->TrySignature(*Mode, Hero.Get()));
            Due = Enemy->Smuggler->ResolveTick + 1; Stage = 5; return false;
        }
        if (Stage == 5 && GEditor->PlayWorld && Enemy.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            if (Mode->GetCombatTick() < Due) { return CheckTimeout(); }
            HeroLife = Hero->Health() + Hero->Shield();
            FDMControl C; C.StunTicks = 15; Enemy->ApplyControl(C, Hero.Get(), TEXT("test.break.released_fire"));
            Due = Mode->GetCombatTick() + 11; Stage = 6; return false;
        }
        if (Stage == 6 && GEditor->PlayWorld && Enemy.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            if (Mode->GetCombatTick() < Due) { return CheckTimeout(); }
            Test->TestTrue(TEXT("Released fire keeps damaging while caster is stunned"), Hero->Health() + Hero->Shield() < HeroLife);
            Enemy->Smuggler->Initialize(EDMSmuggler::GangBoss); Enemy->AddBreak(80);
            Test->TestTrue(TEXT("Death fixture is Broken"), Enemy->bBreakVulnerable);
            Hero->DealCombatDamage(Enemy.Get(), 100000, TEXT("test.break.kill"));
            Test->TestFalse(TEXT("Death clears Broken"), Enemy->bBreakVulnerable);
            Test->TestEqual(TEXT("Death clears public Resolve"), Enemy->Resolve->MaxResolve, 0.f);
            return Finish();
        }
        if (Stage == 7 && !GEditor->PlayWorld)
        { if (Window) { Window->RequestDestroyWindow(); Window.Reset(); } return true; }
        return CheckTimeout();
    }
private:
    bool Finish() { GEditor->RequestEndPlayMap(); Stage = 7; return false; }
    bool CheckTimeout()
    {
        if (FPlatformTime::Seconds() <= Deadline) { return false; }
        Test->AddError(FString::Printf(TEXT("Break test timed out at stage %d"), Stage));
        GEditor->RequestEndPlayMap(); if (Window) { Window->RequestDestroyWindow(); } return true;
    }
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    TWeakObjectPtr<ADMCombatant> Hero, Enemy;
    int32 Stage = 0, Due = 0;
    float HeroLife = 0;
    double Deadline = 0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMBreakIntegrationTest, "DreadMeridian.Editor.Break.Control",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMBreakIntegrationTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifyBreak(this)); return true; }
#endif
