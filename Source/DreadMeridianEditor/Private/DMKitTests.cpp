#include "DMEditorPlaySelection.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMAbilityMarker.h"
#include "DMKitComponent.h"
#include "DMAIProfile.h"
#include "DMUtilityAI.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * The Sapper's named kit in a live PIE world: a suppression cone that damages and suppresses what stands in it,
 * a tripwire that only triggers on a crossing, and Dead Ground deferring triggers that would have happened and
 * resolving them together. Combat ticks come from the game mode's own timer, as in the base-Q test.
 */
class FDMVerifySapperKit : public IAutomationLatentCommand
{
public:
    explicit FDMVerifySapperKit(FAutomationTestBase* InTest) : Test(InTest)
    {
        bHadPreference = GConfig->GetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), Original, GEditorPerProjectIni);
    }
    virtual ~FDMVerifySapperKit() override
    {
        if (bHadPreference) { GConfig->SetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), *Original, GEditorPerProjectIni); }
        else { GConfig->RemoveKey(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), GEditorPerProjectIni); }
        GConfig->Flush(false, GEditorPerProjectIni);
        if (Settings) { Settings->RemoveFromRoot(); }
    }

    virtual bool Update() override
    {
        if (Stage == 0)
        {
            DMEditorPlaySelection::Save(TEXT("Sapper"));
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone);
            Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Sapper kit verification"))).ClientSize(FVector2D(640, 480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams Params;
            Params.EditorPlaySettings = Settings;
            Params.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox");
            Params.CustomPIEWindow = Window;
            Params.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(Params);
            GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 90; Stage = 1;
            return false;
        }
        if (Stage == 1 && GEditor->PlayWorld)
        {
            APlayerController* Player = GEditor->PlayWorld->GetFirstPlayerController();
            ADMCombatant* Hero = Player ? Cast<ADMCombatant>(Player->GetPawn()) : nullptr;
            if (!Hero) { return Timeout(); }
            Test->TestEqual(TEXT("PIE possesses the Sapper"), static_cast<int32>(Hero->Investigator->Kind), static_cast<int32>(EDMInvestigator::Sapper));
            Subject = Hero;
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            int32 N = 0;
            for (ADMCombatant* A : Mode->GetCombatants())
            {
                A->bProfileRange = true; A->NextAttackTick = 100000;
                A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(1400, -700 + N++ * 160, 95));
                if (A->bIsEnemy && !Enemy.IsValid()) { Enemy = A; }
            }
            Hero->SetActorLocation(FVector(-1200, 0, 95));
            Test->TestFalse(TEXT("Suppressing Fire needs a direction"), Hero->Kit->Request(EDMKitSlot::W, nullptr, Hero->GetActorLocation()));
            Test->TestFalse(TEXT("Suppressing Fire rejected out of range"), Hero->Kit->Request(EDMKitSlot::W, nullptr, FVector(8000, 0, 95)));
            Test->TestFalse(TEXT("Dead Ground refused with nothing prepared"), Hero->Kit->Request(EDMKitSlot::R, nullptr, Hero->GetActorLocation()));
            Test->TestTrue(TEXT("A rejected cast spends no cooldown"), Hero->Kit->IsReady(EDMKitSlot::W) && Hero->Kit->IsReady(EDMKitSlot::R));

            // W: the enemy stands 400 ahead, inside a cone aimed down +X.
            Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(400, 0, 0));
            EnemyHealth = Enemy->Health();
            Test->TestTrue(TEXT("Suppressing Fire accepted"), Hero->Kit->Request(EDMKitSlot::W, nullptr, Hero->GetActorLocation() + FVector(500, 0, 0)));
            Test->TestEqual(TEXT("Cone zone placed"), Hero->Kit->Zones.Num(), 1);
            Test->TestFalse(TEXT("Suppressing Fire now on cooldown"), Hero->Kit->IsReady(EDMKitSlot::W));
            TestTick = Mode->GetCombatTick() + 7; Stage = 2;
            return false;
        }
        if (Stage >= 2 && GEditor->PlayWorld && Subject.IsValid() && Enemy.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            auto* Hero = Subject.Get();
            auto* Kit = Hero->Kit.Get();
            if (Mode->GetCombatTick() < TestTick) { return false; }
            switch (Stage)
            {
            case 2:
                Test->TestTrue(TEXT("Suppression damages what stands in the cone"), Enemy->Health() < EnemyHealth);
                Test->TestTrue(TEXT("Suppression tags the target"), Enemy->bSuppressed);
                Test->TestTrue(TEXT("Suppressed enemies take more carbine damage"), Hero->Investigator->DamageMultiplier(Enemy->EntityId, Enemy->bSuppressed) > 1);
                // Out of the cone, nothing further lands and the tag lapses.
                Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(0, 900, 0));
                EnemyHealth = Enemy->Health();
                TestTick = Mode->GetCombatTick() + 14; Stage = 3;
                return false;
            case 3:
                Test->TestEqual(TEXT("Outside the cone the enemy is untouched"), Enemy->Health(), EnemyHealth);
                Test->TestFalse(TEXT("Suppression lapses"), Enemy->bSuppressed);
                // E: the first end only arms the placement; the wire is not laid until the second lands.
                Test->TestTrue(TEXT("First wire end accepted"), Kit->Request(EDMKitSlot::E, nullptr, Hero->GetActorLocation() + FVector(300, -200, 0)));
                Test->TestTrue(TEXT("Wire is pending"), Kit->bWirePending);
                Test->TestEqual(TEXT("No wire placed yet"), Kit->Wires.Num(), 0);
                Test->TestFalse(TEXT("A too-short wire is refused"), Kit->Request(EDMKitSlot::E, nullptr, Kit->PendingWireStart + FVector(10, 0, 0)));
                Test->TestTrue(TEXT("Refusal keeps the pending end"), Kit->bWirePending);
                Test->TestTrue(TEXT("Second wire end accepted"), Kit->Request(EDMKitSlot::E, nullptr, Hero->GetActorLocation() + FVector(300, 200, 0)));
                Test->TestEqual(TEXT("Wire placed"), Kit->Wires.Num(), 1);
                Test->TestFalse(TEXT("Pending end cleared"), Kit->bWirePending);
                // Park the enemy on one side, clear of the wire, and let the position cache seed.
                Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(300, -500, 0));
                EnemyHealth = Enemy->Health();
                TestTick = Mode->GetCombatTick() + 8; Stage = 4;
                return false;
            case 4:
                Test->TestEqual(TEXT("Standing clear of the wire does not trigger it"), Enemy->Health(), EnemyHealth);
                Test->TestEqual(TEXT("Wire still armed"), Kit->Wires.Num(), 1);
                Enemy->NextAttackTick = 0;
                // Cross it.
                Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(300, 500, 0));
                TestTick = Mode->GetCombatTick() + 2; Stage = 5;
                return false;
            case 5:
                Test->TestTrue(TEXT("Crossing the wire applies its damage"), Enemy->Health() < EnemyHealth);
                Test->TestTrue(TEXT("Crossing staggers the crosser"), Enemy->StaggeredUntilTick > Mode->GetCombatTick());
                Test->TestEqual(TEXT("A wire is spent by its first crossing"), Kit->Wires.Num(), 0);
                // R: prepare a satchel, then defer its trigger.
                Hero->Investigator->Charges = 3;
                Test->TestTrue(TEXT("Satchel placed for Dead Ground"), Hero->Primary->Request(nullptr, Hero->GetActorLocation() + FVector(400, 0, 0)));
                TestTick = Mode->GetCombatTick() + 7; Stage = 6;
                return false;
            case 6:
            {
                Test->TestEqual(TEXT("Satchel armed and waiting"), Hero->Primary->Satchels.Num(), 1);
                Test->TestTrue(TEXT("Dead Ground accepted with a trap prepared"), Kit->Request(EDMKitSlot::R, nullptr, Hero->GetActorLocation()));
                Test->TestTrue(TEXT("Dead Ground window open"), Kit->IsRActive());
                Test->TestTrue(TEXT("The ultimate spikes Madness"), Hero->Investigator->Madness >= 30);
                // Walk into the blast: the trigger is legitimate, so it is recorded rather than resolved.
                Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(400, 60, 0));
                EnemyHealth = Enemy->Health();
                TestTick = Mode->GetCombatTick() + 3; Stage = 7;
                return false;
            }
            case 7:
                Test->TestEqual(TEXT("Dead Ground defers the blast"), Enemy->Health(), EnemyHealth);
                Test->TestEqual(TEXT("The charge stays armed while deferred"), Hero->Primary->Satchels.Num(), 1);
                Test->TestTrue(TEXT("The trigger was recorded"), !Kit->Ledger.Tags.IsEmpty());
                TestTick = Mode->GetCombatTick() + FDMDeadGroundLedger::DelayTicks + 2; Stage = 8;
                return false;
            case 8:
                Test->TestTrue(TEXT("The deferred blast lands together"), Enemy->Health() <= EnemyHealth - 55);
                Test->TestEqual(TEXT("A trap that tagged is spent"), Hero->Primary->Satchels.Num(), 0);
                Test->TestTrue(TEXT("The ledger is cleared"), Kit->Ledger.Tags.IsEmpty());
                Test->TestTrue(TEXT("Kit state reaches the replication summary"), Kit->ReplicationSummary().Contains(TEXT("zones=")));
                GEditor->RequestEndPlayMap(); Stage = 9;
                return false;
            default: break;
            }
        }
        if (Stage == 9 && !GEditor->PlayWorld)
        {
            if (Window) { Window->RequestDestroyWindow(); Window.Reset(); }
            return true;
        }
        if (FPlatformTime::Seconds() > Deadline) { return Timeout(); }
        return false;
    }

