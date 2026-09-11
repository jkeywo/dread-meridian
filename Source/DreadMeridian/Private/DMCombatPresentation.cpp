#include "DMCombatPresentation.h"
#include "DMCombatant.h"
#include "DMAttackFX.h"
#include "Animation/AnimSequence.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// Index into Clips: the two Paths entries first, then AnimNames in order. Append only, and keep both lists in step.
namespace { enum EClip { Idle, Jog, RifleIdle, RifleAim, RifleJog, RifleFire, RifleAimFire, Death, Hit, GetUp, JabR, JabL, FightIdle, Grenade, Hold, CastGesture, Place, Start, Stop, TurnL, TurnR, RifleStart, RifleStop, RifleTurnL, RifleTurnR,
    Cast1, CastUp, BlockStart, BlockEnd, Superpunch, SkipFwd, Backelbow, GroundSlam, CameraCheck, MGShoot, CallOut, ClipCount }; }
UDMCombatPresentation::UDMCombatPresentation()
{
    const TCHAR* Names[] = { TEXT("Sapper"), TEXT("Photographer"), TEXT("Medium"), TEXT("Smuggler") };
    for (const TCHAR* Name : Names)
    {
        ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(*FString::Printf(TEXT("/Game/DreadMeridian/Characters/Investigators/%s/SKM_%s"), Name, Name));
        Skins.Add(Mesh.Object);
    }
    const TCHAR* EnemyNames[] = {TEXT("Gunman"),TEXT("Bruiser"),TEXT("Lookout"),TEXT("Bomber"),TEXT("GangBoss")};
    for (const TCHAR* Name : EnemyNames)
    { ConstructorHelpers::FObjectFinder<USkeletalMesh> M(*FString::Printf(TEXT("/Game/DreadMeridian/Characters/Enemies/Smugglers/%s/SKM_%s"),Name,Name)); EnemySkins.Add(M.Object); }
    const TCHAR* Motions[] = {TEXT("Idle"),TEXT("Jog"),TEXT("Start"),TEXT("Stop"),TEXT("TurnL"),TEXT("TurnR")};
    for (const TCHAR* Name : EnemyNames)
    { for (const TCHAR* Motion : Motions)
        { ConstructorHelpers::FObjectFinder<UAnimSequence> A(*FString::Printf(TEXT("/Game/DreadMeridian/Presentation/Animations/Smugglers/A_DM_%s_%s"),Name,Motion)); EnemyLocomotion.Add(A.Object); } }
    for (const TCHAR* Name : {TEXT("KB_Gun"),TEXT("KB_p_Hook_R"),TEXT("MG_gestures1")})
    { ConstructorHelpers::FObjectFinder<UAnimSequence> A(*FString::Printf(TEXT("/Game/DreadMeridian/Presentation/Animations/Smugglers/A_DM_%s"),Name)); EnemyActions.Add(A.Object); }
    const TCHAR* Equipment[] = {TEXT("GunmanRifle"),TEXT("BruiserTruncheon"),TEXT("LookoutPistol"),TEXT("BomberGrenade"),TEXT("GangBossSmg"),TEXT("LookoutBinoculars"),TEXT("BomberSatchel")};
    for (const TCHAR* Name : Equipment)
    { ConstructorHelpers::FObjectFinder<UStaticMesh> M(*FString::Printf(TEXT("/Game/DreadMeridian/Characters/Enemies/Smugglers/Equipment/SM_%s"),Name)); EnemyItems.Add(M.Object); }
    const TCHAR* Paths[] = { TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"), TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd") };
    for (const TCHAR* Path : Paths) { ConstructorHelpers::FObjectFinder<UAnimSequence> Clip(Path); Clips.Add(Clip.Object); }
    const TCHAR* AnimNames[] = { TEXT("Idle_Rifle_Hip"), TEXT("Idle_Rifle_Ironsights"), TEXT("Jog_Fwd_Rifle"), TEXT("Fire_Rifle_Hip"), TEXT("Fire_Rifle_Ironsights"), TEXT("Death_1"), TEXT("Hit_React_1"), TEXT("Prone_To_Stand"), TEXT("KB_p_Jab_R_1"), TEXT("KB_p_Jab_L_1"), TEXT("KB_Idle_1"), TEXT("KB_Grenade"), TEXT("KB_Block_Loop"), TEXT("KB_KnifeThrow"), TEXT("Anim_IN_SA_pick_up_ground"), TEXT("Locomotion_M_Neutral_Run_Start_F_Rfoot"), TEXT("Locomotion_M_Neutral_Run_Stop_F_Rfoot"), TEXT("Locomotion_M_Neutral_Stand_Turn_090_L"), TEXT("Locomotion_M_Neutral_Stand_Turn_090_R"), TEXT("Locomotion_M_Neutral_Run_Start_F_Rfoot_Rifle"), TEXT("Locomotion_M_Neutral_Run_Stop_F_Rfoot_Rifle"), TEXT("Locomotion_M_Neutral_Stand_Turn_090_L_Rifle"), TEXT("Locomotion_M_Neutral_Stand_Turn_090_R_Rifle"),
        // Named-kit clips, in EClip order from Cast1.
        TEXT("KB_Projectile_1"), TEXT("KB_Projectile_Up"), TEXT("KB_Block_Start"), TEXT("KB_Block_End"), TEXT("KB_Superpunch"),
        TEXT("KB_SkipFwd_1"), TEXT("KB_m_Backelbow_R"), TEXT("KB_GroundAttack"), TEXT("Anim_IN_check_CO"), TEXT("MG_shoot"), TEXT("Anim_EM_call_out") };
    for (const TCHAR* Name : AnimNames) { ConstructorHelpers::FObjectFinder<UAnimSequence> Clip(*FString::Printf(TEXT("/Game/DreadMeridian/Presentation/Animations/A_DM_%s"), Name)); Clips.Add(Clip.Object); }
    static_assert(UE_ARRAY_COUNT(AnimNames) + UE_ARRAY_COUNT(Paths) == ClipCount, "EClip out of sync with the clip lists");
    const TCHAR* PropNames[] = { TEXT("SapperCarbine_Held"), TEXT("PhotographerRifle_Held"), TEXT("PhotographerCamera_Stowed"), TEXT("PhotographerCamera_Held"), TEXT("PhotographerRifle_Stowed"), TEXT("MediumWisp_Held") };
    for (const TCHAR* Name : PropNames) { ConstructorHelpers::FObjectFinder<UStaticMesh> Item(*FString::Printf(TEXT("/Game/DreadMeridian/Presentation/Props/SM_%s"), Name)); Items.Add(Item.Object); }
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Blast(TEXT("/Game/Explosions_W3Vol1/Niagara/NS_ImpactExplosion")); Explosion = Blast.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Flash(TEXT("/Game/Explosions_W3Vol1/Niagara/NS_Burst2")); MuzzleFlash = Flash.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Snap(TEXT("/Game/Explosions_W3Vol1/Niagara/NS_highimpact")); WireSnap = Snap.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Wave(TEXT("/Game/BigNiagaraBundle/NiagaraEffectMix2/Effects/NS_ShockWave")); Shock = Wave.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Aura(TEXT("/Game/BigNiagaraBundle/NiagaraEffectsMix/Effects/NS_EnergyLife")); DeferredAura = Aura.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Mark(TEXT("/Game/NiagaraExamples/FX_Markers/NS_Marker_Target")); TargetMark = Mark.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Shimmer(TEXT("/Game/Explosions_W3Vol1/Niagara/NS_FlareShimmer1")); Flashbulb = Shimmer.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Shine(TEXT("/Game/BigNiagaraBundle/NiagaraBlackAndWhite/NiagaraSystems/White/NS_ShiningWhite")); Developed = Shine.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Photons(TEXT("/Game/BigNiagaraBundle/NiagaraAbstractSpace3/Effects/NS_The_Birth_Of_Photons")); PhotographAura = Photons.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Spirits(TEXT("/Game/BigNiagaraBundle/NiagaraEffectMix2/Effects/NS_Spirits_of_Colors")); SpiritArrival = Spirits.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Reverse(TEXT("/Game/BigNiagaraBundle/NiagaraAbstractSpace2/Effects/NS_ReverseBurst")); Intervention = Reverse.Object;
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> Ring(TEXT("/Game/BigNiagaraBundle/NiagaraAbstractSpace2/Effects/NS_RingOfOmnipotence")); SeanceCircle = Ring.Object;
}
void UDMCombatPresentation::Spawn(UNiagaraSystem* System, const FVector& At, float Scale, float Seconds)
{
    if (!System) { return; }
    auto* FX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System, At, FRotator::ZeroRotator, FVector(Scale));
    if (!FX) { return; }
    FX->SetAutoDestroy(true);
    // A bounded lifetime: several of these systems loop, and a cosmetic effect must never outlive its cue.
    FTimerHandle Timer;
    GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(FX, [FX] { FX->DestroyComponent(); }), Seconds, false);
}
void UDMCombatPresentation::Attach(UNiagaraSystem* System, float Seconds)
{
    auto* Actor = CastChecked<ADMCombatant>(GetOwner());
    if (!System) { return; }
    auto* FX = UNiagaraFunctionLibrary::SpawnSystemAttached(System, Actor->GetRootComponent(), NAME_None, FVector(0, 0, -80),
        FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
    if (!FX) { return; }
    FTimerHandle Timer;
    GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(FX, [FX] { FX->DestroyComponent(); }), Seconds, false);
}
void UDMCombatPresentation::Play(UAnimSequence* Clip, bool bLoop, float Rate, bool bRestart)
{
    auto* Actor = CastChecked<ADMCombatant>(GetOwner());
    if (!Clip) { return; }
    if (Playing != Clip || bRestart) { Actor->GetMesh()->PlayAnimation(Clip, bLoop); Playing = Clip; }
    Actor->GetMesh()->SetPlayRate(Rate);
}
void UDMCombatPresentation::Action(UAnimSequence* Clip, float Duration)
{
    if (!Clip) { return; }
    Locomotion.Reset(GetOwner()->GetVelocity().Size2D(), GetOwner()->GetActorRotation().Yaw, GetWorld()->GetTimeSeconds());
    ActionClip = Clip; ActionUntil = GetWorld()->GetTimeSeconds() + Duration;
    Play(Clip, false, Clip->GetPlayLength() / FMath::Max(.1f, Duration), true);
}
void UDMCombatPresentation::Equip(bool bCamera)
{
    auto* Actor = CastChecked<ADMCombatant>(GetOwner());
    if (!HeldItem)
    {
        HeldItem = NewObject<UStaticMeshComponent>(Actor, TEXT("HeldWeapon"));
        StowedItem = NewObject<UStaticMeshComponent>(Actor, TEXT("HolsteredGear"));
        for (auto* Item : { HeldItem.Get(), StowedItem.Get() })
        { Actor->AddInstanceComponent(Item); Item->SetCollisionEnabled(ECollisionEnabled::NoCollision); Item->RegisterComponent(); }
    }
    HeldItem->SetStaticMesh(nullptr); StowedItem->SetStaticMesh(nullptr);
    HeldItem->AttachToComponent(Actor->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("SOCKET_hand_r"));
    if (Kind == 1) { HeldItem->SetStaticMesh(Items[0]); }
    if (Kind == 2)
    {
        HeldItem->SetStaticMesh(Items[bCamera ? 3 : 1]); StowedItem->SetStaticMesh(Items[bCamera ? 4 : 2]);
        StowedItem->AttachToComponent(Actor->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, bCamera ? TEXT("SOCKET_stow_back") : TEXT("SOCKET_stow_chest"));
    }
    if (Kind == 3) { HeldItem->SetStaticMesh(Items[5]); HeldItem->AttachToComponent(Actor->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("SOCKET_fx_palm_r")); }
    if (Kind >= 11 && Kind <= 15)
    {
        HeldItem->SetStaticMesh(EnemyItems[Kind-11]);
        if (Kind == 13 || Kind == 14)
        {
            StowedItem->SetStaticMesh(EnemyItems[Kind == 13 ? 5 : 6]);
            StowedItem->AttachToComponent(Actor->GetMesh(),FAttachmentTransformRules::SnapToTargetIncludingScale,
                Kind == 13 ? TEXT("SOCKET_stow_chest") : TEXT("SOCKET_holster_hip_r"));
            if (StowedItem->DoesSocketExist(TEXT("ANCHOR_stow")))
            { StowedItem->SetRelativeTransform(StowedItem->GetSocketTransform(TEXT("ANCHOR_stow"),RTS_Component).Inverse()); }
        }
        UpdateEnemyGrip();
    }
    bCameraHeld = bCamera;
}
void UDMCombatPresentation::UpdatePresentation()
{
    if (GetNetMode() == NM_DedicatedServer) { return; }
    auto* Actor = CastChecked<ADMCombatant>(GetOwner());
    const uint8 NewKind = Actor->bIsEnemy ? (Actor->Smuggler->Role == EDMSmuggler::None ? 0 : 10 + static_cast<uint8>(Actor->Smuggler->Role)) : static_cast<uint8>(Actor->Investigator->Kind);
    if (Kind != NewKind)
    {
        Kind = NewKind;
        // Scale from the feet without changing gameplay collision or movement.
        const float VisualScale = (Kind == 4 || Kind == 12) ? 1.2f :
            (Kind == 1 || Kind == 11 || Kind == 15) ? 1.1f : 1.f;
        Actor->GetMesh()->SetRelativeScale3D(FVector(VisualScale));
        if (Kind >= 1 && Kind <= 4 && Skins[Kind - 1]) { Actor->GetMesh()->SetSkeletalMeshAsset(Skins[Kind - 1]); }
        if (Kind >= 11 && Kind <= 15 && EnemySkins[Kind-11]) { Actor->GetMesh()->SetSkeletalMeshAsset(EnemySkins[Kind-11]); }
        Playing = nullptr; Equip(false);
    }
    if ((Kind == 1 || Kind == 2) && HeldItem && HeldItem->DoesSocketExist(TEXT("Support")))
    {
        // Keep the authored grip in the right hand and orient its support axis toward the left hand.
        const FTransform Hand = Actor->GetMesh()->GetSocketTransform(TEXT("SOCKET_hand_r"));
        HeldItem->SetRelativeRotation(FRotator::ZeroRotator);
        const bool bAimedShot = GetWorld()->GetTimeSeconds() < AimUntil && HeldItem->DoesSocketExist(TEXT("Muzzle"));
        const FVector SourceAxis = HeldItem->GetSocketLocation(bAimedShot ? TEXT("Muzzle") : TEXT("Support")) - Hand.GetLocation();
        const FVector TargetAxis = (bAimedShot ? AimPoint : Actor->GetMesh()->GetSocketLocation(TEXT("hand_l"))) - Hand.GetLocation();
        if (!SourceAxis.IsNearlyZero() && !TargetAxis.IsNearlyZero())
        { HeldItem->SetWorldRotation(FQuat::FindBetweenNormals(SourceAxis.GetSafeNormal(), TargetAxis.GetSafeNormal()) * Hand.GetRotation()); }
    }
    UpdateEnemyGrip();
    const float Now = GetWorld()->GetTimeSeconds();
    const float Speed = Actor->GetVelocity().Size2D();
    const float Yaw = Actor->GetActorRotation().Yaw;
    const bool bDown = Actor->IsDown();
    if (bDown || bWasDown || (ActionClip && Now < ActionUntil) || Actor->Primary->FrameTarget ||
        Actor->Primary->HeldTarget || Actor->IsRestrained() || Actor->ReviveProgress > 0)
    { Locomotion.Reset(Speed, Yaw, Now); }
    if (bDown)
    {
        if (!bWasDown) { ActionClip = nullptr; Play(Clips[Death], false, 1, true); }
        bWasDown = true; if (Kind == 3) { HeldItem->SetVisibility(false); } return;
    }
    if (bWasDown) { bWasDown = false; Action(Clips[GetUp], 1.f); }
    // The camera is raised while framing and for a moment after a Flashbulb or Impossible Photograph. Those are
    // brief poses rather than gestures: the migrated camera clip runs 21 seconds, and Action() time-scales, so
    // playing it over half a second would run it at 35x.
    const bool bFrame = Actor->Primary->FrameTarget != nullptr;
    const bool bCamera = bFrame || Now < CameraUntil;
    if (Kind == 2 && bCameraHeld != bCamera) { Equip(bCamera); }
    if (Kind == 3) { HeldItem->SetVisibility(Now < ActionUntil || !Actor->Primary->Bindings.IsEmpty()); }
    if (ActionClip && Now < ActionUntil) { return; }
    ActionClip = nullptr;
    if (bCamera) { Play(Clips[RifleAim], true); return; }
    if (Actor->Primary->HeldTarget || Actor->IsRestrained()) { Play(Clips[Hold], true); return; }
    if (Actor->ReviveProgress > 0) { Play(Clips[Place], true); return; }
    const EDMLocomotion Previous = Locomotion.Phase;
    const EDMLocomotion Motion = Locomotion.Update(Speed, Yaw, Now);
    const bool bRifle = UsesGunPose();
    int32 Index = bRifle ? RifleIdle : (Kind == 4 ? FightIdle : Idle);
    float Duration = 0;
    switch (Motion)
    {
    case EDMLocomotion::Run: Index = bRifle ? RifleJog : Jog; break;
    case EDMLocomotion::Start: Index = bRifle ? RifleStart : Start; Duration = FDMLocomotionPresentation::StartDuration; break;
    case EDMLocomotion::Stop: Index = bRifle ? RifleStop : Stop; Duration = FDMLocomotionPresentation::StopDuration; break;
    case EDMLocomotion::TurnLeft: Index = bRifle ? RifleTurnL : TurnL; Duration = FDMLocomotionPresentation::TurnDuration; break;
    case EDMLocomotion::TurnRight: Index = bRifle ? RifleTurnR : TurnR; Duration = FDMLocomotionPresentation::TurnDuration; break;
    default: break;
    }
    UAnimSequence* MotionClip = Clips[Index];
    if (Kind >= 11 && Kind <= 15)
    {
        const int32 MotionIndex = Motion == EDMLocomotion::Run ? 1 : Motion == EDMLocomotion::Start ? 2 :
            Motion == EDMLocomotion::Stop ? 3 : Motion == EDMLocomotion::TurnLeft ? 4 : Motion == EDMLocomotion::TurnRight ? 5 : 0;
        MotionClip = EnemyLocomotion[(Kind-11)*6 + MotionIndex];
    }
    const float Rate = Duration > 0 && MotionClip ? MotionClip->GetPlayLength() / Duration :
        Motion == EDMLocomotion::Run ? FMath::Clamp(Speed / 420.f, .4f, 1.5f) : 1.f;
    Play(MotionClip, Duration == 0, Rate, Previous != Motion);
    // Real movement supersedes the cosmetic attack-facing hold; stationary poses keep facing the last attack target.
    if (Speed > 35.f) { FacingOffset = 0.f; }
    Actor->GetMesh()->SetRelativeRotation(FRotator(0, -90 + Locomotion.VisualYaw(Now) + FacingOffset, 0));
}
FString UDMCombatPresentation::CurrentClip() const { return Playing ? Playing->GetName() : TEXT("none"); }
void UDMCombatPresentation::Cue(uint8 Event, FVector Target)
{
    if (GetNetMode() == NM_DedicatedServer) { return; }
    if (Event == 0) { AimPoint = Target; AimUntil = GetWorld()->GetTimeSeconds() + .3f; }
    UpdatePresentation();
    auto* Actor = CastChecked<ADMCombatant>(GetOwner());
    auto Burst = [&](FVector From, FVector To, FLinearColor Color, uint8 Style)
    { if (auto* FX = GetWorld()->SpawnActor<ADMAttackFX>()) { FX->Initialize(From, To, Color, Style); } };
    if (Event == 4)
    {
        if (Explosion) { if (auto* FX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Explosion, Target, FRotator::ZeroRotator, FVector(.28f))) { FX->SetAutoDestroy(true); FTimerHandle Timer; GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(FX, [FX] { FX->DestroyComponent(); }), .65f, false); } }
        return;
    }
    if (Event == 2)
    {
        Burst(Target, Target, FLinearColor(.65f, .025f, .02f), 5);
        if (!Actor->IsDown() && !ActionClip && GetWorld()->GetTimeSeconds() - LastHitTime > .7f)
        { Action(Clips[Hit], .22f); LastHitTime = GetWorld()->GetTimeSeconds(); }
        return;
    }
    if (Actor->IsDown()) { return; }
    if (Event == 0)
    {
        const bool bRifle = UsesGunPose();
        UAnimSequence* AttackClip = Kind == 13 ? EnemyActions[0] : Kind == 12 ? EnemyActions[1] :
            Clips[bRifle ? (Kind == 2 ? RifleAimFire : RifleFire) : Kind == 14 ? Grenade : Kind == 3 ? CastGesture : (Actor->Investigator->Combo % 2 ? JabL : JabR)];
        Action(AttackClip, Kind == 13 || Kind == 12 || Kind == 14 ? .55f : bRifle ? .25f : .4f);
        const FName Muzzle = Kind >= 11 ? TEXT("ANCHOR_muzzle") : TEXT("Muzzle");
        const FVector Hand = bRifle && HeldItem && HeldItem->DoesSocketExist(Muzzle) ? HeldItem->GetSocketLocation(Muzzle) : Actor->GetMesh()->GetSocketLocation(TEXT("hand_r"));
        if (bRifle)
        {
            Burst(Hand, Hand + (Target - Hand).GetSafeNormal() * 12, FLinearColor(5, 2, .3f), 4);
            Burst(Hand, Target, FLinearColor(3, 1.8f, .4f), 1);
            if (Kind == 15)
            {
                // Three cosmetic tracers for one accepted SMG attack; no extra damage.
                for (float Delay : {.07f,.14f})
                { FTimerHandle Timer; GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this,[this,Target]
                    { auto* OwnerActor=Cast<ADMCombatant>(GetOwner()); if (!OwnerActor || OwnerActor->IsDown() || !HeldItem) { return; }
                      if(auto* FX=GetWorld()->SpawnActor<ADMAttackFX>()) { FX->Initialize(HeldItem->GetSocketLocation(TEXT("ANCHOR_muzzle")),Target,FLinearColor(3,1.8f,.4f),1); }
                    }),Delay,false); }
            }
        }
        else if (Kind == 14) { Burst(Hand,Target,FLinearColor(3,.4f,.05f),6); }
        else if (Kind == 3) { Burst(Hand, Target, FLinearColor(.6f, .15f, 2), 3); }
        else
        { Burst(Target, Target, FLinearColor(2, 1.4f, .6f), 4); }
    }
    else if (Event == 1)
    {
        if (Kind != 2) { Action(Clips[Kind == 1 ? Grenade : Kind == 4 ? Hold : CastGesture], .55f); }
        Burst(Actor->GetMesh()->GetSocketLocation(TEXT("hand_r")), Target, Actor->Investigator->Color() * 2, Kind == 3 ? 3 : 4);
    }
    else if (Event == 5) { Action(Clips[Hold],.6f); }
    else if (Event == 8) { Action(EnemyActions[1],.3f); Burst(Target,Target,FLinearColor(2,.6f,.15f),9); }
    else if (Event == 6) { Action(Clips[Grenade],.6f); Burst(Actor->GetMesh()->GetSocketLocation(TEXT("hand_r")),Target,FLinearColor(2,.25f,.05f),6); }
    else if (Event == 7) { Action(Kind == 15 ? EnemyActions[2] : Clips[CastGesture],.7f); Burst(Target,Target,FLinearColor(2,.12f,.05f),Kind == 15 ? 8 : 7); }
    else if (Event == 3) { Action(Clips[Grenade], .4f); Burst(Target, Target, FLinearColor(2, 1.5f, .4f), 4); }
    // ---- Sapper kit. 9/11/13 play on the caster; 10/12/14 play on the enemy the effect reached.
    else if (Event == 9)
    {
        Action(Clips[MGShoot], .8f);
        const FVector Barrel = HeldItem && HeldItem->DoesSocketExist(TEXT("Muzzle")) ? HeldItem->GetSocketLocation(TEXT("Muzzle")) : Actor->GetMesh()->GetSocketLocation(TEXT("hand_r"));
        Burst(Barrel, Barrel + (Target - Barrel).GetSafeNormal() * 12, FLinearColor(5, 2.4f, .4f), 4);
        Spawn(MuzzleFlash, Barrel, .35f, .8f);
    }
    else if (Event == 10) { Burst(Target, Target, FLinearColor(2, 1.2f, .35f), 5); }
    else if (Event == 11) { Action(Clips[Place], .6f); Burst(Target, Target, FLinearColor(1.6f, 1.1f, .3f), 9); }
    else if (Event == 12) { Spawn(WireSnap, Target, .5f, 1.f); Spawn(Shock, Target, .5f, 1.f); Burst(Target, Target, FLinearColor(3, 1.6f, .5f), 9); }
    else if (Event == 13) { Attach(DeferredAura, 6.f); }
    else if (Event == 14) { Spawn(TargetMark, Target + FVector(0, 0, 40), .6f, 1.5f); }
    // ---- Photographer kit. The camera rises for the flash and the photograph; Develop plays on the subject.
    else if (Event == 15)
    {
        CameraUntil = GetWorld()->GetTimeSeconds() + .6f;
        const FVector Lens = HeldItem ? HeldItem->GetComponentLocation() : Actor->GetMesh()->GetSocketLocation(TEXT("hand_r"));
        Spawn(Flashbulb, Lens, .5f, .7f);
        Burst(Lens, Target, FLinearColor(4, 4, 5), 1);
    }
    else if (Event == 16) { Spawn(Developed, Target + FVector(0, 0, 40), .4f, .9f); Burst(Target, Target, FLinearColor(1.4f, 2.6f, 4), 5); }
    else if (Event == 17) { CameraUntil = GetWorld()->GetTimeSeconds() + 1.f; Attach(PhotographAura, 8.f); }
    // ---- Medium kit. 18 plays on the caster when spirits are called and again where each one lands.
    else if (Event == 18)
    {
        if (Target.Equals(Actor->GetActorLocation(), 1.f)) { Action(Clips[CastUp], .7f); }
        Spawn(SpiritArrival, Target, .5f, 1.2f);
    }
    else if (Event == 19) { Action(Clips[Cast1], .6f); Spawn(Intervention, Target, .5f, 1.f); Burst(Actor->GetMesh()->GetSocketLocation(TEXT("hand_r")), Target, FLinearColor(.7f, .3f, 2.2f), 3); }
    else if (Event == 20) { Spawn(SeanceCircle, Target - FVector(0, 0, 80), .6f, 8.f); }
    const FVector Direction = Target - Actor->GetActorLocation();
    if (!Direction.IsNearlyZero())
    {
        FacingOffset = FRotator::NormalizeAxis(Direction.Rotation().Yaw - Actor->GetActorRotation().Yaw);
        Actor->GetMesh()->SetRelativeRotation(FRotator(0, FacingOffset - 90, 0));
    }
}

