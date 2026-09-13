#include "DMMechanicFixture.h"
#include "DMVision.h"
#include "DMObjective.h"
#include "DMSquadController.h"
#include "EngineUtils.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMVisionTest,"DreadMeridian.Editor.Vision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDMVisionTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(FDMMechanicFixture(this,[this](UWorld* W,ADMCombatGameMode* M,ADMCombatant* Hero)
    {
        auto* Lurker=M->SpawnEncounterActor(TEXT("test.vision_lurker"),Hero->GetActorLocation()+FVector(400,0,0),500,5); Lurker->Swamp->Initialize(EDMSwampThing::Lurker);
        auto* Reeds=W->SpawnActor<ADMVisionArea>(Lurker->GetActorLocation(),FRotator::ZeroRotator); Reeds->Radius=250;
        TestFalse(TEXT("Reeds conceal waiting Lurker"),DMVision::CanSee(Hero,Lurker));
        const FVector Position=Lurker->GetActorLocation(); Hero->SetActorLocation(Position-FVector(700,0,0)); Lurker->Swamp->Step(M->GetCombatTick()+1);
        TestFalse(TEXT("Stalking does not prematurely commit"),Lurker->Swamp->Capture().ResolveAt>0);
        TestTrue(TEXT("Waiting ambusher closes toward a valid commitment range"),Lurker->HasMoveGoal());
        Hero->SetActorLocation(Position-FVector(400,0,0));
        TestFalse(TEXT("Concealed actor is not connection relevant"),Lurker->IsNetRelevantFor(Hero,Hero,Hero->GetActorLocation()));
        TestFalse(TEXT("Basic attacks reject hidden target"),Hero->TryAttack(Lurker));
        TestFalse(TEXT("Direct basic damage cannot bypass sight"),Hero->DealCombatDamage(Lurker,5,TEXT("test.hidden_basic"),true));
        TestTrue(TEXT("Targeted Q rejects hidden actor"),!Hero->Primary->Validate(Lurker,Lurker->GetActorLocation(),false).IsEmpty());
        auto* Brain=W->SpawnActor<ADMSquadController>(); Brain->Possess(Hero); Brain->Think(*M);
        const int32 HiddenIndex=M->GetCombatants().IndexOfByKey(Lurker); const auto& HiddenView=Brain->GetLastContext().Actors[HiddenIndex];
        TestTrue(TEXT("AI cannot read hidden identity or location"),HiddenView.EntityId.IsEmpty() && HiddenView.Location.IsZero() && !HiddenView.bVisible);
        TestTrue(TEXT("Hidden enemy cannot become a phantom revive target"),HiddenView.bEnemy && HiddenView.bDown); Brain->UnPossess();
        auto* Lookout=M->SpawnEncounterActor(TEXT("test.vision_lookout"),Hero->GetActorLocation(),500,1); Lookout->bHumanEnemy=true; Lookout->Smuggler->Initialize(EDMSmuggler::Lookout);
        const TCHAR* Reason=nullptr; TestFalse(TEXT("Enemy signatures also respect concealment"),Lookout->Smuggler->CanSignature(*M,Lurker,&Reason)); TestEqual(TEXT("Concealment rejects before casting"),FString(Reason),FString(TEXT("target_hidden")));
        ADMCombatant* Ally=nullptr; for (ADMCombatant* A : M->GetCombatants()) { if (!A->bIsEnemy && A!=Hero) { Ally=A; break; } }
        Ally->SetActorLocation(Lurker->GetActorLocation()+FVector(0,100,0)); TestTrue(TEXT("Nearby ally shares detection"),DMVision::CanSee(Hero,Lurker));
        Ally->SetActorLocation(FVector(4000,4000,95)); TestFalse(TEXT("Losing shared detection restores concealment"),DMVision::CanSee(Hero,Lurker));
        TestTrue(TEXT("Lurker commits from concealment"),Lurker->Swamp->TrySignature(Hero)); TestTrue(TEXT("Commit reveals actor"),DMVision::CanSee(Hero,Lurker));
        TestTrue(TEXT("Enemy signature can target revealed Lurker"),Lookout->Smuggler->CanSignature(*M,Lurker));
        TestTrue(TEXT("Committed actor becomes relevant"),Lurker->IsNetRelevantFor(Hero,Hero,Hero->GetActorLocation()));
        auto Idle=Lurker->Swamp->Capture(); Idle.ResolveAt=0; Idle.ReadyAt=0; Idle.TargetId.Reset(); Lurker->Swamp->Restore(Idle);
        TestFalse(TEXT("Idle state can conceal again"),DMVision::CanSee(Hero,Lurker));
        TestTrue(TEXT("Blind area damage remains legal"),Hero->DealCombatDamage(Lurker,5,TEXT("test.blind_area")));
        TestTrue(TEXT("Accepted damage reveals"),DMVision::CanSee(Hero,Lurker)); Lurker->VisionRevealUntil=0;
        Reeds->bEnabled=false; TestTrue(TEXT("Disabled reeds restore ordinary sight"),DMVision::CanSee(Hero,Lurker)); Reeds->bEnabled=true;
        auto* Lighthouse=W->SpawnActor<ADMObjective>(); FDMObjectiveStep Step; Step.Location=Hero->GetActorLocation(); Step.WorkTicks=1;
        Lighthouse->Configure(TEXT("test.vision_lighthouse"),TEXT("Lighthouse"),{Step},EDMObjectiveReward::Vision);
        const auto Initial=Lighthouse->Capture();
        TestTrue(TEXT("Operate lighthouse"),Lighthouse->Interact(Hero)); Lighthouse->Step(M->GetCombatTick()+1);
        TestTrue(TEXT("Completed lighthouse reveals reeds"),Lighthouse->bVisionOnline && DMVision::CanSee(Hero,Lurker));
        auto Completed=Lighthouse->Capture(); Lighthouse->Restore(Completed); int32 Sources=0;
        for (TActorIterator<ADMVisionArea> It(W);It;++It) { if (!It->bReeds) { ++Sources; It->bEnabled=false; } }
        TestEqual(TEXT("Restoration does not duplicate source"),Sources,1); TestFalse(TEXT("Source disabling removes vision"),DMVision::CanSee(Hero,Lurker));
        Lighthouse->Restore(Completed); TestTrue(TEXT("Completed objective restores its source"),DMVision::CanSee(Hero,Lurker));
        Lighthouse->Restore(Initial); TestFalse(TEXT("Restoring unfinished objective removes reward vision"),DMVision::CanSee(Hero,Lurker));
    })); return true;
}
#endif
