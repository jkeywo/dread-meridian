#include "DMCombatant.h"
#include "DMCombatPresentation.h"
#include "DMHealthAttributes.h"
#include "DMRelicComponent.h"
#include "DMObjective.h"
#include "DMVision.h"
#include "EngineUtils.h"
#include "DMAttackFX.h"
#include "DMScroungePickup.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DMBasicAttackAbility.h"
#include "DMCombatGameMode.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ADMCombatant::ADMCombatant()
{
    Swamp=CreateDefaultSubobject<UDMSwampThing>(TEXT("SwampThing"));
    Relics=CreateDefaultSubobject<UDMRelicComponent>(TEXT("Relics"));
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    GetCapsuleComponent()->InitCapsuleSize(30, 90);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GetCharacterMovement()->MaxWalkSpeed = 420;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    bUseControllerRotationYaw = false;
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    AbilitySystem->SetIsReplicated(true);
    AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Full);
    Attributes = CreateDefaultSubobject<UDMHealthAttributes>(TEXT("Attributes"));
    Smuggler = CreateDefaultSubobject<UDMSmugglerComponent>(TEXT("SmugglerFaction"));
    MadnessCore = CreateDefaultSubobject<UDMMadnessComponent>(TEXT("MadnessCore"));
    Injuries = CreateDefaultSubobject<UDMInjuryComponent>(TEXT("Injuries"));
    Resolve = CreateDefaultSubobject<UDMBreakComponent>(TEXT("Resolve"));
    Primary = CreateDefaultSubobject<UDMPrimaryComponent>(TEXT("Primary"));
    Kit = CreateDefaultSubobject<UDMKitComponent>(TEXT("Kit"));
    Presentation = CreateDefaultSubobject<UDMCombatPresentation>(TEXT("Presentation"));
    Progression = CreateDefaultSubobject<UDMProgressionComponent>(TEXT("Progression"));
    Investigator = CreateDefaultSubobject<UDMInvestigatorComponent>(TEXT("Investigator"));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Run(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd"));
    GetMesh()->SetSkeletalMeshAsset(Manny.Object);
    GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
    GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    IdleAnimation = Idle.Object; RunAnimation = Run.Object;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreyboxBody"));
    Body->SetupAttachment(GetRootComponent());
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cylinder"));
    Body->SetStaticMesh(BodyMesh.Object);
    Body->SetVisibility(false);
    Body->SetRelativeScale3D(FVector(.6, .6, 1));
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusLabel"));
    Label->SetupAttachment(GetRootComponent());
    // The badge's rotation is driven entirely by the per-tick camera-facing update below, never by the capsule's
    // own facing - absolute rotation stops the character's turning from dragging the badge along with it.
    Label->SetUsingAbsoluteRotation(true);
    Label->SetRelativeLocation(FVector(0, 0, 130));
    Label->SetWorldSize(20);
    Label->SetHorizontalAlignment(EHTA_Center);
    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(GetRootComponent());
    CameraArm->SetUsingAbsoluteRotation(true);
    CameraArm->SetRelativeRotation(FRotator(-65, 0, 0));
    CameraArm->TargetArmLength = 1500;
    CameraArm->bDoCollisionTest = false;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm);
    SetNetUpdateFrequency(20);
}

void ADMCombatant::BeginPlay()
{
    Super::BeginPlay();
    AbilitySystem->InitAbilityActorInfo(this, this);
    GetMesh()->PlayAnimation(IdleAnimation, true);
    Primary->Initialize();
    Kit->Initialize();
    if (HasAuthority()) { BasicAttackHandle = AbilitySystem->GiveAbility(FGameplayAbilitySpec(UDMBasicAttackAbility::StaticClass(), 1)); }
}

void ADMCombatant::InitializeCombatant(const FString& Id, bool bEnemy, float MaxHP, float Damage, float InitialShield)
{
    check(HasAuthority());
    EntityId = Id;
    bIsEnemy = bEnemy;
    AttackDamage = Damage;
    Attributes->InitMaxHealth(MaxHP);
    Attributes->InitHealth(MaxHP);
    Attributes->InitShield(InitialShield);
    Resolve->Reset();
    Injuries->Reset();
    ForceNetUpdate();
}

