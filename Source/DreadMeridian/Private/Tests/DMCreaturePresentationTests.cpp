#include "DMCombatPresentation.h"
#include "DMCombatant.h"
#include "DMShubMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMCreaturePresentationTest,"DreadMeridian.Foundation.CreaturePresentation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMCreaturePresentationTest::RunTest(const FString& Parameters)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    const TCHAR* Names[]={TEXT("Crawler"),TEXT("Lurker"),TEXT("Spitter"),TEXT("Grasper"),TEXT("OldThing"),TEXT("Broodling"),TEXT("BlackGoat")};
    for (int32 I=0; I<7; ++I)
    {
        auto* A=World->SpawnActor<ADMCombatant>();
        A->InitializeCombatant(TEXT("creature.presentation"),true,100,10);
        // Begin as a human enemy to exercise replacement and equipment cleanup.
        A->Smuggler->Initialize(EDMSmuggler::Gunman); A->Presentation->UpdatePresentation();
        if (I<5) { TestTrue(TEXT("Swamp role initialized"),A->Swamp->Initialize(static_cast<EDMSwampThing>(I+1))); }
        else
        {
            auto* Shub=NewObject<UDMShubMinion>(A); A->AddInstanceComponent(Shub); Shub->RegisterComponent();
            Shub->Initialize(I==5 ? EDMShubMinionKind::Broodling : EDMShubMinionKind::Goat);
        }
        A->Presentation->UpdatePresentation();
        const bool bStatic=I==0 || I==2 || I==5;
        TestNull(TEXT("Creature has no leftover human weapon"),A->Presentation->GetHeldItem()->GetStaticMesh().Get());
        TestNull(TEXT("Creature has no leftover holstered gear"),A->Presentation->GetStowedItem()->GetStaticMesh().Get());
        if (bStatic)
        {
            auto* Body=A->Presentation->GetCreatureBody();
            TestNotNull(TEXT("Static creature component exists"),Body);
            if (Body && Body->GetStaticMesh())
            {
                TestEqual(TEXT("Correct resting creature mesh"),Body->GetStaticMesh()->GetName(),FString(TEXT("SM_"))+Names[I]);
                TestTrue(TEXT("Resting creature visible"),Body->IsVisible());
                TestEqual(TEXT("Cosmetic body has no collision"),Body->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
            }
            else { AddError(TEXT("Missing static creature mesh")); }
            TestFalse(TEXT("Human placeholder hidden"),A->GetMesh()->IsVisible());
        }
        else
        {
            TestTrue(TEXT("Humanoid skin visible"),A->GetMesh()->IsVisible());
            TestEqual(TEXT("Correct humanoid creature skin"),A->GetMesh()->GetSkeletalMeshAsset()->GetName(),FString(TEXT("SKM_"))+Names[I]);
            TestEqual(TEXT("Humanoid idles"),A->Presentation->CurrentClip(),FString(TEXT("MM_Idle")));
            A->GetCharacterMovement()->Velocity=FVector(300,0,0); A->Presentation->UpdatePresentation();
            TestEqual(TEXT("Humanoid locomotion active"),A->Presentation->CurrentClip(),FString(TEXT("MF_Unarmed_Jog_Fwd")));
            A->Presentation->Cue(0,FVector(200,0,0));
            TestTrue(TEXT("Creature melee animation active"),A->Presentation->CurrentClip().Contains(TEXT("Jab")));
        }
        TestEqual(TEXT("Presentation preserves actor scale"),A->GetActorScale3D(),FVector::OneVector);
        TestEqual(TEXT("Presentation does not damage actor"),A->Health(),100.f);
        A->InitializeCombatant(TEXT("creature.presentation"),true,0,10); A->Presentation->UpdatePresentation();
        if (!bStatic) { TestEqual(TEXT("Creature plays death"),A->Presentation->CurrentClip(),FString(TEXT("A_DM_Death_1"))); }
    }
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
#endif
