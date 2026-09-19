#include "DMTestArena.h"
#include "DMCombatant.h"
#include "DMAbilityMarker.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS
/** Real paused PIE world: input routes, placement transaction, movement and hazard freeze. */
class FDMCheckArena : public IAutomationLatentCommand
{
public:
    explicit FDMCheckArena(FAutomationTestBase* InTest) : Test(InTest) {}
    virtual ~FDMCheckArena() override { if (Settings) { Settings->RemoveFromRoot(); } }
    virtual bool Update() override
    {
        if (Stage==0)
        {
            Settings=DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(),GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone); Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window=SNew(SWindow).Title(FText::FromString(TEXT("Arena verification"))).ClientSize(FVector2D(1280,800));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(),true);
            FRequestPlaySessionParams Params; Params.EditorPlaySettings=Settings; Params.GlobalMapOverride=TEXT("/Game/DreadMeridian/Maps/L_TestArena");
            Params.CustomPIEWindow=Window; Params.bAllowOnlineSubsystem=false;
            GEditor->RequestPlaySession(Params); GEditor->StartQueuedPlaySessionRequest(); Deadline=FPlatformTime::Seconds()+45; Stage=1; return false;
        }
        if (Stage==5 && !GEditor->PlayWorld) { if (Window) { Window->RequestDestroyWindow(); } return true; }
        UWorld* World=GEditor->PlayWorld;
        auto* Mode=World ? World->GetAuthGameMode<ADMTestArenaGameMode>() : nullptr;
        auto* PC=World ? Cast<ADMTestArenaController>(World->GetFirstPlayerController()) : nullptr;
        auto* HUD=PC ? Cast<ADMTestArenaHUD>(PC->GetHUD()) : nullptr;
        auto* Hero=PC ? Cast<ADMCombatant>(PC->GetPawn()) : nullptr;
        if (Stage==1 && Mode && Hero && HUD)
        {
            Test->TestTrue(TEXT("PIE starts genuinely paused"),UGameplayStatics::IsGamePaused(World));
            Test->TestEqual(TEXT("Solo roster"),Mode->GetCombatants().Num(),1);
            Test->TestEqual(TEXT("Setup has not ticked"),Mode->GetCombatTick(),0);
            FString LoginError; Mode->PreLogin(TEXT(""),TEXT("remote"),FUniqueNetIdRepl(),LoginError);
            Test->TestFalse(TEXT("Remote login rejected"),LoginError.IsEmpty());
            Test->TestFalse(TEXT("Cover collision rejects placement"),Mode->Place({EDMArenaEnemy::Gunman},FVector(200,-1550,95)));
            Test->TestEqual(TEXT("Rejected placement creates nothing"),Mode->GetCombatants().Num(),1);
            // Settle the viewport and HUD before exercising its hit rectangles.
            At=FPlatformTime::Seconds(); Stage=2;
        }
        if (Stage==2 && Mode && FPlatformTime::Seconds()-At>.5)
        {
            // Hidden automation windows do not receive Slate paint calls. Render the real
            // viewport explicitly, so the test exercises the HUD's actual hit rectangles.
            PC->GetLocalPlayer()->ViewportClient->Viewport->Draw(false);
            Test->TestTrue(TEXT("Real paused HUD has rendered"),HUD->HasDrawnPanel());
            const FVector2D PlacePoint=HUD->PanelPoint(100,610);
            Test->TestTrue(TEXT("Paused setup panel is clickable"),HUD->Click(PlacePoint.X,PlacePoint.Y));
            Test->TestTrue(TEXT("Place button arms placement"),PC->bPlacing);
            FInputKeyEventArgs Cancel; Cancel.Key=EKeys::RightMouseButton; Cancel.Event=IE_Pressed;
            Test->TestTrue(TEXT("Right click consumed while placing"),PC->InputKey(Cancel));
            Test->TestFalse(TEXT("Right click cancels placement"),PC->bPlacing);
            Test->TestTrue(TEXT("Single enemy placed"),Mode->Place({EDMArenaEnemy::Bomber},Hero->GetActorLocation()+FVector(400,0,0)));
            Mode->Undo(true); Test->TestEqual(TEXT("Clear removes all enemies"),Mode->GetCombatants().Num(),1);
            Mode->Place({EDMArenaEnemy::Bomber},Hero->GetActorLocation()+FVector(400,0,0));
            const FVector2D FightPoint=HUD->PanelPoint(70,75);
            Test->TestTrue(TEXT("Fight button works under pause"),HUD->Click(FightPoint.X,FightPoint.Y));
            Test->TestTrue(TEXT("Fight unpauses world"),!UGameplayStatics::IsGamePaused(World));
            At=FPlatformTime::Seconds(); Stage=3;
        }
        if (Stage==3 && Mode && Mode->GetCombatTick()>=3)
        {
            Mode->FightOrPause(); StartTick=Mode->GetCombatTick(); Position=Hero->GetActorLocation(); Health=Hero->Health(); Shield=Hero->Shield();
            Enemy=Mode->GetCombatants().Last(); NextAttack=Enemy->NextAttackTick; Signature=Enemy->Smuggler->ResolveTick;
            Test->TestTrue(TEXT("Bomber signature is actually pending"),Signature>StartTick);
            Test->TestTrue(TEXT("Reinforcement placement while paused"),Mode->Place({EDMArenaEnemy::Crawler},FVector(600,600,95)));
            const int32 Count=Mode->GetCombatants().Num(); Mode->Undo(true); Test->TestEqual(TEXT("Cannot remove after fight"),Mode->GetCombatants().Num(),Count);
            At=FPlatformTime::Seconds(); Stage=4;
        }
        if (Stage==4 && Mode && FPlatformTime::Seconds()-At>.5)
        {
            Test->TestEqual(TEXT("Logical clock frozen"),Mode->GetCombatTick(),StartTick);
            Test->TestTrue(TEXT("Character movement frozen"),Hero->GetActorLocation().Equals(Position,.01));
            Test->TestEqual(TEXT("Health frozen"),Hero->Health(),Health); Test->TestEqual(TEXT("Shield frozen"),Hero->Shield(),Shield);
            Test->TestEqual(TEXT("Attack cooldown frozen"),Enemy->NextAttackTick,NextAttack);
            Test->TestEqual(TEXT("Pending hazard signature frozen"),Enemy->Smuggler->ResolveTick,Signature);
            GEditor->RequestEndPlayMap(); Stage=5;
        }
        if (FPlatformTime::Seconds()>Deadline)
        { Test->AddError(FString::Printf(TEXT("Arena PIE verification timed out at stage %d"),Stage)); GEditor->RequestEndPlayMap(); if (Window) { Window->RequestDestroyWindow(); } return true; }
        return false;
    }
private:
    FAutomationTestBase* Test; ULevelEditorPlaySettings* Settings=nullptr; TSharedPtr<SWindow> Window;
    TWeakObjectPtr<ADMCombatant> Enemy;
    int32 Stage=0,StartTick=0,NextAttack=0,Signature=0; double Deadline=0,At=0; FVector Position; float Health=0,Shield=0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMArenaPIETest,"DreadMeridian.Editor.TestArena",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMArenaPIETest::RunTest(const FString& Parameters)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMCheckArena(this)); return true; }
#endif
