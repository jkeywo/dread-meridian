#include "DMCombatant.h"
#include "DMCombatPresentation.h"
#include "DMCombatGameMode.h"
#include "DMGameState.h"
#include "DMAbilityMarker.h"
#include "DMSquadController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#if WITH_DEV_AUTOMATION_TESTS
class FDMSmugglerScenario : public IAutomationLatentCommand
{
public:
    explicit FDMSmugglerScenario(FAutomationTestBase* T):Test(T){}
    ~FDMSmugglerScenario() { if(Settings) { Settings->RemoveFromRoot(); } }
    void Freeze(ADMCombatant* A)
    {
        A->NextAttackTick=100000; A->Smuggler->NextSignatureTick=100000;
        A->GetCharacterMovement()->DisableMovement(); A->StopGoal(); A->SetAttackTarget(nullptr);
        if(A->GetController() && !A->IsPlayerControlled()) { A->GetController()->UnPossess(); }
    }
    virtual bool Update() override
    {
        if(Stage==0)
        {
            Settings=DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(),GetTransientPackage());Settings->AddToRoot();
            Settings->SetPlayNetMode(PIE_Standalone);Settings->SetPlayNumberOfClients(1);Settings->SetRunUnderOneProcess(true);
            Window=SNew(SWindow).Title(FText::FromString(TEXT("Smuggler faction verification"))).ClientSize(FVector2D(800,600));
            FSlateApplication::Get().AddWindow(Window.ToSharedRef(),false);
            FRequestPlaySessionParams P;P.EditorPlaySettings=Settings;P.GlobalMapOverride=TEXT("/Game/DreadMeridian/Maps/L_CombatSandbox");P.CustomPIEWindow=Window;P.bAllowOnlineSubsystem=false;
            GEditor->RequestPlaySession(P);GEditor->StartQueuedPlaySessionRequest();Deadline=FPlatformTime::Seconds()+90;Stage=1;return false;
        }
        UWorld* W=GEditor->PlayWorld;auto* M=W?W->GetAuthGameMode<ADMCombatGameMode>():nullptr;
        if(Stage==1 && M && W->GetFirstPlayerController() && W->GetFirstPlayerController()->GetPawn())
        {
            Hero=CastChecked<ADMCombatant>(W->GetFirstPlayerController()->GetPawn());
            for(ADMCombatant* A:M->GetCombatants())
            {
                Freeze(A);
                if(!A->bIsEnemy) { A->InitializeCombatant(A->EntityId,false,10000,20);if(A!=Hero) { Ally=A; } }
                else
                {
                    if(A->Smuggler->Role==EDMSmuggler::Gunman) { Gun=A; }
                    if(A->Smuggler->Role==EDMSmuggler::Bruiser) { Bruiser=A; }
                    if(A->Smuggler->Role==EDMSmuggler::Lookout) { Lookout=A; }
                    if(A->Smuggler->Role==EDMSmuggler::Bomber) { Bomber=A; }
                    A->Presentation->UpdatePresentation();
                    Test->TestTrue(TEXT("Faction skin replaces mannequin"),A->GetMesh()->GetSkeletalMeshAsset()->GetPathName().Contains(TEXT("/Enemies/Smugglers/")));
                    Test->TestNotNull(TEXT("Enemy carries authored weapon"),A->Presentation->GetHeldItem()->GetStaticMesh().Get());
                    Test->TestTrue(TEXT("Occupation contains no boss"),A->Smuggler->Role!=EDMSmuggler::GangBoss);
                }
                A->SetActorLocation(FVector(2500,2000,95));
            }
            if(!Gun||!Bruiser||!Lookout||!Bomber||!Ally) { Test->AddError(TEXT("Missing faction role"));GEditor->RequestEndPlayMap();Stage=99;return false; }
            Hero->SetActorLocation(FVector(-1900,0,95));Gun->SetActorLocation(FVector(-1500,0,95));
            Gun->SetAttackTarget(Hero);Gun->Smuggler->Step(*M);Tick=M->GetCombatTick();Stage=2;
        }
        if(Stage==2 && M && M->GetCombatTick()>=Tick+21)
        {
            Test->TestTrue(TEXT("Gunman establishes firing position"),Gun->Smuggler->bSetPosition);
            Test->TestEqual(TEXT("Established position increases damage"),Gun->Smuggler->DamageMultiplier(*M,Hero),1.5f);
            Gun->ApplyDisplacement(FVector(70,0,0));Gun->Smuggler->Step(*M);
            Test->TestFalse(TEXT("Displacement immediately breaks set position"),Gun->Smuggler->bSetPosition);
            Lookout->SetActorLocation(FVector(-1500,200,95));Lookout->Smuggler->NextSignatureTick=0;
            Test->TestTrue(TEXT("Lookout marks an investigator"),Lookout->Smuggler->TrySignature(*M,Hero));
            Test->TestTrue(TEXT("Nearby gunline receives mark"),UDMSmugglerComponent::FocusTarget(*M,Gun,false)==Hero);
            Test->TestEqual(TEXT("Marked target takes extra gunline pressure"),Gun->Smuggler->DamageMultiplier(*M,Hero),1.2f);
            Bomber->SetActorLocation(FVector(-1500,-200,95));Bomber->Smuggler->NextSignatureTick=0;
            Test->TestTrue(TEXT("Bomber starts telegraphed cast"),Bomber->Smuggler->TrySignature(*M,Hero));
            Before=Hero->Health();Ally->SetActorLocation(Hero->GetActorLocation()+FVector(0,80,0));OtherBefore=Ally->Health();
            Hero->SetActorLocation(FVector(-2400,0,95));Tick=M->GetCombatTick();Stage=3;
        }
        if(Stage==3 && M && M->GetCombatTick()>=Tick+13)
        {
            Test->TestEqual(TEXT("Leaving bomb telegraph avoids blast"),Hero->Health(),Before);
            Test->TestTrue(TEXT("Blast damages investigator inside area"),Ally->Health()<OtherBefore);
            OtherBefore=Ally->Health();Tick=M->GetCombatTick();Stage=4;
        }
        if(Stage==4 && M && M->GetCombatTick()>=Tick+11)
        {
            Test->TestTrue(TEXT("Explosive leaves damaging denial area"),Ally->Health()<OtherBefore);
            Bomber->HeldBy=Hero;Bomber->Smuggler->Step(*M);
            Test->TestFalse(TEXT("Control interrupts bomber signature"),Bomber->Smuggler->IsCasting());
            int32 HostileAreas=0;for(TActorIterator<ADMAbilityMarker> It(W);It;++It) { if(It->bHostile && !It->BoundTarget) { ++HostileAreas; } }
            Test->TestEqual(TEXT("Interrupted bomber clears area"),HostileAreas,0);Bomber->HeldBy=nullptr;
            Lookout->Smuggler->Cancel();Hero->SetActorLocation(FVector(-1900,0,95));Bruiser->SetActorLocation(FVector(-1760,0,95));
            Bruiser->Smuggler->NextSignatureTick=0;Before=Hero->Health();Position=Hero->GetActorLocation();
            Test->TestTrue(TEXT("Bruiser winds up bodyguard shove"),Bruiser->Smuggler->TrySignature(*M,Hero));Tick=M->GetCombatTick();Stage=5;
        }
        if(Stage==5 && M && M->GetCombatTick()>=Tick+7)
        {
            Test->TestTrue(TEXT("Shove applies accepted damage"),Hero->Health()<Before);
            Test->TestTrue(TEXT("Shove displaces diver away from gunline"),Hero->GetActorLocation().X<Position.X-50);
            for(ADMCombatant* A:M->GetCombatants()) { if(A->bIsEnemy && A!=Gun) { Hero->DealCombatDamage(A,10000,TEXT("ability.test.clear")); } }
            Tick=M->GetCombatTick();Stage=6;
        }
        if(Stage==6 && M && M->GetCombatTick()>Tick)
        {
            Test->TestTrue(TEXT("Last surviving occupation enemy blocks boss"),M->GetEncounterStage()==EDMSmugglerWave::Occupation);
            Hero->DealCombatDamage(Gun,10000,TEXT("ability.test.clear"));Tick=M->GetCombatTick();Stage=7;
        }
        if(Stage==7 && M && M->GetCombatTick()>Tick)
        {
            Test->TestTrue(TEXT("Cleared occupation waits for boss"),M->GetEncounterStage()==EDMSmugglerWave::Arrival && M->IsCombatActive());
            Test->TestTrue(TEXT("Arrival countdown is publicly projected"),M->GetGameState<ADMGameState>()->EncounterObjective.Contains(TEXT("arrives")));
            Stage=8;
        }
        if(Stage==8 && M && M->GetEncounterStage()==EDMSmugglerWave::Finale)
        {
            Test->TestEqual(TEXT("Boss and four posse members added once"),M->GetCombatants().Num(),20);
            int32 Bosses=0;
            for(ADMCombatant* A:M->GetCombatants())
            {
                if(A->bIsEnemy&&!A->IsDown())
                {
                    Freeze(A);A->SetActorLocation(FVector(-1400,300,95));
                    if(A->Smuggler->Role==EDMSmuggler::GangBoss) { Boss=A;++Bosses; }
                    if(A->Smuggler->Role==EDMSmuggler::Gunman) { Gun=A; }
                }
            }
            Test->TestEqual(TEXT("Exactly one elite boss"),Bosses,1);
            if(!Boss) { Test->AddError(TEXT("Boss did not spawn"));GEditor->RequestEndPlayMap();Stage=99;return false; }
            Test->TestFalse(TEXT("Gang Boss is an elite, not a common clinch target"),Boss->bCommonEnemy);
            Boss->Presentation->UpdatePresentation();Test->TestEqual(TEXT("Boss uses authored model"),Boss->GetMesh()->GetSkeletalMeshAsset()->GetName(),FString(TEXT("SKM_GangBoss")));
            Hero->SetActorLocation(FVector(-1800,0,95));Ally->SetActorLocation(FVector(-1800,300,95));Boss->Smuggler->NextSignatureTick=0;
            Test->TestTrue(TEXT("Boss calls Focus Fire"),Boss->Smuggler->TrySignature(*M,Ally));
            auto* Bot=W->SpawnActor<ADMSquadController>();Bot->Possess(Gun);Gun->Threat.Add(Hero->EntityId,1000000);Bot->Think(*M);
            Test->TestTrue(TEXT("Focus Fire overrides normal threat"),Gun->GetAttackTarget()==Ally);Bot->UnPossess();
            Hero->DealCombatDamage(Boss,10000,TEXT("ability.test.clear"));
            Test->TestNull(TEXT("Killing commander ends Focus Fire"),UDMSmugglerComponent::FocusTarget(*M,Gun,true));Tick=M->GetCombatTick();Stage=9;
        }
        if(Stage==9 && M && M->GetCombatTick()>Tick)
        {
            Test->TestTrue(TEXT("Boss death alone is not victory while posse remains"),M->IsCombatActive());
            for(ADMCombatant* A:M->GetCombatants()) { if(A->bIsEnemy&&!A->IsDown()) { Hero->DealCombatDamage(A,10000,TEXT("ability.test.clear")); } }
            Stage=10;
        }
        if(Stage==10 && M && !M->IsCombatActive())
        {
            Test->TestTrue(TEXT("Whole posse clear completes encounter"),M->GetGameState<ADMGameState>()->GetRunState().Phase==EDMRunPhase::Victory);
            GEditor->RequestEndPlayMap();Stage=99;
        }
        if(Stage==99 && !GEditor->PlayWorld) { Window->RequestDestroyWindow();return true; }
        if(FPlatformTime::Seconds()>Deadline) { Test->AddError(FString::Printf(TEXT("Smuggler test timed out at stage %d"),Stage));GEditor->RequestEndPlayMap();return true; }
        return false;
    }
private:
    FAutomationTestBase* Test;ULevelEditorPlaySettings* Settings=nullptr;TSharedPtr<SWindow> Window;
    int32 Stage=0,Tick=0;double Deadline=0;float Before=0,OtherBefore=0;FVector Position;
    ADMCombatant *Hero=nullptr,*Ally=nullptr,*Gun=nullptr,*Bruiser=nullptr,*Lookout=nullptr,*Bomber=nullptr,*Boss=nullptr;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMSmugglerScenarioTest,"DreadMeridian.Editor.SmugglerFaction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMSmugglerScenarioTest::RunTest(const FString& Parameters) { ADD_LATENT_AUTOMATION_COMMAND(FDMSmugglerScenario(this));return true; }
#endif
