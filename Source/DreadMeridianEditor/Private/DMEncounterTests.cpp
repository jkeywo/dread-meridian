#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMEncounterLayout.h"
#include "DMSquadController.h"
#include "DMSandboxArena.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#if WITH_DEV_AUTOMATION_TESTS
class FDMCheckEncounters : public IAutomationLatentCommand
{
public:
    explicit FDMCheckEncounters(FAutomationTestBase* InTest) : Test(InTest) {}
    virtual ~FDMCheckEncounters() override { if (Settings) { Settings->RemoveFromRoot(); } }
    virtual bool Update() override
    {
        if (Stage == 0)
        {
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone);
            Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Encounter verification"))).ClientSize(FVector2D(640, 480));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(), false);
            FRequestPlaySessionParams Params; Params.EditorPlaySettings = Settings;
            Params.GlobalMapOverride = TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox");
            Params.CustomPIEWindow = Window; Params.bAllowOnlineSubsystem = false;
            GEditor->RequestPlaySession(Params); GEditor->StartQueuedPlaySessionRequest();
            Deadline = FPlatformTime::Seconds() + 45; Stage = 1; return false;
        }
        UWorld* World = GEditor->PlayWorld;
        auto* Mode = World ? World->GetAuthGameMode<ADMCombatGameMode>() : nullptr;
        auto* PC = World ? World->GetFirstPlayerController() : nullptr;
        auto* Hero = PC ? Cast<ADMCombatant>(PC->GetPawn()) : nullptr;
        if (Stage == 1 && Mode && Hero)
        {
            Test->TestTrue(TEXT("Normal PIE uses authored encounters"), Mode->UsesEncounterLayout());
            Test->TestEqual(TEXT("Four investigators and eleven enemies"), Mode->GetCombatants().Num(), 15);
            if (Mode->GetCombatants().Num() != 15) { GEditor->RequestEndPlayMap(); Stage = 3; return false; }
            int32 Groups[4] = {};
            for (ADMCombatant* A : Mode->GetCombatants())
            {
                if (!A->bIsEnemy) { continue; }
                auto* Bot = Cast<ADMSquadController>(A->GetController());
                Test->TestNotNull(TEXT("Enemy has AI controller"), Bot);
                if (Bot && Bot->GetEncounterGroup() >= 0 && Bot->GetEncounterGroup() < 4) { ++Groups[Bot->GetEncounterGroup()]; }
            }
            for (int32 I = 0; I < 3; ++I) { Test->TestEqual(TEXT("Camp has three defenders"), Groups[I], 3); }
            Test->TestEqual(TEXT("Patrol has two members"), Groups[3], 2);
            for (TActorIterator<ADMSandboxArena> It(World); It; ++It)
            {
                TArray<UStaticMeshComponent*> Meshes; It->GetComponents(Meshes);
                for (auto* Mesh : Meshes) { if (Mesh->GetName() == TEXT("Floor"))
                { Test->TestTrue(TEXT("Saved map loads enlarged floor"), Mesh->Bounds.BoxExtent.X >= 2990 && Mesh->Bounds.BoxExtent.Y >= 2490); } }
            }
            PatrolStart = Mode->GetCombatants()[13]->GetActorLocation();
            StartTick = Mode->GetCombatTick(); Stage = 2;
        }
        if (Stage == 2 && Mode && Mode->GetCombatTick() >= StartTick + 10)
        {
            const auto& Actors = Mode->GetCombatants();
            Test->TestTrue(TEXT("Patrol physically advances while unengaged"), FVector::Dist2D(PatrolStart, Actors[13]->GetActorLocation()) > 100);
            Test->TestTrue(TEXT("Camp remains at home before contact"), FVector::Dist2D(DMEncounterLayout::EnemyPosition(0), Actors[4]->GetActorLocation()) < 100);
            for (ADMCombatant* A : Actors) { A->NextAttackTick = 100000; A->GetCharacterMovement()->DisableMovement(); }
            auto* Guard = Actors[4].Get(); auto* GuardBot = CastChecked<ADMSquadController>(Guard->GetController());
            Hero->SetActorLocation(Guard->GetActorLocation() + FVector(-300, 0, 0));
            GuardBot->Think(*Mode);
            Test->TestTrue(TEXT("Guard acquires nearby investigator"), Guard->GetAttackTarget() == Hero);
            auto* FriendBot = CastChecked<ADMSquadController>(Actors[5]->GetController()); FriendBot->Think(*Mode);
            Test->TestTrue(TEXT("Camp responds together"), Actors[5]->GetAttackTarget() == Hero);
            Hero->SetActorLocation(FVector(-2600, 0, 95));
            Guard->SetActorLocation(FVector(-2400, 0, 95)); GuardBot->Think(*Mode);
            Test->TestNull(TEXT("Leash releases distant target"), Guard->GetAttackTarget());
            Test->TestTrue(TEXT("Leash clears stale threat"), Guard->Threat.IsEmpty());
            auto* PatrolBot = CastChecked<ADMSquadController>(Actors[13]->GetController());
            Actors[13]->SetActorLocation(DMEncounterLayout::PatrolPoint(PatrolBot->GetPatrolWaypoint()));
            const int32 Before = PatrolBot->GetPatrolWaypoint(); PatrolBot->Think(*Mode);
            Test->TestEqual(TEXT("Patrol advances to next waypoint"), PatrolBot->GetPatrolWaypoint(), (Before + 1) % 4);
            GEditor->RequestEndPlayMap(); Stage = 3;
        }
        if (Stage == 3 && !GEditor->PlayWorld) { if (Window) { Window->RequestDestroyWindow(); } return true; }
        if (FPlatformTime::Seconds() > Deadline)
        { Test->AddError(TEXT("Encounter PIE check timed out")); GEditor->RequestEndPlayMap(); return true; }
        return false;
    }
private:
    FAutomationTestBase* Test; ULevelEditorPlaySettings* Settings = nullptr; TSharedPtr<SWindow> Window;
    int32 Stage = 0, StartTick = 0; double Deadline = 0; FVector PatrolStart;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMEncounterTest, "DreadMeridian.Editor.EnemyEncounters",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMEncounterTest::RunTest(const FString& Parameters)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMCheckEncounters(this)); return true; }
#endif