UAbilitySystemComponent* ADMCombatant::GetAbilitySystemComponent() const { return AbilitySystem; }
float ADMCombatant::Health() const { return Attributes->GetHealth(); }
float ADMCombatant::MaxHealth() const { return Attributes->GetMaxHealth(); }
float ADMCombatant::Shield() const { return Attributes->GetShield(); }
void ADMCombatant::MoveToward(const FVector& Location) { MoveGoal = Location; bHasMoveGoal = true; }
void ADMCombatant::StopGoal() { bHasMoveGoal = false; }

void ADMCombatant::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    GetCharacterMovement()->MaxWalkSpeed = IsDown() || IsRestrained() || IsStunned() ? 0.f : ReplicatedMoveSpeed;
    if (IsDown() || IsRestrained() || IsStunned()) { GetCharacterMovement()->StopMovementImmediately(); }
    // The server drives the charge; the owning client must not predict against it.
    else if (Kit->IsCharging()) { if (HasAuthority()) { Kit->AdvanceCharge(DeltaSeconds); } }
    else if (bHasMoveGoal && IsLocallyControlled())
    {
        const FVector Direction = (MoveGoal - GetActorLocation()).GetSafeNormal2D();
        if (FVector::DistSquared2D(MoveGoal, GetActorLocation()) > FMath::Square(35.f)) { AddMovementInput(Direction); }
        else { StopGoal(); }
    }
    if (GetNetMode() != NM_DedicatedServer)
    {
        // Health now reads from the overhead bar the HUD projects; the world label carries identity only.
        // A dead enemy is inert scenery, not a unit to track - drop its nameplate.
        const bool bShowLabel = !(bIsEnemy && IsDown());
        Label->SetVisibility(bShowLabel);
        if (bShowLabel)
        {
            Label->SetText(FText::FromString(DisplayName()));
            Label->SetTextRenderColor(bIsEnemy ? FColor(255, 100, 80) : FColor(100, 220, 255));
            if (!bIsEnemy) { Label->SetTextRenderColor(Investigator->Color().ToFColor(true)); }
            if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
            {
                // Align to the camera's view plane, not its position. The camera sits almost directly above the
                // local pawn, so aiming each badge at the camera's location swivels it toward that pawn instead.
                // Facing back along the view direction gives every badge the same screen-parallel angle, laid
                // flat and horizontal by the camera's own pitch.
                if (PC->PlayerCameraManager)
                { Label->SetWorldRotation((-PC->PlayerCameraManager->GetCameraRotation().Vector()).Rotation()); }
            }
        }
        Presentation->UpdatePresentation();
    }
}

bool ADMCombatant::TryAttack(ADMCombatant* Target)
{
    if (!HasAuthority()) { return false; }
    AttackTarget = Target;
    ADMCombatGameMode* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Mode || !Mode->IsCombatActive() || IsDown() || !IsValid(Target) || Target->IsDown()
        || !IsHostileTo(Target) || !DMVision::CanSee(this,Target) || NextAttackTick > Mode->GetCombatTick()
        || FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(GetAttackRange())) { bTelegraphActive = false; return false; }
    // Bot brain attack-channel gate: the target projection above still updates while held.
    if (bAttackHold) { bTelegraphActive = false; return false; }
    if (IsRestrained() || IsStunned() || !Injuries->CanAttack() || StaggeredUntilTick > 0 || Primary->FrameTarget || Smuggler->IsCasting()) { return false; }
    if (bIsEnemy && !bProfileRange)
    {
        if (TelegraphTarget.Get() != Target) { bTelegraphActive = false; }
        if (!bTelegraphActive) { TelegraphTarget = Target; bTelegraphActive = true; TelegraphEndTick = Mode->GetCombatTick() + 3; StopGoal(); ForceNetUpdate(); return false; }
        if (Mode->GetCombatTick() < TelegraphEndTick) { return false; }
    }
    const bool bResult = AbilitySystem->TryActivateAbility(BasicAttackHandle);
    bTelegraphActive = false;
    return bResult;
}

void ADMCombatant::ApplyAttributeDelta(const FGameplayAttribute& Attribute, float Delta)
{
    UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
    Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = Attribute;
    Modifier.ModifierOp = EGameplayModOp::Additive;
    Modifier.ModifierMagnitude = FScalableFloat(Delta);
    FGameplayEffectSpec Spec(Effect, AbilitySystem->MakeEffectContext(), 1);
    AbilitySystem->ApplyGameplayEffectSpecToSelf(Spec);
}

