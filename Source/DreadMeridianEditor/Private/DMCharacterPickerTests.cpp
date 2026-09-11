#include "DMEditorPlaySelection.h"
#include "DMCombatant.h"
#include "DMSquadController.h"
#include "Editor.h"
#include "Engine/World.h"
#include "ToolMenus.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS
class FDMVerifyCharacterPicker : public IAutomationLatentCommand
{
public:
    explicit FDMVerifyCharacterPicker(FAutomationTestBase* InTest) : Test(InTest)
    {
        OriginalBot = DMEditorPlaySelection::LoadBotControl();
        bHadPreference = GConfig->GetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), Original, GEditorPerProjectIni);
    }
    virtual ~FDMVerifyCharacterPicker() override
    {
        if (bHadPreference) { GConfig->SetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), *Original, GEditorPerProjectIni); }
        else { GConfig->RemoveKey(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), GEditorPerProjectIni); }
        DMEditorPlaySelection::SaveBotControl(OriginalBot);
        GConfig->Flush(false, GEditorPerProjectIni);
        if (Settings) { Settings->RemoveFromRoot(); }
    }
    virtual bool Update() override
    {
        if (Stage == 0)
        {
            if (Index == 8) { return true; }
            DMEditorPlaySelection::SaveBotControl(Index >= 4);
            DMEditorPlaySelection::Save(DMEditorPlaySelection::Choices()[Index % 4]);
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
            ADMCombatant* Hero = Player ? Cast<ADMCombatant>(Index >= 4 ? Player->GetViewTarget() : Player->GetPawn()) : nullptr;
            if (Hero)
            {
                Test->TestEqual(*FString::Printf(TEXT("PIE possesses selected %s"), *DMEditorPlaySelection::Choices()[Index % 4]),
                    static_cast<int32>(Hero->Investigator->Kind), Index % 4 + 1);
                Test->TestEqual(TEXT("Checkbox chooses control mode"), Hero->ControlKind, FString(Index >= 4 ? TEXT("bot") : TEXT("human")));
                if (Index >= 4)
                {
                    Test->TestNull(TEXT("Observer does not possess bot"), Player->GetPawn());
                    Test->TestNotNull(TEXT("Selected hero retains squad AI"), Cast<ADMSquadController>(Hero->GetController()));
                }
                GEditor->RequestEndPlayMap(); Stage = 2;
            }
        }
        if (Stage == 2 && !GEditor->PlayWorld)
        {
            if (Window) { Window->RequestDestroyWindow(); Window.Reset(); }
            ++Index; Stage = 0; return false;
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
    bool OriginalBot = false;
    bool bHadPreference = false;
    int32 Index = 0, Stage = 0;
    double Deadline = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMCharacterPickerTest, "DreadMeridian.Editor.CharacterPickerPIE",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMCharacterPickerTest::RunTest(const FString& Parameters)
{
    if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Run the picker check in an idle editor.")); return false; }
    UToolMenu* Menu = UToolMenus::Get()->FindMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    TestTrue(TEXT("Character dropdown registered beside Play"), Menu && Menu->FindSection("Play") && Menu->FindSection("Play")->FindEntry("DreadMeridian.CharacterPicker"));
    TestTrue(TEXT("Bot checkbox registered beside picker"), Menu && Menu->FindSection("Play") && Menu->FindSection("Play")->FindEntry("DreadMeridian.BotControl"));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FDMVerifyCharacterPicker>(this));
    return true;
}
#endif