private:
    bool Timeout()
    {
        Test->AddError(FString::Printf(TEXT("Sapper kit PIE verification timed out at stage %d."), Stage));
        GEditor->RequestEndPlayMap();
        if (Window) { Window->RequestDestroyWindow(); }
        return true;
    }
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    FString Original;
    bool bHadPreference = false;
    TWeakObjectPtr<ADMCombatant> Subject, Enemy;
    float EnemyHealth = 0;
    int32 TestTick = 0;
    int32 Stage = 0;
    double Deadline = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMSapperKitTest, "DreadMeridian.Editor.Kits.Sapper",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMSapperKitTest::RunTest(const FString& Parameters)
{
    if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run the kit check in an idle editor.")); return false; }
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FDMVerifySapperKit>(this));
    return true;
}

/**
 * The Photographer's kit: a flash that exposes and disrupts what it catches, Develop spending that Exposure, and
 * Impossible Photograph exposing the visible battlefield and holding those readings while it runs.
 */
class FDMVerifyPhotographerKit : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyPhotographerKit(FAutomationTestBase* InTest) : Test(InTest)
    {
        bHadPreference = GConfig->GetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), Original, GEditorPerProjectIni);
    }
    virtual ~FDMVerifyPhotographerKit() override
    {
        if (bHadPreference) { GConfig->SetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), *Original, GEditorPerProjectIni); }
        else { GConfig->RemoveKey(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), GEditorPerProjectIni); }
        GConfig->Flush(false, GEditorPerProjectIni);
        if (Settings) { Settings->RemoveFromRoot(); }
    }

    virtual bool Update() override
    {
        if (Stage == 0)
        {
            DMEditorPlaySelection::Save(TEXT("Photographer"));
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone);
            Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Photographer kit verification"))).ClientSize(FVector2D(640, 480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams Params;
            Params.EditorPlaySettings = Settings;
            Params.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox");
            Params.CustomPIEWindow = Window;
            Params.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(Params);
            GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 90; Stage = 1;
            return false;
        }
        if (Stage == 1 && GEditor->PlayWorld)
        {
            APlayerController* Player = GEditor->PlayWorld->GetFirstPlayerController();
            ADMCombatant* Hero = Player ? Cast<ADMCombatant>(Player->GetPawn()) : nullptr;
            if (!Hero) { return Timeout(); }
            Test->TestEqual(TEXT("PIE possesses the Photographer"), static_cast<int32>(Hero->Investigator->Kind), static_cast<int32>(EDMInvestigator::Photographer));
            Subject = Hero;
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            int32 N = 0;
            for (ADMCombatant* A : Mode->GetCombatants())
            {
                A->bProfileRange = true; A->NextAttackTick = 100000;
                A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(1400, -700 + N++ * 160, 95));
                if (A->bIsEnemy && !Enemy.IsValid()) { Enemy = A; }
                else if (A->bIsEnemy && !Second.IsValid()) { Second = A; }
            }
            Hero->SetActorLocation(FVector(-1200, 0, 95));
            Test->TestFalse(TEXT("Develop refused without Exposure"), Hero->Kit->Request(EDMKitSlot::E, Enemy.Get(), Enemy->GetActorLocation()));
            Test->TestFalse(TEXT("The photograph needs something in view"), Hero->Kit->Request(EDMKitSlot::R, nullptr, Hero->GetActorLocation()));
            Test->TestTrue(TEXT("Refusals spend no cooldown"), Hero->Kit->IsReady(EDMKitSlot::E) && Hero->Kit->IsReady(EDMKitSlot::R));

            Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(250, 0, 0));
            EnemyHealth = Enemy->Health();
            Test->TestTrue(TEXT("Flashbulb accepted"), Hero->Kit->Request(EDMKitSlot::W, nullptr, Hero->GetActorLocation() + FVector(400, 0, 0)));
            Test->TestTrue(TEXT("The flash exposes what it catches"), Hero->Investigator->PeekExposure(Enemy->EntityId) > 0);
            Test->TestTrue(TEXT("The flash staggers"), Enemy->StaggeredUntilTick > Mode->GetCombatTick());
            TestTick = Mode->GetCombatTick() + 2; Stage = 2;
            return false;
        }
        if (Stage >= 2 && GEditor->PlayWorld && Subject.IsValid() && Enemy.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            auto* Hero = Subject.Get();
            auto* Kit = Hero->Kit.Get();
            if (Mode->GetCombatTick() < TestTick) { return false; }
            switch (Stage)
            {
            case 2:
            {
                const float Exposure = Hero->Investigator->PeekExposure(Enemy->EntityId);
                Test->TestTrue(TEXT("Develop accepted on an exposed subject"), Kit->Request(EDMKitSlot::E, Enemy.Get(), Enemy->GetActorLocation()));
                Test->TestTrue(TEXT("Develop damages in proportion to Exposure"), Enemy->Health() <= EnemyHealth - Exposure * .5f);
                Test->TestEqual(TEXT("Develop spends the stored Exposure"), Hero->Investigator->PeekExposure(Enemy->EntityId), 0.f);
                Test->TestFalse(TEXT("Develop refused again with nothing stored"), Kit->Request(EDMKitSlot::E, Enemy.Get(), Enemy->GetActorLocation()));
                TestTick = Mode->GetCombatTick() + 2; Stage = 3;
                return false;
            }
            case 3:
            {
                // R: everything visible becomes heavily exposed, and those readings hold while the window runs.
                if (Second.IsValid()) { Second->SetActorLocation(Hero->GetActorLocation() + FVector(300, 200, 0)); }
                Test->TestFalse(TEXT("Develop is still cooling from its first use"), Kit->IsReady(EDMKitSlot::E));
                Test->TestTrue(TEXT("Impossible Photograph accepted"), Kit->Request(EDMKitSlot::R, nullptr, Hero->GetActorLocation()));
                Test->TestTrue(TEXT("The photograph exposes the visible battlefield"), Hero->Investigator->PeekExposure(Enemy->EntityId) >= 80);
                if (Second.IsValid()) { Test->TestTrue(TEXT("Every visible subject is captured"), Hero->Investigator->PeekExposure(Second->EntityId) >= 80); }
                Test->TestTrue(TEXT("The ultimate spikes Madness"), Hero->Investigator->Madness >= 30);
                // The window brings the running Develop cooldown forward, well short of the 6 seconds it started.
                Test->TestTrue(TEXT("The window brings Develop forward"), Kit->CooldownSeconds(EDMKitSlot::E) <= 2.1f);
                TestTick = Mode->GetCombatTick() + 22; Stage = 4;
                return false;
            }
            case 4:
            {
                Test->TestTrue(TEXT("Develop returns on the window's shorter wait"), Kit->IsReady(EDMKitSlot::E));
                EnemyHealth = Enemy->Health();
                const float Stored = Hero->Investigator->PeekExposure(Enemy->EntityId);
                Test->TestTrue(TEXT("Develop accepted inside the window"), Kit->Request(EDMKitSlot::E, Enemy.Get(), Enemy->GetActorLocation()));
                Test->TestTrue(TEXT("The developed shot still lands"), Enemy->Health() < EnemyHealth);
                Test->TestEqual(TEXT("Stored Exposure survives Develop while the photograph holds"), Hero->Investigator->PeekExposure(Enemy->EntityId), Stored);
                TestTick = Mode->GetCombatTick() + 70; Stage = 5;
                return false;
            }
            case 5:
                Test->TestFalse(TEXT("The window closes"), Kit->IsRActive());
                Test->TestFalse(TEXT("Exposure thaws with it"), Hero->Investigator->bExposureFrozen);
                GEditor->RequestEndPlayMap(); Stage = 6;
                return false;
            default: break;
            }
        }
        if (Stage == 6 && !GEditor->PlayWorld)
        {
            if (Window) { Window->RequestDestroyWindow(); Window.Reset(); }
            return true;
        }
        if (FPlatformTime::Seconds() > Deadline) { return Timeout(); }
        return false;
    }