bool ADMCombatant::ResolveAttack()
{
    ADMCombatant* Target = AttackTarget.Get();
    ADMCombatGameMode* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !Mode || !Mode->IsCombatActive() || IsDown() || bAttackHold || !Target || Target->IsDown()
        || !IsHostileTo(Target) || !DMVision::CanSee(this,Target) || Mode->GetCombatTick() < NextAttackTick
        || FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(GetAttackRange())) { return false; }
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(BasicAttack), false, this);
    // Bodies do not absorb basic attacks; world geometry still blocks the shot.
    for (ADMCombatant* Other : Mode->GetCombatants()) { Query.AddIgnoredActor(Other); }
    if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, Query)) { return false; }
    if (IsRestrained() || IsStunned() || !Injuries->CanAttack() || StaggeredUntilTick > 0 || Primary->FrameTarget || Smuggler->IsCasting()) { return false; }
    NextAttackTick = Mode->GetCombatTick() + AttackIntervalTicks;
    return DealCombatDamage(Target, AttackDamage * (bIsEnemy ? 1.f : Progression->Get().BasicMultiplier()) * Investigator->DamageMultiplier(Target->EntityId, Target->bSuppressed) * (bIsEnemy ? Smuggler->DamageMultiplier(*Mode, Target) : 1.f), TEXT("ability.basic_attack"), true);
}

bool ADMCombatant::DealCombatDamage(ADMCombatant* Target, float Damage, const FString& AbilityId, bool bBasic, bool bHazard)
{
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !Mode || !Mode->IsCombatActive() || IsDown() || !IsValid(Target) || Target->IsDown()
        || Target->GetWorld()!=GetWorld() || !IsHostileTo(Target) || !FMath::IsFinite(Damage) || Damage <= 0) { return false; }
    const float Before = Target->Health();
    if (bBasic && !DMVision::CanSee(this,Target)) { return false; }
    Target->VisionRevealUntil=Mode->GetCombatTick()+30;
    Damage*=Relics->SpendMedal(Target,AbilityId,bBasic);
    if (bBasic && bSwampThing) { Damage*=Swamp->DamageMultiplier(Target); }
    const float ShieldBefore = Target->Shield();
    const float Incoming = Target->IncomingUntilTick > Mode->GetCombatTick() ? FMath::Clamp(Target->IncomingMultiplier, 0.f, 2.f) : 1.f;
    const float BaseDamage = Damage * Kit->OutgoingTo(Target) * Target->Progression->IncomingFrom(this) * Target->Relics->IncomingMultiplier() * MadnessCore->Outgoing(Target) * (1 - FMath::Clamp(Target->SpiritProtection, 0.f, .5f)) * Incoming;
    const float ResolvedDamage = BaseDamage * Target->Injuries->Incoming(FMath::Max(0.f, BaseDamage - ShieldBefore), bHazard);
    const float Absorbed = FMath::Min(ShieldBefore, ResolvedDamage);
    if (Absorbed > 0) { Target->Relics->ShieldSpent(Absorbed); Target->ApplyAttributeDelta(UDMHealthAttributes::GetShieldAttribute(), -Absorbed); }
    Target->ApplyAttributeDelta(UDMHealthAttributes::GetHealthAttribute(), -(ResolvedDamage - Absorbed));
    Target->Threat.Add(EntityId, Before - Target->Health());
    Target->LastDamageTick = Mode->GetCombatTick();
    Target->Kit->RecordBraceHit(this, Incoming > 0 ? BaseDamage / Incoming : Damage, BaseDamage);
    Mode->NoteDamage(*this, *Target, ResolvedDamage, Before + ShieldBefore);
    TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("actor_id"), EntityId);
    Data->SetStringField(TEXT("target_id"), Target->EntityId);
    Data->SetStringField(TEXT("ability_id"), AbilityId);
    Data->SetNumberField(TEXT("damage"), ResolvedDamage);
    Data->SetNumberField(TEXT("health_before"), Before);
    Data->SetNumberField(TEXT("health_after"), Target->Health());
    Data->SetNumberField(TEXT("shield_before"), ShieldBefore);
    Data->SetNumberField(TEXT("shield_after"), Target->Shield());
    Data->SetBoolField(TEXT("persistent_hazard"), bHazard);
    Mode->Emit(TEXT("combat.damage"), Data);
    MadnessCore->OnDamage(Target, Before - Target->Health());
    Target->MadnessCore->InterruptGrounding();
    if (bBasic) { MadnessCore->InterruptGrounding(); }
    Target->Injuries->RecordLoss(Before - Target->Health(), Target->IsDown(), bHazard, this, AbilityId);
    if (bBasic) { Injuries->OnAttack(); }
    const bool bFinisher = bBasic && Investigator->OnHit(Target->EntityId, Mode->GetCombatTick());
    Target->Investigator->Pressure(Mode->GetCombatTick(), Before - Target->Health() + Absorbed);
    // Dig In converts absorbed pressure into extra Momentum (doubled while Drowned Man Walking holds the floor).
    if (Target->IsBraced()) { Target->Investigator->Pressure(Mode->GetCombatTick(), ResolvedDamage * .5f * (Target->Investigator->MomentumFloor > 0 ? 2.f : 1.f)); }
    if (bBasic) { MulticastPresentation(0, Target->GetActorLocation() + FVector(0, 0, 15)); }
    Target->MulticastPresentation(2, Target->GetActorLocation() + FVector(0, 0, 15));
    RecordResources(TEXT("basic_hit")); Target->RecordResources(TEXT("incoming_pressure"));
    if (bFinisher && Target->bIsEnemy && Target->bCommonEnemy && !Target->IsDown())
    { FDMControl Control; Control.Displacement = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * 45;
      Control.StaggerTicks = 3; Target->ApplyControl(Control, this, TEXT("ability.basic.finisher")); }
    if (Target->IsDown())
    {
        Target->Resolve->Reset(); Target->StunnedUntilTick = 0; Target->StaggeredUntilTick = 0;
        Mode->NoteDowned(*Target);
        Target->Smuggler->Cancel(); Target->StopGoal(); Target->Primary->CancelChannel(); Target->Primary->ReleaseClinch(); Target->Kit->Cancel(false);
        if (Target->HeldBy) { Target->HeldBy->Primary->ReleaseClinch(); }
        Target->bTelegraphActive = false;
        if (Target->bIsEnemy && Target->bHumanEnemy)
        { GetWorld()->SpawnActor<ADMScroungePickup>(Target->GetActorLocation() + FVector(0, 0, -65), FRotator::ZeroRotator); }
        for (ADMCombatant* Ally : Mode->GetCombatants())
        { Ally->Investigator->ThinPlace(Target->GetActorLocation(), Target->bIsEnemy ? 15 : 25); Ally->RecordResources(TEXT("thin_place")); }
        if (Target->bIsEnemy) { Target->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
        TSharedRef<FJsonObject> Down = MakeShared<FJsonObject>();
        Down->SetStringField(TEXT("entity_id"), Target->EntityId);
        Down->SetNumberField(TEXT("injuries"), Target->InjuryCount);
        Down->SetNumberField(TEXT("grievous"), Target->GrievousCount);
        Mode->Emit(Target->bIsEnemy ? TEXT("combat.killed") : TEXT("combat.downed"), Down);
    }
    Target->ForceNetUpdate();
    return true;
}

