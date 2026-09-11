#include "Modules/ModuleManager.h"
#include "DMEditorPlaySelection.h"
#include "Editor.h"
#include "ToolMenus.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "DreadMeridianEditor"
class FDreadMeridianEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDreadMeridianEditorModule::RegisterToolbar));
    }
    virtual void ShutdownModule() override
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
    }
private:
    void RegisterToolbar()
    {
        FToolMenuOwnerScoped Owner(this);
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
        auto Picker = SNew(SComboButton)
            .ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
            .ContentPadding(FMargin(8, 2))
            .IsEnabled_Lambda([] { return GEditor && !GEditor->PlayWorld; })
            .ToolTipText(LOCTEXT("Tooltip", "Choose your investigator for the next Play In Editor session. Saved for this project on this computer."))
            .OnGetMenuContent_Lambda([] {
                FMenuBuilder Builder(true, nullptr);
                const TArray<FText> Names = {
                    LOCTEXT("Sapper", "Trench Raider / Sapper"), LOCTEXT("Photographer", "Expedition Photographer"),
                    LOCTEXT("Medium", "Stage Medium"), LOCTEXT("Smuggler", "Bare-Knuckle Smuggler") };
                for (int32 Index = 0; Index < Names.Num(); ++Index)
                {
                    const FString Choice = DMEditorPlaySelection::Choices()[Index];
                    Builder.AddMenuEntry(Names[Index], FText::GetEmpty(), FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([Choice] { DMEditorPlaySelection::Save(Choice); }),
                            FCanExecuteAction(), FIsActionChecked::CreateLambda([Choice] { return DMEditorPlaySelection::Load() == Choice; })),
                        NAME_None, EUserInterfaceActionType::RadioButton);
                }
                return Builder.MakeWidget();
            })
            .ButtonContent()
            [ SNew(STextBlock)
                .Text_Lambda([] { return FText::Format(LOCTEXT("PlayAs", "Play as: {0}"), FText::FromString(DMEditorPlaySelection::Load())); }) ];
        FToolMenuEntry Entry = FToolMenuEntry::InitWidget("DreadMeridian.CharacterPicker", Picker, LOCTEXT("Character", "Investigator"));
        Entry.InsertPosition = FToolMenuInsert(NAME_None, EToolMenuInsertType::First);
        Menu->FindOrAddSection("Play").AddEntry(Entry);
        auto BotControl = SNew(SCheckBox)
            .IsEnabled_Lambda([] { return GEditor && !GEditor->PlayWorld; })
            .IsChecked_Lambda([] { return DMEditorPlaySelection::LoadBotControl() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
            .OnCheckStateChanged_Lambda([](ECheckBoxState Value) { DMEditorPlaySelection::SaveBotControl(Value == ECheckBoxState::Checked); })
            .ToolTipText(LOCTEXT("BotTooltip", "Let the squad AI control your selected investigator next PIE session. The camera and HUD follow them. Uncheck before Play to control them yourself."))
            [ SNew(STextBlock).Text(LOCTEXT("BotControlled", "Bot controlled")) ];
        FToolMenuEntry BotEntry = FToolMenuEntry::InitWidget("DreadMeridian.BotControl", BotControl, LOCTEXT("BotControl", "Bot controlled"));
        BotEntry.InsertPosition = FToolMenuInsert("DreadMeridian.CharacterPicker", EToolMenuInsertType::After);
        Menu->FindOrAddSection("Play").AddEntry(BotEntry);
    }
};
IMPLEMENT_MODULE(FDreadMeridianEditorModule, DreadMeridianEditor)
#undef LOCTEXT_NAMESPACE