bool UDMCombatPresentation::UsesGunPose() const { return Kind == 1 || Kind == 2 || Kind == 11 || Kind == 13 || Kind == 15; }
void UDMCombatPresentation::UpdateEnemyGrip()
{
    if (Kind < 11 || Kind > 15 || !HeldItem || !HeldItem->DoesSocketExist(TEXT("ANCHOR_grip_r"))) { return; }
    auto* Actor = CastChecked<ADMCombatant>(GetOwner());
    HeldItem->SetRelativeTransform(HeldItem->GetSocketTransform(TEXT("ANCHOR_grip_r"),RTS_Component).Inverse());
    const FVector Hand = Actor->GetMesh()->GetSocketLocation(TEXT("SOCKET_hand_r"));
    const bool bAim = GetWorld()->GetTimeSeconds() < AimUntil && HeldItem->DoesSocketExist(TEXT("ANCHOR_muzzle"));
    if (!bAim && !HeldItem->DoesSocketExist(TEXT("ANCHOR_support_l"))) { return; }
    const FVector From = HeldItem->GetSocketLocation(bAim ? TEXT("ANCHOR_muzzle") : TEXT("ANCHOR_support_l")) - Hand;
    const FVector To = (bAim ? AimPoint : Actor->GetMesh()->GetSocketLocation(TEXT("hand_l"))) - Hand;
    if (!From.IsNearlyZero() && !To.IsNearlyZero())
    {
        const FQuat Q = FQuat::FindBetweenNormals(From.GetSafeNormal(),To.GetSafeNormal());
        HeldItem->SetWorldLocationAndRotation(Hand + Q.RotateVector(HeldItem->GetComponentLocation()-Hand), Q*HeldItem->GetComponentQuat());
    }
}
