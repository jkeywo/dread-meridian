#include "DMEditorPlaySelection.h"
#include "DMCombatant.h"
#include "DMCombatPlayerController.h"
#include "Components/BoxComponent.h"
#include "DMCombatGameMode.h"
#include "DMAbilityMarker.h"
#include "DMScroungePickup.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "ToolMenus.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS
class FDMVerifyBaseQ : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyBaseQ(FAutomationTestBase* InTest) : Test(InTest)
    {
        bHadPreference = GConfig->GetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), Original, GEditorPerProjectIni);
    }
    virtual ~FDMVerifyBaseQ() override
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
            if (Index == 4) { return true; }
            DMEditorPlaySelection::Save(DMEditorPlaySelection::Choices()[Index]);
            if (!Settings)
            {
                Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
                Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone);
                Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            }
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Character picker verification"))).ClientSize(FVector2D(640, 480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams Params;
            Params.EditorPlaySettings = Settings;
            Params.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox");
            Params.CustomPIEWindow = Window;
            Params.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(Params);
            GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 45; Stage = 1;
            return false;
        }
        if (Stage == 1 && GEditor->PlayWorld)
        {
            APlayerController* Player = GEditor->PlayWorld->GetFirstPlayerController();
            ADMCombatant* Hero = Player ? Cast<ADMCombatant>(Player->GetPawn()) : nullptr;
            if (Hero)
            {
                Test->TestEqual(*FString::Printf(TEXT("PIE possesses selected %s"), *DMEditorPlaySelection::Choices()[Index]),
                    static_cast<int32>(Hero->Investigator->Kind), Index + 1);
                Subject = Hero;
                auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
                int32 N = 0;
                for (ADMCombatant* A : Mode->GetCombatants())
                {
                    A->bProfileRange = true; A->NextAttackTick = 100000;
                    A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(900, -600 + N++ * 160, 95));
                    if (A->bIsEnemy && !Enemy.IsValid()) { Enemy = A; }
                    if (!A->bIsEnemy && A != Hero && !Ally.IsValid()) { Ally = A; }
                }
                Hero->SetActorLocation(FVector(-300, 0, 95)); Enemy->SetActorLocation(FVector(-180, 0, 95)); Ally->SetActorLocation(FVector(-300, 150, 95));
                Test->TestFalse(TEXT("Out-of-range Q rejected"), Hero->Primary->Request(nullptr, FVector(8000, 0, 95)));
                const auto Kind = Hero->Investigator->Kind;
                if (Kind == EDMInvestigator::Smuggler)
                {
                    Enemy->bCommonEnemy = false;
                    Test->TestFalse(TEXT("Unbroken elite cannot be grabbed"), Hero->Primary->Request(Enemy.Get(), Enemy->GetActorLocation()));
                    Enemy->bBreakVulnerable = true;
                    Test->TestTrue(TEXT("Break-vulnerable elite passes gate"), Hero->Primary->Validate(Enemy.Get(), Enemy->GetActorLocation()).IsEmpty());
                    Enemy->bCommonEnemy = true; Enemy->bBreakVulnerable = false;
                }
                EnemyHealth = Enemy->Health(); HeroHealth = Hero->Health();
                ADMCombatant* Target = Kind == EDMInvestigator::Sapper ? nullptr : Kind == EDMInvestigator::Medium ? Ally.Get() : Enemy.Get();
                Test->TestTrue(TEXT("Base Q activation accepted through GAS"), Hero->Primary->Request(Target, Enemy->GetActorLocation()));
                if (Kind == EDMInvestigator::Sapper)
                {
                    Test->TestEqual(TEXT("Placement spends one charge"), Hero->Investigator->Charges, 1);
                    Test->TestFalse(TEXT("Immediate detonation rejected while arming"), Hero->Primary->Request(nullptr, Hero->GetActorLocation(), true));
                }
                if (Kind == EDMInvestigator::Photographer)
                {
                    Enemy->bTelegraphActive = true;
                    Test->TestFalse(TEXT("Cannot basic attack during Frame"), Hero->TryAttack(Enemy.Get()));
                }
                TestTick = Mode->GetCombatTick() + 6; Stage = 3;
            }
        }
        if (Stage >= 3 && GEditor->PlayWorld && Subject.IsValid())
        {
            auto* Mode = GEditor->PlayWorld->GetAuthGameMode<ADMCombatGameMode>();
            auto* Hero = Subject.Get(); auto* Q = Hero->Primary.Get();
            if (Mode->GetCombatTick() < TestTick) { return false; }
            const auto Kind = Hero->Investigator->Kind;
            bool bDone = true;
            if (Kind == EDMInvestigator::Sapper)
            {
                bDone = false;
                if (Stage == 3)
                {
                    Test->TestEqual(TEXT("Player charge triggers automatically after arming"), Q->Satchels.Num(), 0);
                    Test->TestEqual(TEXT("Proximity damage goes through shared resolver"), Enemy->Health(), EnemyHealth - 55);
                    Test->TestEqual(TEXT("Explosion does not damage caster"), Hero->Health(), HeroHealth);
                    Test->TestFalse(TEXT("No double detonation"), Q->Request(nullptr, Hero->GetActorLocation(), true));
                    for (int32 I = 0; I < 2; ++I)
                    { auto* Pickup = GEditor->PlayWorld->SpawnActor<ADMScroungePickup>(Hero->GetActorLocation(), FRotator::ZeroRotator); Pickup->Tick(.1f); }
                    Test->TestEqual(TEXT("Scavenging replenishes spent charge"), Hero->Investigator->Charges, 2);
                    TestTick = Mode->GetCombatTick() + 3; Stage = 4;
                }
                else if (Stage == 4)
                {
                    Hero->SetActorLocation(FVector(-2100, 0, 95));
                    Test->TestTrue(TEXT("Charge can be placed in expanded map area away from enemies"), Q->Request(nullptr, Hero->GetActorLocation() + FVector(0, -500, 0)));
                    TestTick = Mode->GetCombatTick() + 6; Stage = 5;
                }
                else if (Stage == 5)
                {
                    Test->TestEqual(TEXT("Untriggered armed charge persists"), Q->Satchels.Num(), 1);
                    Test->TestTrue(TEXT("Manual detonation remains available"), Q->Request(nullptr, Hero->GetActorLocation(), true));
                    Test->TestEqual(TEXT("Remote blast does not damage distant enemy"), Enemy->Health(), EnemyHealth - 55);
                    Enemy->SetActorLocation(Hero->GetActorLocation() + FVector(300, 0, 0));
                    Ally->SetActorLocation(Hero->GetActorLocation() + FVector(150, 0, 0));
                    auto* Wall = GEditor->PlayWorld->SpawnActor<AActor>();
                    auto* Box = NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Box);
                    Box->SetBoxExtent(FVector(20, 100, 150)); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                    Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
                    Box->RegisterComponent(); Wall->SetActorLocation(Hero->GetActorLocation() + FVector(200, 0, 0));
                    Hero->NextAttackTick = 0;
                    const float HealthBehindCover = Enemy->Health();
                    Hero->TryAttack(Enemy.Get());
                    Test->TestEqual(TEXT("World cover prevents basic attack damage"), Enemy->Health(), HealthBehindCover);
                    Test->TestEqual(TEXT("Blocked shot does not spend attack cooldown"), Hero->NextAttackTick, 0); Wall->Destroy();
                    auto* PC = CastChecked<ADMCombatPlayerController>(Hero->GetController());
                    PC->StartAutoAttack();
                    Test->TestTrue(TEXT("Attack command acquires nearest enemy"), PC->GetSelectedTarget() == Enemy.Get());
                    EnemyHealth = Enemy->Health(); TestTick = Mode->GetCombatTick() + 20; Stage = 6;
                }
                else if (Stage == 6)
                {
                    Test->TestTrue(TEXT("Player repeatedly auto-attacks through allied bodies"), Enemy->Health() <= EnemyHealth - Hero->AttackDamage * 2);
                    bDone = true;
                }
            }
            else if (Kind == EDMInvestigator::Photographer)
            {
                Test->TestTrue(TEXT("Frame produces target Exposure"), !Hero->Investigator->Exposure.IsEmpty() && Hero->Investigator->Exposure[0].Value >= 12);
                Test->TestTrue(TEXT("Exposure increases precision rifle damage"), Hero->Investigator->DamageMultiplier(Enemy->EntityId, false) > 1);
                Q->CancelChannel(); Test->TestFalse(TEXT("Cancel clears frame"), IsValid(Q->FrameTarget));
                Test->TestFalse(TEXT("Frame cancellation starts cooldown"), Q->Request(Enemy.Get(), Enemy->GetActorLocation()));
            }
            else if (Kind == EDMInvestigator::Medium)
            {
                bDone = false;
                if (Stage == 3)
                {
                    Test->TestTrue(TEXT("Ally spirit protects its target"), Ally->SpiritProtection >= .1f);
                    TestTick = Mode->GetCombatTick() + 25; Stage = 4;
                }
                else if (Stage == 4)
                {
                    Test->TestTrue(TEXT("Enemy binding accepted"), Q->Request(Enemy.Get(), Enemy->GetActorLocation()));
                    Hero->DealCombatDamage(Enemy.Get(), 1, TEXT("ability.basic_attack"), true);
                    TestTick = Mode->GetCombatTick() + 2; Stage = 5;
                }
                else if (Stage == 5)
                {
                    Test->TestTrue(TEXT("Enemy spirit slows more strongly with Attention"), Enemy->SpiritSlow > .1f);
                    Test->TestTrue(TEXT("Spirit Lash feeds only matching spirit"), Hero->Investigator->Spirits.Num() == 2 && Hero->Investigator->Spirits[1].Value >= 12 && Hero->Investigator->Spirits[0].Value == 0);
                    TestTick = Mode->GetCombatTick() + 25; Stage = 6;
                }
                else if (Stage == 6)
                {
                    Test->TestTrue(TEXT("Ground binding accepted"), Q->Request(nullptr, Hero->GetActorLocation()));
                    TestTick = Mode->GetCombatTick() + 2; Stage = 7;
                }
                else
                {
                    Test->TestTrue(TEXT("Ground spirit protects nearby caster"), Hero->SpiritProtection >= .1f);
                    Test->TestEqual(TEXT("Three independent bindings"), Hero->Investigator->Spirits.Num(), 3);
                    bDone = true;
                }
            }
            else
            {
                Test->TestTrue(TEXT("Clinch restrains target"), Enemy->IsRestrained());
                Test->TestFalse(TEXT("Held target cannot attack"), Enemy->TryAttack(Hero));
                const FVector Before = Enemy->GetActorLocation();
                Test->TestTrue(TEXT("Recast throws in chosen direction"), Q->Request(nullptr, Before + FVector(500, 0, 0)));
                Test->TestFalse(TEXT("Throw releases restraint"), Enemy->IsRestrained());
                Test->TestTrue(TEXT("Throw displaces through collision sweep"), Enemy->GetActorLocation().X > Before.X + 100);
                Test->TestTrue(TEXT("Clinch and throw build Momentum"), Hero->Investigator->Momentum >= 20);
                Test->TestTrue(TEXT("Throw starts cooldown"), Q->Cooldown > 0);
            }
            if (bDone) { GEditor->RequestEndPlayMap(); Stage = 2; }
        }
        if (Stage == 2 && !GEditor->PlayWorld)
        {
            if (Window) { Window->RequestDestroyWindow(); Window.Reset(); }
            ++Index; Subject.Reset(); Enemy.Reset(); Ally.Reset(); Stage = 0; return false;
        }
        if (FPlatformTime::Seconds() > Deadline)
        {
            Test->AddError(TEXT("Character picker PIE startup/teardown timed out."));
            GEditor->RequestEndPlayMap();
            if (Window) { Window->RequestDestroyWindow(); }
            return true;
        }
        return false;
    }
private:
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    FString Original;
    bool bHadPreference = false;
    TWeakObjectPtr<ADMCombatant> Subject, Enemy, Ally;
    float EnemyHealth = 0, HeroHealth = 0;
    int32 TestTick = 0;
    int32 Index = 0, Stage = 0;
    double Deadline = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMBaseQTest, "DreadMeridian.Editor.BaseQ",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMBaseQTest::RunTest(const FString& Parameters)
{
    if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run the picker check in an idle editor.")); return false; }
    UToolMenu* Menu = UToolMenus::Get()->FindMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    TestTrue(TEXT("Character dropdown registered beside Play"), Menu && Menu->FindSection("Play") && Menu->FindSection("Play")->FindEntry("DreadMeridian.CharacterPicker"));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FDMVerifyBaseQ>(this));
    return true;
}
#endif
