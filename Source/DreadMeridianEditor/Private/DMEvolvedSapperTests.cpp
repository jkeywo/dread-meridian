#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMRecoverySupply.h"
#include "DMAbilityMarker.h"
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
class FDMVerifySapperEvolution : public IAutomationLatentCommand
{
public:
    explicit FDMVerifySapperEvolution(FAutomationTestBase* InTest) : Test(InTest) {}
    ~FDMVerifySapperEvolution() { if (Settings) { Settings->RemoveFromRoot(); } }
    bool Update() override
    {
        if (Stage == 0)
        {
            Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
            Settings->AddToRoot(); Settings->SetPlayNetMode(PIE_Standalone); Settings->SetPlayNumberOfClients(1); Settings->SetRunUnderOneProcess(true);
            Window = SNew(SWindow).Title(FText::FromString(TEXT("Madness verification"))).ClientSize(FVector2D(640,480));
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
                if (A->bIsEnemy && !Enemy.IsValid()) { Enemy = A; }
            }

            for (uint8 Node = 1; Node <= 5; ++Node)
            {
                auto* A = W->SpawnActor<ADMCombatant>(FVector(-1200,0,95), FRotator::ZeroRotator);
                A->InitializeCombatant(FString::Printf(TEXT("test.sapper.%d"), Node), false, 200, 10, 0);
                A->InitializeInvestigator(EDMInvestigator::Sapper, false); A->ControlKind = TEXT("human");
                A->GetCharacterMovement()->DisableMovement();
                A->Progression->Award(TEXT("test.build"),1200);
                for (uint8 Slot = 0; Slot < 3; ++Slot)
                { A->Progression->Choose(Slot, Node <= 2 ? Node : Node == 5 ? 2 : 1); if (Node > 2) { A->Progression->Choose(Slot,Node); } }
                Enemy->InitializeCombatant(TEXT("test.enemy"),true,1000,10,0);
                Enemy->SetActorLocation(FVector(-1100,0,95));
                A->Primary->ApplySatchel(Enemy.Get(), FVector(-1100,0,18));
                Test->TestTrue(TEXT("Every evolved blast damages"), Enemy->Health() < 1000);
                Test->TestTrue(TEXT("Suppression cast"), A->Kit->Request(EDMKitSlot::W,nullptr,FVector(-800,0,95)));
                Test->TestTrue(TEXT("Suppression marker created"), !A->Kit->Zones.IsEmpty());
                if (Node == 5)
                {
                    const int32 Until = A->Kit->Zones[0]->ExpiresTick;
                    Test->TestTrue(TEXT("Walking Fire reaims during cooldown"), A->Kit->Request(EDMKitSlot::W,nullptr,FVector(-1200,300,95)));
                    Test->TestEqual(TEXT("Sweep never extends lifetime"), A->Kit->Zones[0]->ExpiresTick, Until);
                    Test->TestTrue(TEXT("Sweep changed direction"), A->Kit->Zones[0]->Direction.Y > .9f);
                }
                const float Half = Node == 1 || Node == 3 || Node == 4 ? 300 : 180;
                Test->TestTrue(TEXT("Evolved wire placement"), A->Kit->RequestWire(FVector(-1000,-Half,95),FVector(-1000,Half,95)));
                if (Node == 1 || Node == 3 || Node == 4)
                { Test->TestTrue(TEXT("Reinforced wire exceeds base length"), FVector::Dist2D(A->Kit->Wires[0]->GetActorLocation(), A->Kit->Wires[0]->WireEnd) > 500); }
                if (Node == 5)
                {
                    auto* Empty = W->SpawnActor<ADMAbilityMarker>(FVector(-1500,500,18),FRotator::ZeroRotator);
                    Empty->ArmedTick = 0; Empty->Serial = 101; A->Primary->Satchels.Add(Empty);
                    Test->TestFalse(TEXT("Daisy Chain never spends empty charge"), A->Primary->TriggerNearbySatchel(Empty->GetActorLocation(),100));
                    Test->TestEqual(TEXT("Empty charge retained"), A->Primary->Satchels.Num(),1);
                    A->Kit->Wires[0]->ArmedTick = 0;
                    Enemy->SetActorLocation(FVector(-1040,0,95)); A->Kit->Step(M->GetCombatTick());
                    Enemy->SetActorLocation(FVector(-960,0,95)); A->Kit->Step(M->GetCombatTick()+1);
                    Test->TestEqual(TEXT("Resetting wire survives crossing"), A->Kit->Wires.Num(),1);
                    Test->TestTrue(TEXT("Resetting wire has reset delay"), A->Kit->Wires[0]->ArmedTick > M->GetCombatTick());
                }
                A->Kit->Cancel(true); A->Destroy();
            }
            GEditor->RequestEndPlayMap(); Stage = 4; return false;
        }
        if (Stage == 4 && !W) { if (Window) { Window->RequestDestroyWindow(); } return true; }
        if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("Family test timed out")); GEditor->RequestEndPlayMap(); return true; }
        return false;
    }
private:
    FAutomationTestBase* Test;
    ULevelEditorPlaySettings* Settings = nullptr;
    TSharedPtr<SWindow> Window;
    TWeakObjectPtr<ADMCombatant> Hero, Enemy;
    int32 Stage = 0;
    double Deadline = 0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMSapperEvolutionTest, "DreadMeridian.Editor.Evolution.Sapper",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDMSapperEvolutionTest::RunTest(const FString&)
{ ADD_LATENT_AUTOMATION_COMMAND(FDMVerifySapperEvolution(this)); return true; }
#endif
