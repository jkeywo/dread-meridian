#include "DMObjective.h"
#include "DMObjectiveCatalogue.h"
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
#if WITH_DEV_AUTOMATION_TESTS
class FDMObjectivePlayTest : public IAutomationLatentCommand
{
public:
    FAutomationTestBase* Test;
    explicit FDMObjectivePlayTest(FAutomationTestBase* T) : Test(T) {}
    ~FDMObjectivePlayTest() { if (Settings) { Settings->RemoveFromRoot(); } }
    bool Update() override
    {
        if (!Started)
        {
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(),GetTransientPackage()); Settings->AddToRoot();
            Settings->SetPlayNetMode(PIE_Standalone); Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Objective verification"))).ClientSize(FVector2D(640,480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(),false);
            FRequestPlaySessionParams P; P.EditorPlaySettings = Settings; P.CustomPIEWindow = Window;
            P.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox"); P.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(P); GEditor->StartQueuedPlaySessionRequest(); Started = true; Deadline = FPlatformTime::Seconds()+90; return false;
        }
        UWorld* W = GEditor->PlayWorld; auto* M = W ? W->GetAuthGameMode<ADMCombatGameMode>() : nullptr;
        if (!M || M->GetCombatants().Num()<5)
        { if (FPlatformTime::Seconds()<Deadline) { return false; } Test->AddError(TEXT("PIE startup timed out")); GEditor->RequestEndPlayMap(); return true; }
        ADMCombatant* Hero = nullptr;
        for (ADMCombatant* A : M->GetCombatants())
        {
            if (A->GetController()) { A->GetController()->UnPossess(); }
            A->StopGoal(); A->SetAttackTarget(nullptr); A->SetAttackHold(true); A->GetCharacterMovement()->DisableMovement(); A->SetActorLocation(FVector(4000,4000,95));
            if (!A->bIsEnemy && !Hero) { Hero = A; }
        }
        if (!Hero) { Test->AddError(TEXT("Missing investigator")); GEditor->RequestEndPlayMap(); return true; }
        int32 Tick = M->GetCombatTick()+1;
        for (const FString& Id : DMObjectiveCatalogue::Ids())
        {
            auto* O = W->SpawnActor<ADMObjective>();
            Test->TestTrue(Id+TEXT(" configures"),O->ConfigureAuthored(Id,FVector(-1200,0,35),3,TEXT("test.")+Id));
            int32 Budget = 2000;
            while (!O->IsTerminal() && --Budget>0)
            {
                const FDMObjectiveStep S = *O->Current();
                Hero->SetActorLocation(O->PublicState.PayloadLocation+FVector(0,0,60));
                if (S.Verb == EDMObjectiveVerb::Destroy)
                { Test->TestNotNull(TEXT("Authored destructible"),O->Destructible.Get()); if (O->Destructible) { Hero->DealCombatDamage(O->Destructible,10000,TEXT("test.objective")); } }
                else if (S.Verb == EDMObjectiveVerb::Sequence)
                { if (O->bSequenceReady) { O->Interact(Hero,S.Sequence[O->PublicState.Progress]); } }
                else
                {
                    if (!O->IsInteracting(Hero)) { Test->TestTrue(TEXT("Nearby interaction accepted"),O->Interact(Hero)); }
                    if (S.Verb == EDMObjectiveVerb::Carry) { Hero->SetActorLocation(S.Destination+FVector(0,0,60)); }
                }
                O->Step(Tick++);
            }
            Test->TestEqual(Id+TEXT(" completes through gameplay verbs"),O->PublicState.State,EDMObjectiveState::Completed);
            Test->TestTrue(TEXT("Reward committed exactly once"),O->PublicState.bRewarded);
            const int32 XP = Hero->Progression->Get().XP;
            O->Step(Tick++); Test->TestEqual(TEXT("Completion cannot repeat XP"),Hero->Progression->Get().XP,XP);
            O->Destroy();
        }
        GEditor->RequestEndPlayMap(); return true;
    }
private:
    bool Started = false; double Deadline = 0;
    ULevelEditorPlaySettings* Settings = nullptr; TSharedPtr<SWindow> Window;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMObjectivePlayAutomation,"DreadMeridian.Editor.ObjectiveCatalogue",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMObjectivePlayAutomation::RunTest(const FString&) { ADD_LATENT_AUTOMATION_COMMAND(FDMObjectivePlayTest(this)); return true; }
#endif