bool ADMCombatant::Revive(ADMCombatant* Ally)
{
    ADMCombatGameMode* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !Mode || !Mode->IsCombatActive() || IsDown() || IsStunned() || IsRestrained() || bIsEnemy || !Ally
        || Ally == this || Ally->bIsEnemy || !Ally->IsDown()
        || FVector::DistSquared(GetActorLocation(), Ally->GetActorLocation()) > FMath::Square(160.f)) { return false; }
    Ally->ApplyAttributeDelta(UDMHealthAttributes::GetHealthAttribute(), Ally->Attributes->GetMaxHealth() * .5f);
    for (ADMCombatant* Enemy : Mode->GetCombatants())
    {
        if (Enemy->bIsEnemy) { Enemy->Threat.Scale(Ally->EntityId,.25f); }
    }
    TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("actor_id"), EntityId);
    Data->SetStringField(TEXT("target_id"), Ally->EntityId);
    Data->SetNumberField(TEXT("health_after"), Ally->Health());
    Mode->Emit(TEXT("combat.revived"), Data);
    Mode->NoteRevive();
    Ally->ForceNetUpdate();
    return true;
}

void ADMCombatant::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADMCombatant, HeldBy); DOREPLIFETIME(ADMCombatant, bBreakVulnerable);
    DOREPLIFETIME(ADMCombatant, bTelegraphActive); DOREPLIFETIME(ADMCombatant, SpiritProtection); DOREPLIFETIME(ADMCombatant, SpiritSlow);
    DOREPLIFETIME(ADMCombatant, ReplicatedMoveSpeed);
    DOREPLIFETIME(ADMCombatant, StunnedUntilTick); DOREPLIFETIME(ADMCombatant, StaggeredUntilTick);
    DOREPLIFETIME(ADMCombatant, Break); DOREPLIFETIME(ADMCombatant, BrokenUntilTick); DOREPLIFETIME(ADMCombatant, SuppressedUntilTick);
    DOREPLIFETIME(ADMCombatant, IncomingMultiplier); DOREPLIFETIME(ADMCombatant, ReachBonus);
    DOREPLIFETIME(ADMCombatant, bHumanEnemy);
    DOREPLIFETIME(ADMCombatant, bCommonEnemy);
    DOREPLIFETIME(ADMCombatant, bSuppressed);
    DOREPLIFETIME(ADMCombatant, EntityId);
    DOREPLIFETIME(ADMCombatant, bIsEnemy);
    DOREPLIFETIME(ADMCombatant, InjuryCount);
    DOREPLIFETIME(ADMCombatant, GrievousCount);
    DOREPLIFETIME(ADMCombatant, ControlKind);
    DOREPLIFETIME(ADMCombatant, AttackTargetId);
    DOREPLIFETIME(ADMCombatant, ReviverId);
    DOREPLIFETIME(ADMCombatant, EncounterLabel);
    DOREPLIFETIME(ADMCombatant, ReviveProgress);
    DOREPLIFETIME(ADMCombatant, AttackIntervalTicks);
    DOREPLIFETIME(ADMCombatant, bSwampThing);
    DOREPLIFETIME(ADMCombatant, bRequiresVision);
    DOREPLIFETIME(ADMCombatant, NextAttackTick);
}