private:
    bool Timeout()
    {
        Test->AddError(FString::Printf(TEXT("Photographer kit PIE verification timed out at stage %d."), Stage));
        GEditor->RequestEndPlayMap();
        if (Window) { Window->RequestDestroyWindow(); }
        return true;
    }
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    FString Original;
    bool bHadPreference = false;
    TWeakObjectPtr<ADMCombatant> Subject, Enemy, Second;
    float EnemyHealth = 0;
    int32 TestTick = 0;
    int32 Stage = 0;
    double Deadline = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMPhotographerKitTest, "DreadMeridian.Editor.Kits.Photographer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMPhotographerKitTest::RunTest(const FString& Parameters)
{
    if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run the kit check in an idle editor.")); return false; }
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FDMVerifyPhotographerKit>(this));
    return true;
}

/**
 * The Medium's kit: spirits called across the field and pulsing where they land, an intervention whose shape
 * depends on what the spirit is bound to, and a seance that stops the calling from exhausting them.
 */
class FDMVerifyMediumKit : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyMediumKit(FAutomationTestBase* InTest) : Test(InTest)
    {
        bHadPreference = GConfig->GetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), Original, GEditorPerProjectIni);
    }
    virtual ~FDMVerifyMediumKit() override
    {
        if (bHadPreference) { GConfig->SetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), *Original, GEditorPerProjectIni); }
        else { GConfig->RemoveKey(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), GEditorPerProjectIni); }
        GConfig->Flush(false, GEditorPerProjectIni);
        if (Settings) { Settings->RemoveFromRoot(); }
    }

    virtual bool Update() override
    {
        if (Stage == 0)
        {
            DMEditorPlaySelection::Save(TEXT("Medium"));
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone);
            Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Medium kit verification"))).ClientSize(FVector2D(640, 480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams Params;
            Params.EditorPlaySettings = Settings;
            Params.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox");
            Params.CustomPIEWindow = Window;
            Params.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(Params);
            GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 90; Stage = 1;
            return false;
        }
        if (Stage == 1 && GEditor->PlayWorld)
        {
            APlayerController* Player = GEditor->PlayWorld->GetFirstPlayerController();
            ADMCombatant* Hero = Player ? Cast<ADMCombatant>(Player->GetPawn()) : nullptr;
            if (!Hero) { return Timeout(); }
            Test->TestEqual(TEXT("PIE possesses the Medium"), static_cast<int32>(Hero->Investigator->Kind), static_cast<int32>(EDMInvestigator::Medium));
            Subject = Hero;
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            int32 N = 0;
            for (ADMCombatant* A : Mode->GetCombatants())
            {
                A->bProfileRange = true; A->NextAttackTick = 100000;
                A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(1400, -700 + N++ * 160, 95));
                if (A->bIsEnemy && !Enemy.IsValid()) { Enemy = A; }
                if (!A->bIsEnemy && A != Hero && !Ally.IsValid()) { Ally = A; }
            }
            Hero->SetActorLocation(FVector(-1200, 0, 95));
            Test->TestFalse(TEXT("Beckon refused with no spirits"), Hero->Kit->Request(EDMKitSlot::W, nullptr, Hero->GetActorLocation() + FVector(300, 0, 0)));
            Test->TestFalse(TEXT("Intercession refused with no spirits"), Hero->Kit->Request(EDMKitSlot::E, nullptr, Hero->GetActorLocation()));
            Test->TestFalse(TEXT("Open Seance refused with no spirits"), Hero->Kit->Request(EDMKitSlot::R, nullptr, Hero->GetActorLocation()));
            Test->TestTrue(TEXT("Refusals spend no cooldown"), Hero->Kit->IsReady(EDMKitSlot::W) && Hero->Kit->IsReady(EDMKitSlot::E) && Hero->Kit->IsReady(EDMKitSlot::R));

            // Bind a spirit to the enemy, then feed it Attention with Spirit Lash so it is worth calling on.
            Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(300, 0, 0));
            Ally->SetActorLocation(Hero->GetActorLocation() + FVector(0, 300, 0));
            Test->TestTrue(TEXT("Spirit bound to the enemy"), Hero->Primary->Request(Enemy.Get(), Enemy->GetActorLocation()));
            for (int32 I = 0; I < 3; ++I) { Hero->DealCombatDamage(Enemy.Get(), 1, TEXT("ability.basic_attack"), true); }
            TestTick = Mode->GetCombatTick() + 2; Stage = 2;
            return false;
        }
        if (Stage >= 2 && GEditor->PlayWorld && Subject.IsValid() && Enemy.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            auto* Hero = Subject.Get();
            auto* Kit = Hero->Kit.Get();
            if (Mode->GetCombatTick() < TestTick) { return false; }
            switch (Stage)
            {
            case 2:
            {
                ADMAbilityMarker* Spirit = Hero->Primary->Bindings.IsEmpty() ? nullptr : Hero->Primary->Bindings[0].Get();
                if (!Spirit) { Test->AddError(TEXT("No bound spirit to work with.")); return Timeout(); }
                SpiritId = Spirit->SpiritId;
                Attention = Hero->Investigator->PeekAttention(SpiritId);
                Test->TestTrue(TEXT("Spirit Lash feeds the bound spirit"), Attention >= 20);
                EnemyHealth = Enemy->Health();
                Test->TestTrue(TEXT("Intercession accepted on a listening spirit"), Kit->Request(EDMKitSlot::E, nullptr, Hero->GetActorLocation()));
                Test->TestTrue(TEXT("A hostile binding is struck"), Enemy->Health() < EnemyHealth);
                Test->TestTrue(TEXT("Calling on a spirit exhausts it"), Hero->Investigator->PeekAttention(SpiritId) < Attention);
                TestTick = Mode->GetCombatTick() + 2; Stage = 3;
                return false;
            }
            case 3:
            {
                // W: the spirit leaves the enemy and crosses to a chosen point, pulsing where it lands.
                const FVector Destination = Hero->GetActorLocation() + FVector(0, 300, 0);
                Test->TestTrue(TEXT("Beckon accepted"), Kit->Request(EDMKitSlot::W, nullptr, Destination));
                ADMAbilityMarker* Spirit = Hero->Primary->Bindings.IsEmpty() ? nullptr : Hero->Primary->Bindings[0].Get();
                Test->TestTrue(TEXT("The called spirit is in flight"), Spirit && Spirit->bTravelling);
                Test->TestTrue(TEXT("A called spirit leaves what it was bound to"), Spirit && Spirit->BoundTarget == nullptr);
                if (Ally.IsValid()) { AllyShield = Ally->Shield(); }
                TestTick = Mode->GetCombatTick() + 12; Stage = 4;
                return false;
            }
            case 4:
            {
                ADMAbilityMarker* Spirit = Hero->Primary->Bindings.IsEmpty() ? nullptr : Hero->Primary->Bindings[0].Get();
                Test->TestTrue(TEXT("The spirit arrives"), Spirit && !Spirit->bTravelling);
                if (Ally.IsValid()) { Test->TestTrue(TEXT("Arrival shields the ally it reaches"), Ally->Shield() > AllyShield); }
                Test->TestTrue(TEXT("The arrival point is recorded for the resource"), Spirit && Hero->Investigator->Spirits.Num() > 0);
                // R: the seance stops Intercession exhausting the spirit it calls on.
                Hero->Investigator->AddExposure(TEXT("unused"), 0, false, Mode->GetCombatTick());
                Hero->Investigator->ThinPlace(Spirit->GetActorLocation(), 40);
                Attention = Hero->Investigator->PeekAttention(SpiritId);
                Test->TestTrue(TEXT("Thin Places feeds the settled spirit"), Attention >= 20);
                Test->TestFalse(TEXT("Intercession is still cooling from its first use"), Kit->IsReady(EDMKitSlot::E));
                Test->TestTrue(TEXT("Open Seance accepted"), Kit->Request(EDMKitSlot::R, nullptr, Hero->GetActorLocation()));
                Test->TestTrue(TEXT("The ultimate spikes Madness"), Hero->Investigator->Madness >= 30);
                Test->TestTrue(TEXT("The seance brings Intercession forward"), Kit->CooldownSeconds(EDMKitSlot::E) <= 3.1f);
                TestTick = Mode->GetCombatTick() + 32; Stage = 5;
                return false;
            }
            case 5:
                Test->TestTrue(TEXT("Intercession returns on the seance's shorter wait"), Kit->IsReady(EDMKitSlot::E));
                Test->TestTrue(TEXT("Intercession accepted inside the seance"), Kit->Request(EDMKitSlot::E, nullptr, Hero->GetActorLocation()));
                Test->TestEqual(TEXT("The seance spares the spirit it calls on"), Hero->Investigator->PeekAttention(SpiritId), Attention);
                GEditor->RequestEndPlayMap(); Stage = 6;
                return false;
            default: break;
            }
        }
        if (Stage == 6 && !GEditor->PlayWorld)
        {
            if (Window) { Window->RequestDestroyWindow(); Window.Reset(); }
            return true;
        }
        if (FPlatformTime::Seconds() > Deadline) { return Timeout(); }
        return false;
    }

