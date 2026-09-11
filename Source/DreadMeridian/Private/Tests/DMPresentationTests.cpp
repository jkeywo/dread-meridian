#include "DMCombatPresentation.h"
#include "DMCombatant.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMPresentationTest, "DreadMeridian.Foundation.CombatPresentation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMPresentationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    const TCHAR* Names[] = { TEXT("Sapper"), TEXT("Photographer"), TEXT("Medium"), TEXT("Smuggler") };
    for (int32 I = 0; I < 4; ++I)
    {
        auto* A = World->SpawnActor<ADMCombatant>();
        A->InitializeCombatant(TEXT("investigator.test"), false, 100, 18);
        A->InitializeInvestigator(static_cast<EDMInvestigator>(I + 1), false);
        A->Presentation->UpdatePresentation();
        TestEqual(TEXT("Native investigator skin selected"), A->GetMesh()->GetSkeletalMeshAsset()->GetName(), FString(TEXT("SKM_")) + Names[I]);
        TestTrue(TEXT("Imported rig has attachment sockets"), A->GetMesh()->DoesSocketExist(TEXT("SOCKET_hand_r")));
        auto* Item = A->Presentation->GetHeldItem();
        TestNotNull(TEXT("Held-item component created"), Item);
        if (I < 2) { TestNotNull(TEXT("Rifle equipped"), Item->GetStaticMesh().Get()); TestTrue(TEXT("Gunfire has authored muzzle socket"), Item->DoesSocketExist(TEXT("Muzzle"))); TestEqual(TEXT("Weapon attached to right hand"), Item->GetAttachSocketName(), FName(TEXT("SOCKET_hand_r"))); }
        if (I == 1)
        {
            auto* Gear = A->Presentation->GetStowedItem();
            TestEqual(TEXT("Camera holstered on chest"), Gear->GetAttachSocketName(), FName(TEXT("SOCKET_stow_chest")));
            A->Primary->FrameTarget = A; A->Presentation->UpdatePresentation();
            TestTrue(TEXT("Frame equips camera"), Item->GetStaticMesh()->GetName().Contains(TEXT("Camera_Held")));
            TestEqual(TEXT("Frame stows rifle on back"), Gear->GetAttachSocketName(), FName(TEXT("SOCKET_stow_back")));
            A->Primary->FrameTarget = nullptr; A->Presentation->UpdatePresentation();
        }
        if (I == 3) { TestNull(TEXT("Smuggler remains bare-knuckled"), Item->GetStaticMesh().Get()); }
        A->GetCharacterMovement()->Velocity = FVector(420, 0, 0); A->Presentation->UpdatePresentation();
        TestTrue(TEXT("Movement starts with imported clip"), A->Presentation->CurrentClip().Contains(TEXT("Run_Start")));
        TestEqual(TEXT("Rifle variants selected for gun users"), A->Presentation->CurrentClip().EndsWith(TEXT("_Rifle")), I < 2);
        auto* MotionClip = Cast<UAnimSequence>(A->GetMesh()->GetSingleNodeInstance()->GetCurrentAsset());
        TestTrue(TEXT("Locomotion cannot drive gameplay root motion"), MotionClip && !MotionClip->bEnableRootMotion && MotionClip->bForceRootLock);
        A->GetCharacterMovement()->Velocity = FVector::ZeroVector; A->Presentation->UpdatePresentation();
        TestTrue(TEXT("Stopping interrupts start"), A->Presentation->CurrentClip().Contains(TEXT("Run_Stop")));
        A->SetActorRotation(FRotator(0,90,0)); A->Presentation->UpdatePresentation();
        TestTrue(TEXT("Heading change plays right turn"), A->Presentation->CurrentClip().Contains(TEXT("Turn_090_R")));
        TestEqual(TEXT("Cosmetic turn preserves actor heading"), A->GetActorRotation().Yaw, 90.0);
        const FVector Location = A->GetActorLocation(); const int32 Charges = A->Investigator->Charges;
        A->Presentation->Cue(0, Location + FVector(100, 0, 0));
        TestTrue(TEXT("Basic attack plays retargeted clip"), A->Presentation->CurrentClip().StartsWith(TEXT("A_DM_")));
        TestEqual(TEXT("Presentation does not apply damage"), A->Health(), 100.f);
        TestEqual(TEXT("Presentation does not spend resource"), A->Investigator->Charges, Charges);
        TestTrue(TEXT("Presentation does not move gameplay actor"), A->GetActorLocation().Equals(Location));
        A->InitializeCombatant(TEXT("investigator.test"), false, 0, 18); A->Presentation->UpdatePresentation();
        TestEqual(TEXT("Down state plays collapse"), A->Presentation->CurrentClip(), FString(TEXT("A_DM_Death_1")));
        A->InitializeCombatant(TEXT("investigator.test"), false, 100, 18); A->Presentation->UpdatePresentation();
        TestEqual(TEXT("Revive plays get-up"), A->Presentation->CurrentClip(), FString(TEXT("A_DM_Prone_To_Stand")));
    }
    const TCHAR* EnemyNames[] = {TEXT("Gunman"),TEXT("Bruiser"),TEXT("Lookout"),TEXT("Bomber"),TEXT("GangBoss")};
    const TCHAR* Attacks[] = {TEXT("A_DM_Fire_Rifle_Hip"),TEXT("A_DM_KB_p_Hook_R"),TEXT("A_DM_KB_Gun"),TEXT("A_DM_KB_Grenade"),TEXT("A_DM_Fire_Rifle_Hip")};
    for (int32 I=0; I<5; ++I)
    {
        auto* A=World->SpawnActor<ADMCombatant>();
        A->Smuggler->Initialize(static_cast<EDMSmuggler>(I+1));
        A->InitializeCombatant(TEXT("enemy.presentation"),true,100,10);
        A->Presentation->UpdatePresentation();
        TestEqual(TEXT("Smuggler uses supplied model"),A->GetMesh()->GetSkeletalMeshAsset()->GetName(),FString(TEXT("SKM_"))+EnemyNames[I]);
        TestEqual(TEXT("Idle preserves equipped pose"),A->Presentation->CurrentClip(),FString(TEXT("A_DM_"))+EnemyNames[I]+TEXT("_Idle"));
        TestNotNull(TEXT("Enemy weapon held"),A->Presentation->GetHeldItem()->GetStaticMesh().Get());
        TestTrue(TEXT("Enemy weapon has authored grip"),A->Presentation->GetHeldItem()->DoesSocketExist(TEXT("ANCHOR_grip_r")));
        if (I==2 || I==3) { TestNotNull(TEXT("Extra gear holstered"),A->Presentation->GetStowedItem()->GetStaticMesh().Get()); }
        const FVector Before=A->GetActorLocation();
        A->Presentation->Cue(0,Before+FVector(200,0,0));
        TestEqual(TEXT("Role-specific basic attack"),A->Presentation->CurrentClip(),FString(Attacks[I]));
        for (uint8 Event : {uint8(2),uint8(5),uint8(6),uint8(7),uint8(8)}) { A->Presentation->Cue(Event,Before+FVector(100,0,0)); }
        TestEqual(TEXT("Enemy effects do not apply damage"),A->Health(),100.f);
        TestTrue(TEXT("Enemy effects do not displace actor"),A->GetActorLocation().Equals(Before));
        A->InitializeCombatant(TEXT("enemy.presentation"),true,0,10);A->Presentation->UpdatePresentation();
        TestEqual(TEXT("All enemy models play death"),A->Presentation->CurrentClip(),FString(TEXT("A_DM_Death_1")));
    }
    // Named-kit clips. A missing retarget leaves a null entry in the clip list and every cue that uses it silently
    // does nothing, so the assets are checked directly rather than through a cue that does not exist yet.
    const TCHAR* KitClips[] = { TEXT("KB_Projectile_1"), TEXT("KB_Projectile_Up"), TEXT("KB_Block_Start"), TEXT("KB_Block_End"),
        TEXT("KB_Superpunch"), TEXT("KB_SkipFwd_1"), TEXT("KB_m_Backelbow_R"), TEXT("KB_GroundAttack"),
        TEXT("Anim_IN_check_CO"), TEXT("MG_shoot"), TEXT("Anim_EM_call_out") };
    for (const TCHAR* Name : KitClips)
    {
        const FString Path = FString::Printf(TEXT("/Game/DreadMeridian/Presentation/Animations/A_DM_%s"), Name);
        auto* Clip = LoadObject<UAnimSequence>(nullptr, *Path);
        if (!TestNotNull(*FString::Printf(TEXT("Kit clip %s retargeted"), Name), Clip)) { continue; }
        TestTrue(TEXT("Kit clip has duration"), Clip->GetPlayLength() > 0);
        TestTrue(TEXT("Kit clip cannot drive gameplay root motion"), !Clip->bEnableRootMotion && Clip->bForceRootLock);
    }
    World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMLocomotionTest, "DreadMeridian.Foundation.LocomotionTransitions",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FDMLocomotionTest::RunTest(const FString& Parameters)
{
    FDMLocomotionPresentation M;
    TestTrue(TEXT("Spawn does not invent a transition"), M.Update(0,0,0) == EDMLocomotion::Idle);
    TestTrue(TEXT("Tiny velocity noise stays idle"), M.Update(12,0,.01f) == EDMLocomotion::Idle);
    TestTrue(TEXT("Acceleration starts"), M.Update(100,0,.02f) == EDMLocomotion::Start);
    TestTrue(TEXT("Hysteresis avoids flicker"), M.Update(20,0,.03f) == EDMLocomotion::Start);
    TestTrue(TEXT("Start settles to run"), M.Update(420,0,.38f) == EDMLocomotion::Run);
    TestTrue(TEXT("Deceleration stops"), M.Update(0,0,.4f) == EDMLocomotion::Stop);
    TestTrue(TEXT("Restart interrupts stop immediately"), M.Update(420,0,.41f) == EDMLocomotion::Start);
    TestTrue(TEXT("Left turn direction"), M.Update(420,-90,.5f) == EDMLocomotion::TurnLeft);
    TestTrue(TEXT("Turn initially preserves previous visual heading"), FMath::IsNearlyEqual(M.VisualYaw(.5f),90.f));
    TestTrue(TEXT("Turn offset settles"), FMath::IsNearlyZero(M.VisualYaw(.97f)));
    TestTrue(TEXT("Turn returns to gait"), M.Update(420,-90,.97f) == EDMLocomotion::Run);
    M.Reset(0,179,1);
    TestTrue(TEXT("Yaw wrap does not spuriously turn"), M.Update(0,-179,1.01f) == EDMLocomotion::Idle);
    M.Reset(0,0,2);
    TestTrue(TEXT("Right turn direction"), M.Update(0,90,2.01f) == EDMLocomotion::TurnRight);
    M.Reset(420,90,3);
    TestTrue(TEXT("Combat override reset cannot replay stale transition"), M.Update(420,90,3.1f) == EDMLocomotion::Run);
    return true;
}
#endif
