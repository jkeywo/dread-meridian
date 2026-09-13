#pragma once
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
/** Real PIE combat authority with paused tactical actors; assertions drive accepted runtime APIs. */
class FDMMechanicFixture : public IAutomationLatentCommand
{
public:
    FDMMechanicFixture(FAutomationTestBase* T,TFunction<void(UWorld*,ADMCombatGameMode*,ADMCombatant*)> Work,FString Map=TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox")) : Test(T),Body(MoveTemp(Work)),MapPath(MoveTemp(Map)) {}
    ~FDMMechanicFixture() { if (Settings) { Settings->RemoveFromRoot(); } }
    bool Update() override
    {
        if (!Started)
        {
            Settings=DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(),GetTransientPackage()); Settings->AddToRoot();
            Settings->SetPlayNetMode(PIE_Standalone); Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window=SNew(SWindow).Title(FText::FromString(TEXT("Mechanic verification"))).ClientSize(FVector2D(640,480)); FSlateApplication::Get().AddWindow(Window.ToSharedRef(),false);
            FRequestPlaySessionParams P; P.EditorPlaySettings=Settings; P.CustomPIEWindow=Window; P.GlobalMapOverride=MapPath; P.bAllowOnlineSubsystem=false;
            GEditor->RequestPlaySession(P); GEditor->StartQueuedPlaySessionRequest(); Started=true; Deadline=FPlatformTime::Seconds()+90; return false;
        }
        UWorld* W=GEditor->PlayWorld; auto* M=W ? W->GetAuthGameMode<ADMCombatGameMode>() : nullptr;
        if (!M || M->GetCombatants().Num()<5)
        { if (FPlatformTime::Seconds()<Deadline) { return false; } Test->AddError(TEXT("PIE fixture startup timeout")); GEditor->RequestEndPlayMap(); return true; }
        ADMCombatant* Hero=nullptr;
        for (ADMCombatant* A : M->GetCombatants())
        {
            if (A->GetController()) { A->GetController()->UnPossess(); } A->StopGoal(); A->SetAttackTarget(nullptr); A->SetAttackHold(true);
            A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(4000,4000,95)); if (!A->bIsEnemy && !Hero) { Hero=A; }
        }
        if (Hero) { Hero->SetActorLocation(FVector(-1200,0,95)); Body(W,M,Hero); } else { Test->AddError(TEXT("Missing investigator")); }
        GEditor->RequestEndPlayMap(); return true;
    }
private:
    FAutomationTestBase* Test; TFunction<void(UWorld*,ADMCombatGameMode*,ADMCombatant*)> Body;
    FString MapPath;
    bool Started=false; double Deadline=0; ULevelEditorPlaySettings* Settings=nullptr; TSharedPtr<SWindow> Window;
};