void ADMCombatant::InitializeInvestigator(EDMInvestigator Kind, bool bUseProfileTuning)
{
    Investigator->Initialize(Kind); bProfileRange = bUseProfileTuning;
    if (!bUseProfileTuning) { AttackDamage = Investigator->Damage(); AttackIntervalTicks = Investigator->Interval(); }
}
FString ADMCombatant::DisplayName() const
{ return !EncounterLabel.IsEmpty() ? EncounterLabel : bIsEnemy && Smuggler->Role != EDMSmuggler::None ? Smuggler->Name() : bIsEnemy ? Investigator->DisplayName() + TEXT(" ") + EntityId.Mid(EntityId.Find(TEXT(".")) + 1) : Investigator->DisplayName(); }
float ADMCombatant::GetAttackRange() const
{ return (bSwampThing ? Swamp->Range() : bProfileRange ? 420 : bIsEnemy && Smuggler->Role != EDMSmuggler::None ? Smuggler->Range() : Investigator->Range()) + ReachBonus; }
bool ADMCombatant::IsHostileTo(const ADMCombatant* Other) const
{
    if (!IsValid(Other) || Other==this) { return false; }
    if (bIsEnemy!=Other->bIsEnemy) { return true; }
    return bIsEnemy && bSwampThing!=Other->bSwampThing && (bSwampThing ? Other->bHumanEnemy : bHumanEnemy);
}
bool ADMCombatant::IsNetRelevantFor(const AActor* RealViewer,const AActor* ViewTarget,const FVector& SrcLocation) const
{ return (!bRequiresVision || DMVision::Relevant(this,RealViewer)) && Super::IsNetRelevantFor(RealViewer,ViewTarget,SrcLocation); }
float ADMCombatant::EffectiveResistance() const { return FMath::Max(FMath::Max3(Investigator->Resistance(), BraceResistance, Kit->ChargeResistance()),Relics->ProtectsObjective() ? .75f : 0.f); }
void ADMCombatant::StepInvestigator(int32 Tick)
{
    if (!HasAuthority()) { return; }
    Progression->ChooseForBot();
    Investigator->Step(Tick, !IsDown() && AttackTarget.IsValid() && !AttackTarget->IsDown() ? AttackTarget->EntityId : TEXT(""));
    RecordResources(TEXT("resource_step"));
    if (IsDown() || IsRestrained() || IsStunned()) { bTelegraphActive = false; }
    if (SuppressedUntilTick > 0 && Tick >= SuppressedUntilTick) { SuppressedUntilTick = 0; bSuppressed = false; }
    if (IncomingUntilTick > 0 && Tick >= IncomingUntilTick) { IncomingUntilTick = 0; IncomingMultiplier = 1; }
    Slows.RemoveAll([&](const FDMSlow& S) { return S.UntilTick <= Tick; });
    const float Slow = FMath::Max(DMKitRules::EffectiveSlow(Slows, Tick), SpiritSlow);
    ReplicatedMoveSpeed = (bSwampThing ? Swamp->Speed() : bIsEnemy && Smuggler->Role != EDMSmuggler::None ? Smuggler->Speed() : 420) * (1 + Investigator->Stickiness() * .25f) * (1 - Slow * (1 - EffectiveResistance())) * Injuries->MovementFactor * Relics->MovementMultiplier();
}
void ADMCombatant::StepControl(int32 Tick)
{
    if (!HasAuthority()) { return; }
    Resolve->Step(Tick);
    Injuries->Step(Tick);
    MadnessCore->Step(Tick);
    Relics->Step(Tick);
    if (Tick >= StunnedUntilTick) { StunnedUntilTick = 0; }
    if (Tick >= StaggeredUntilTick) { StaggeredUntilTick = 0; }
}
void ADMCombatant::AddBreak(float Amount) { Resolve->AddPressure(Amount); }
void ADMCombatant::InterruptControl()
{
    if (HasAuthority()) { ++ControlInterruptSerial; }
    if (!HasAuthority()) { return; }
    for (TActorIterator<ADMObjective> It(GetWorld()); It; ++It) { if (It->IsInteracting(this)) { It->Release(this,true); } }
    bTelegraphActive = false;
    Primary->CancelChannel(); Primary->ReleaseClinch();
    Kit->ChargeUntilTick = 0;
    if (bIsEnemy && (Smuggler->IsCasting() || Smuggler->OrderTarget))
    {
        if (auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
        {
            auto Data = MakeShared<FJsonObject>(); Data->SetStringField(TEXT("actor_id"), EntityId);
            Data->SetStringField(TEXT("enemy_role"), StaticEnum<EDMSmuggler>()->GetNameStringByValue(static_cast<int64>(Smuggler->Role)));
            Data->SetStringField(TEXT("stage"), TEXT("interrupted")); Mode->Emit(TEXT("smuggler.signature"), Data);
        }
        Smuggler->Interrupt();
    }
    ForceNetUpdate();
}
void ADMCombatant::ApplyControl(const FDMControl& Control, ADMCombatant* Source, const FString& AbilityId)
{
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !Mode || !Mode->IsCombatActive() || IsDown()
        || !FMath::IsFinite(Control.Damage) || !FMath::IsFinite(Control.Slow) || !FMath::IsFinite(Control.BreakPressure)
        || Control.Displacement.ContainsNaN() || Control.SlowTicks < 0 || Control.SlowTicks > 1000000
        || Control.StaggerTicks < 0 || Control.StaggerTicks > 1000000 || Control.StunTicks < 0 || Control.StunTicks > 1000000
        || Control.Damage < 0 || Control.BreakPressure < 0 || Control.Slow < 0 || Control.Slow > 1
        || (Source && (!IsValid(Source) || Source->GetWorld() != GetWorld() || Source->IsDown() || !IsHostileTo(Source)))) { return; }
    const int32 Tick = Mode->GetCombatTick(); StepControl(Tick);
    // The breaking hit opens the window for subsequent control; it resolves against the preceding state.
    FDMControl Empowered=Control;
    if (Relics->ProtectsObjective()) { Empowered.Slow*=.25f; Empowered.StunTicks=FMath::CeilToInt(Empowered.StunTicks*.25f); Empowered.StaggerTicks=FMath::CeilToInt(Empowered.StaggerTicks*.25f); }
    const float Medal=Source ? Source->Relics->SpendMedal(this,AbilityId,false) : 1.f;
    Empowered.Damage*=Medal; Empowered.BreakPressure*=Medal;
    Empowered.Slow=FMath::Min(1.f,Empowered.Slow*Relics->ControlExposure());
    Empowered.StunTicks=FMath::CeilToInt(Empowered.StunTicks*Relics->ControlExposure());
    Empowered.StunTicks=FMath::CeilToInt(Empowered.StunTicks*Medal); Empowered.StaggerTicks=FMath::CeilToInt(Empowered.StaggerTicks*Medal);
    FDMControl R = DMKitRules::ResolveControl(Empowered, !Resolve->IsProtected(), bBreakVulnerable,
        Resolve->Settings.ProtectedSlowFactor, Resolve->InterruptUntilTick > Tick);
    if (R.Damage > 0 && Source) { Source->DealCombatDamage(this, R.Damage, AbilityId); }
    if (IsDown()) { return; }
    if (bBreakVulnerable)
    {
        const int32 Remaining = FMath::Max(0, BrokenUntilTick - Tick);
        R.SlowTicks = FMath::Min(R.SlowTicks, Remaining);
        R.StaggerTicks = FMath::Min(R.StaggerTicks, Remaining);
        R.StunTicks = FMath::Min(R.StunTicks, Remaining);
    }
    if (R.Slow > 0 && R.SlowTicks > 0) { ApplySlow(R.Slow, Tick + R.SlowTicks); }
    if (!R.Displacement.IsNearlyZero()) { ApplyDisplacement(R.Displacement); }
    if (R.StaggerTicks > 0)
    {
        StaggeredUntilTick = FMath::Max(StaggeredUntilTick, Tick + R.StaggerTicks);
        bTelegraphActive = false;
    }
    if (R.StunTicks > 0)
    {
        StunnedUntilTick = FMath::Max(StunnedUntilTick, Tick + R.StunTicks);
        StopGoal(); GetCharacterMovement()->StopMovementImmediately(); InterruptControl();
    }
    if (R.bInterrupt) { InterruptControl(); }
    float Pressure = R.BreakPressure;
    if (Resolve->IsProtected() && !bBreakVulnerable && Pressure == 0
        && (Control.Slow > 0 || Control.StaggerTicks > 0 || Control.StunTicks > 0 || Control.bInterrupt || !Control.Displacement.IsNearlyZero()))
    { Pressure = Resolve->Settings.ControlPressure; }
    if (Pressure > 0) { Resolve->AddPressure(Pressure, Source, AbilityId); }
    if (Source) { Source->Relics->AcceptedControl.Broadcast(this,R); }
    auto Data = MakeShared<FJsonObject>(); Data->SetStringField(TEXT("entity_id"), EntityId);
    Data->SetStringField(TEXT("ability_id"), AbilityId);
    if (Source) { Data->SetStringField(TEXT("source_id"), Source->EntityId); }
    Data->SetNumberField(TEXT("slow"), R.Slow); Data->SetNumberField(TEXT("slow_ticks"), R.SlowTicks);
    Data->SetNumberField(TEXT("stagger_ticks"), R.StaggerTicks); Data->SetNumberField(TEXT("stun_ticks"), R.StunTicks);
    Data->SetBoolField(TEXT("interrupt_permitted"), R.bInterrupt); Mode->Emit(TEXT("control.resolved"), Data);
    ForceNetUpdate();
}
void ADMCombatant::ApplySuppression(int32 UntilTick, ADMCombatant* Source)
{
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !Mode || IsDown() || !bIsEnemy) { return; }
    bSuppressed = true; SuppressedUntilTick = FMath::Max(SuppressedUntilTick, UntilTick);
    FDMControl Control; Control.Slow = .4f; Control.SlowTicks = FMath::Max(1, UntilTick - Mode->GetCombatTick()); Control.BreakPressure = 5;
    ApplyControl(Control, Source, TEXT("ability.w.suppression"));
}
void ADMCombatant::AddShield(float Amount)
{
    if (!HasAuthority()) { return; }
    const float Before = Shield();
    const float Delta = DMKitRules::ShieldGain(Before, MaxHealth() * .5f, Amount);
    if (Delta <= 0) { return; }
    ApplyAttributeDelta(UDMHealthAttributes::GetShieldAttribute(), Delta);
    if (Shield() > Before)
    {
        if (auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
        {
            auto Data = MakeShared<FJsonObject>(); Data->SetStringField(TEXT("entity_id"), EntityId);
            Data->SetNumberField(TEXT("shield_before"), Before); Data->SetNumberField(TEXT("shield_after"), Shield());
            Data->SetNumberField(TEXT("amount"), Shield() - Before); Mode->Emit(TEXT("combat.shield_gained"), Data);
        }
        ForceNetUpdate();
    }
}
void ADMCombatant::SetAttackTarget(ADMCombatant* Target)
{ AttackTarget = Target; if (HasAuthority()) { AttackTargetId = Target ? Target->EntityId : FString(); } }
void ADMCombatant::SetAttackHold(bool bHold)
{ bAttackHold = bHold; if (bHold) { bTelegraphActive = false; } }
void ADMCombatant::SetReviveChannel(const FString& Reviver, float Progress)
{ if (HasAuthority()) { ReviverId = Reviver; ReviveProgress = FMath::Clamp(Progress, 0.f, 1.f); } }
void ADMCombatant::ApplySlow(float Fraction, int32 UntilTick)
{
    if (!HasAuthority()) { return; }
    const auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    DMKitRules::AddSlow(Slows, Fraction, UntilTick, Mode ? Mode->GetCombatTick() : 0);
}
void ADMCombatant::ApplyDisplacement(FVector Delta)
{
    if (!HasAuthority() || IsDown() || Delta.ContainsNaN()) { return; }
    const FVector Before = GetActorLocation(); SetActorLocation(Before + Delta * (1 - EffectiveResistance()), true);
    Injuries->OnDisplacement(FVector::Dist2D(Before, GetActorLocation()));
}
void ADMCombatant::MulticastAttackFX_Implementation(FVector From, FVector To, FLinearColor Color, uint8 Style)
{
    if (GetNetMode() == NM_DedicatedServer) { return; }
    if (ADMAttackFX* Effect = GetWorld()->SpawnActor<ADMAttackFX>()) { Effect->Initialize(From, To, Color, Style); }
}
void ADMCombatant::RecordResources(const FString& Reason)
{
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Mode || bIsEnemy) { return; }
    auto Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("entity_id"), EntityId);
    Data->SetStringField(TEXT("investigator"), Investigator->DisplayName());
    Data->SetNumberField(TEXT("momentum"), Investigator->Momentum); Data->SetNumberField(TEXT("charges"), Investigator->Charges);
    Data->SetNumberField(TEXT("components"), Investigator->Components); Data->SetNumberField(TEXT("combo"), Investigator->Combo);
    TArray<TSharedPtr<FJsonValue>> Exposure, Attention;
    for (const auto& S : Investigator->Exposure)
    { auto Item = MakeShared<FJsonObject>(); Item->SetStringField(TEXT("subject_id"), S.Id); Item->SetNumberField(TEXT("value"), S.Value); Exposure.Add(MakeShared<FJsonValueObject>(Item)); }
    for (const auto& S : Investigator->Spirits)
    { auto Item = MakeShared<FJsonObject>(); Item->SetStringField(TEXT("spirit_id"), S.Id); Item->SetNumberField(TEXT("value"), S.Value); Attention.Add(MakeShared<FJsonValueObject>(Item)); }
    Data->SetArrayField(TEXT("exposure"), Exposure); Data->SetArrayField(TEXT("attention"), Attention);
    FString Snapshot; FJsonSerializer::Serialize(Data, TJsonWriterFactory<>::Create(&Snapshot));
    if (Snapshot == LastResourceSnapshot) { return; }
    LastResourceSnapshot = Snapshot;
    Data->SetStringField(TEXT("reason"), Reason);
    Mode->Emit(TEXT("investigator.resources"), Data); ForceNetUpdate();
}