private:
    bool Timeout()
    {
        Test->AddError(FString::Printf(TEXT("Medium kit PIE verification timed out at stage %d."), Stage));
        GEditor->RequestEndPlayMap();
        if (Window) { Window->RequestDestroyWindow(); }
        return true;
    }
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    FString Original, SpiritId;
    bool bHadPreference = false;
    TWeakObjectPtr<ADMCombatant> Subject, Enemy, Ally;
    float EnemyHealth = 0, AllyShield = 0, Attention = 0;
    int32 TestTick = 0;
    int32 Stage = 0;
    double Deadline = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMMediumKitTest, "DreadMeridian.Editor.Kits.Medium",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMMediumKitTest::RunTest(const FString& Parameters)
{
    if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run the kit check in an idle editor.")); return false; }
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FDMVerifyMediumKit>(this));
    return true;
}

/**
 * The Smuggler's kit: a charge that runs through what it reaches, a brace that blunts incoming damage and can
 * end in a shove, and an altered state that strengthens the grab without waiving the Break layer.
 */
class FDMVerifySmugglerKit : public IAutomationLatentCommand
{
public:
    explicit FDMVerifySmugglerKit(FAutomationTestBase* InTest) : Test(InTest)
    {
        bHadPreference = GConfig->GetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), Original, GEditorPerProjectIni);
    }
    virtual ~FDMVerifySmugglerKit() override
    {
        if (bHadPreference) { GConfig->SetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), *Original, GEditorPerProjectIni); }
        else { GConfig->RemoveKey(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), GEditorPerProjectIni); }
        GConfig->Flush(false, GEditorPerProjectIni);
        if (Settings) { Settings->RemoveFromRoot(); }
    }

    virtual bool Update() override
    {
        if (Stage == 0)
        {
            DMEditorPlaySelection::Save(TEXT("Smuggler"));
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone);
            Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Smuggler kit verification"))).ClientSize(FVector2D(640, 480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams Params;
            Params.EditorPlaySettings = Settings;
            Params.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox");
            Params.CustomPIEWindow = Window;
            Params.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(Params);
            GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 90; Stage = 1;
            return false;
        }
        if (Stage == 1 && GEditor->PlayWorld)
        {
            APlayerController* Player = GEditor->PlayWorld->GetFirstPlayerController();
            ADMCombatant* Hero = Player ? Cast<ADMCombatant>(Player->GetPawn()) : nullptr;
            if (!Hero) { return Timeout(); }
            Test->TestEqual(TEXT("PIE possesses the Smuggler"), static_cast<int32>(Hero->Investigator->Kind), static_cast<int32>(EDMInvestigator::Smuggler));
            Subject = Hero;
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            int32 N = 0;
            for (ADMCombatant* A : Mode->GetCombatants())
            {
                A->bProfileRange = true; A->NextAttackTick = 100000;
                A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(1400, -700 + N++ * 160, 95));
                if (A->bIsEnemy && !Enemy.IsValid()) { Enemy = A; }
            }
            Hero->SetActorLocation(FVector(-1200, 0, 95));
            Test->TestFalse(TEXT("A charge needs a direction"), Hero->Kit->Request(EDMKitSlot::W, nullptr, Hero->GetActorLocation()));
            Test->TestTrue(TEXT("A refused charge spends no cooldown"), Hero->Kit->IsReady(EDMKitSlot::W));

            // W: an enemy on the path is run through and displaced along it.
            Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(220, 0, 0));
            EnemyHealth = Enemy->Health(); EnemyX = Enemy->GetActorLocation().X;
            HeroX = Hero->GetActorLocation().X;
            Test->TestTrue(TEXT("Shoulder Through accepted"), Hero->Kit->Request(EDMKitSlot::W, nullptr, Hero->GetActorLocation() + FVector(500, 0, 0)));
            Test->TestTrue(TEXT("The charge is running"), Hero->Kit->IsCharging());
            TestTick = Mode->GetCombatTick() + 8; Stage = 2;
            return false;
        }
        if (Stage >= 2 && GEditor->PlayWorld && Subject.IsValid() && Enemy.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            auto* Hero = Subject.Get();
            auto* Kit = Hero->Kit.Get();
            if (Mode->GetCombatTick() < TestTick) { return false; }
            switch (Stage)
            {
            case 2:
                Test->TestFalse(TEXT("The charge ends on its own"), Kit->IsCharging());
                Test->TestTrue(TEXT("The charge carried the Smuggler forward"), Hero->GetActorLocation().X > HeroX + 100);
                Test->TestTrue(TEXT("What it reached was struck"), Enemy->Health() < EnemyHealth);
                Test->TestTrue(TEXT("and displaced along the charge"), Enemy->GetActorLocation().X > EnemyX + 50);
                Test->TestTrue(TEXT("Contact builds Momentum"), Hero->Investigator->Momentum >= 15);
                // E: brace, then end it with a shove.
                Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(120, 0, 0));
                Test->TestTrue(TEXT("Dig In accepted"), Kit->Request(EDMKitSlot::E, nullptr, Hero->GetActorLocation()));
                Test->TestTrue(TEXT("The stance is up"), Kit->IsBraced());
                HeroHealth = Hero->Health();
                Hero->Investigator->Pressure(Mode->GetCombatTick(), 0);
                Momentum = Hero->Investigator->Momentum;
                Enemy->DealCombatDamage(Hero, 20, TEXT("ability.basic_attack"), true);
                Test->TestTrue(TEXT("Bracing blunts the blow"), Hero->Health() > HeroHealth - 20);
                Test->TestTrue(TEXT("Absorbed pressure feeds Momentum"), Hero->Investigator->Momentum > Momentum);
                EnemyHealth = Enemy->Health(); EnemyX = Enemy->GetActorLocation().X;
                TestTick = Mode->GetCombatTick() + 2; Stage = 3;
                return false;
            case 3:
                Test->TestTrue(TEXT("Recasting ends the stance with a shove"), Kit->Request(EDMKitSlot::E, nullptr, Hero->GetActorLocation()));
                Test->TestFalse(TEXT("The stance is down"), Kit->IsBraced());
                Test->TestTrue(TEXT("The shove lands"), Enemy->Health() < EnemyHealth);
                Test->TestTrue(TEXT("and pushes clear"), Enemy->GetActorLocation().X > EnemyX + 50);
                Test->TestFalse(TEXT("The stance cooldown starts from its end"), Kit->IsReady(EDMKitSlot::E));
                TestTick = Mode->GetCombatTick() + 2; Stage = 4;
                return false;
            case 4:
            {
                // R: the altered state strengthens the grab without waiving the Break layer.
                Enemy->bCommonEnemy = false; Enemy->Resolve->Reset();
                Test->TestFalse(TEXT("An unbroken elite cannot be grabbed normally"), Hero->Primary->Request(Enemy.Get(), Enemy->GetActorLocation()));
                const float Reach = Hero->GetAttackRange();
                Test->TestTrue(TEXT("Drowned Man Walking accepted"), Kit->Request(EDMKitSlot::R, nullptr, Hero->GetActorLocation()));
                Test->TestTrue(TEXT("The ultimate spikes Madness"), Hero->Investigator->Madness >= 30);
                Test->TestTrue(TEXT("It lends unnatural reach"), Hero->GetAttackRange() > Reach);
                Test->TestTrue(TEXT("and holds Momentum at a floor"), Hero->Investigator->Momentum >= 75 || Kit->IsRActive());
                Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(120, 0, 0));
                BreakBefore = Enemy->Break;
                Test->TestTrue(TEXT("The grab now lands on an unbroken elite"), Hero->Primary->Request(Enemy.Get(), Enemy->GetActorLocation()));
                Test->TestFalse(TEXT("but does not hold it"), Enemy->IsRestrained());
                Test->TestTrue(TEXT("it counts against its Resolve instead"), Enemy->Break > BreakBefore);
                TestTick = Mode->GetCombatTick() + 2; Stage = 5;
                return false;
            }
            case 5:
                Test->TestTrue(TEXT("Momentum holds at the floor"), Hero->Investigator->Momentum >= 75);
                TestTick = Mode->GetCombatTick() + 85; Stage = 6;
                return false;
            case 6:
                Test->TestFalse(TEXT("The altered state ends"), Kit->IsRActive());
                Test->TestEqual(TEXT("and the reach goes with it"), Hero->ReachBonus, 0.f);
                GEditor->RequestEndPlayMap(); Stage = 7;
                return false;
            default: break;
            }
        }
        if (Stage == 7 && !GEditor->PlayWorld)
        {
            if (Window) { Window->RequestDestroyWindow(); Window.Reset(); }
            return true;
        }
        if (FPlatformTime::Seconds() > Deadline) { return Timeout(); }
        return false;
    }