void ADMCombatant::MulticastPresentation_Implementation(uint8 Event, FVector Target)
{ Presentation->Cue(Event, Target); }

void ADMCombatant::RemoveShield(float Amount)
{ if (HasAuthority() && FMath::IsFinite(Amount) && Amount>0) { ApplyAttributeDelta(UDMHealthAttributes::GetShieldAttribute(),-FMath::Min(Shield(),Amount)); ForceNetUpdate(); } }
float ADMCombatant::HealHealth(float Amount)
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !M->IsCombatActive() || IsDown() || !FMath::IsFinite(Amount) || Amount <= 0) { return 0; }
    const float Before = Health();
    const float Delta = FMath::Min(MaxHealth() - Before, Amount * Injuries->HealingFactor);
    const float Excess=FMath::Max(0.f,Amount*Injuries->HealingFactor-Delta);
    if (Excess>0) { Relics->ExcessHealing.Broadcast(Excess); }
    if (Delta <= 0) { return 0; }
    ApplyAttributeDelta(UDMHealthAttributes::GetHealthAttribute(), Delta);
    auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"), EntityId);
    D->SetNumberField(TEXT("health_before"), Before); D->SetNumberField(TEXT("health_after"), Health());
    D->SetNumberField(TEXT("amount"), Health() - Before); M->Emit(TEXT("combat.healed"), D);
    ForceNetUpdate(); return Health() - Before;
}