private:
    bool Timeout()
    {
        Test->AddError(FString::Printf(TEXT("Smuggler kit PIE verification timed out at stage %d."), Stage));
        GEditor->RequestEndPlayMap();
        if (Window) { Window->RequestDestroyWindow(); }
        return true;
    }
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    FString Original;
    bool bHadPreference = false;
    TWeakObjectPtr<ADMCombatant> Subject, Enemy;
    float EnemyHealth = 0, HeroHealth = 0, Momentum = 0, BreakBefore = 0;
    double EnemyX = 0, HeroX = 0;
    int32 TestTick = 0;
    int32 Stage = 0;
    double Deadline = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMSmugglerKitTest, "DreadMeridian.Editor.Kits.Smuggler",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMSmugglerKitTest::RunTest(const FString& Parameters)
{
    if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run the kit check in an idle editor.")); return false; }
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FDMVerifySmugglerKit>(this));
    return true;
}


/**
 * The AI profile assets must carry the kit abilities. `UDMAIProfile::Resolve` prefers the asset over the C++
 * defaults, so an asset authored before an ability existed silently shadows it and the bots never consider it -
 * a failure no Foundation test can see, because those build weights from DefaultWeights directly. This test
 * resolves each investigator profile the way the game does and checks its kit is complete.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMKitProfileTest, "DreadMeridian.Editor.Kits.Profiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDMKitProfileTest::RunTest(const FString&)
{
    struct FCase { EDMInvestigator Kind; const TCHAR* Name; EDMAIAction W, E, R; };
    const FCase Cases[] = {
        { EDMInvestigator::Sapper, TEXT("Sapper"), EDMAIAction::SuppressingFire, EDMAIAction::Tripwire, EDMAIAction::DeadGround },
        { EDMInvestigator::Photographer, TEXT("Photographer"), EDMAIAction::Flashbulb, EDMAIAction::Develop, EDMAIAction::ImpossiblePhotograph },
        { EDMInvestigator::Medium, TEXT("Medium"), EDMAIAction::Beckon, EDMAIAction::Intercession, EDMAIAction::OpenSeance },
        { EDMInvestigator::Smuggler, TEXT("Smuggler"), EDMAIAction::ShoulderThrough, EDMAIAction::DigIn, EDMAIAction::DrownedMan },
    };
    for (const FCase& Case : Cases)
    {
        UDMAIProfile* Profile = UDMAIProfile::Resolve(GetTransientPackage(), EDMSmuggler::None, Case.Kind);
        if (!TestNotNull(*FString::Printf(TEXT("%s profile resolves"), Case.Name), Profile)) { continue; }
        for (EDMAIAction Action : { Case.W, Case.E, Case.R })
        {
            TestTrue(*FString::Printf(TEXT("%s profile carries ability %d"), Case.Name, static_cast<int32>(Action)),
                Profile->Weights.Abilities.Contains(Action));
        }
    }
    return true;
}

#endif
