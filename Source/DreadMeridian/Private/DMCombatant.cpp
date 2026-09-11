#include "DMCombatant.h"
#include "DMCombatPresentation.h"
#include "DMHealthAttributes.h"
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
    Primary = CreateDefaultSubobject<UDMPrimaryComponent>(TEXT("Primary"));
    Presentation = CreateDefaultSubobject<UDMCombatPresentation>(TEXT("Presentation"));
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
    GetCharacterMovement()->MaxWalkSpeed = ReplicatedMoveSpeed;
    if (IsDown() || IsRestrained()) { GetCharacterMovement()->StopMovementImmediately(); }
    else if (bHasMoveGoal && IsLocallyControlled())
    {
        const FVector Direction = (MoveGoal - GetActorLocation()).GetSafeNormal2D();
        if (FVector::DistSquared2D(MoveGoal, GetActorLocation()) > FMath::Square(35.f)) { AddMovementInput(Direction); }
        else { StopGoal(); }
    }
    if (GetNetMode() != NM_DedicatedServer)
    {
        // Health now reads from the overhead bar the HUD projects; the world label carries identity only.
        Label->SetText(FText::FromString(DisplayName()));
        Label->SetTextRenderColor(bIsEnemy ? FColor(255, 100, 80) : FColor(100, 220, 255));
        if (!bIsEnemy) { Label->SetTextRenderColor(Investigator->Color().ToFColor(true)); }
        if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
        { if (PC->PlayerCameraManager) { Label->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation() - Label->GetComponentLocation()).Rotation()); } }
        Presentation->UpdatePresentation();
    }
}

bool ADMCombatant::TryAttack(ADMCombatant* Target)
{
    if (!HasAuthority()) { return false; }
    AttackTarget = Target;
    ADMCombatGameMode* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Mode || !Mode->IsCombatActive() || IsDown() || !IsValid(Target) || Target->IsDown()
        || Target->bIsEnemy == bIsEnemy || NextAttackTick > Mode->GetCombatTick()
        || FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(GetAttackRange())) { bTelegraphActive = false; return false; }
    // Bot brain attack-channel gate: the target projection above still updates while held.
    if (bAttackHold) { bTelegraphActive = false; return false; }
    if (IsRestrained() || Primary->FrameTarget || Smuggler->IsCasting()) { return false; }
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
        || Target->bIsEnemy == bIsEnemy || Mode->GetCombatTick() < NextAttackTick
        || FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(GetAttackRange())) { return false; }
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(BasicAttack), false, this);
    // Bodies do not absorb basic attacks; world geometry still blocks the shot.
    for (ADMCombatant* Other : Mode->GetCombatants()) { Query.AddIgnoredActor(Other); }
    if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, Query)) { return false; }
    if (IsRestrained() || Primary->FrameTarget || Smuggler->IsCasting()) { return false; }
    NextAttackTick = Mode->GetCombatTick() + AttackIntervalTicks;
    return DealCombatDamage(Target, AttackDamage * Investigator->DamageMultiplier(Target->EntityId, Target->bSuppressed) * (bIsEnemy ? Smuggler->DamageMultiplier(*Mode, Target) : 1.f), TEXT("ability.basic_attack"), true);
}

bool ADMCombatant::DealCombatDamage(ADMCombatant* Target, float Damage, const FString& AbilityId, bool bBasic)
{
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !Mode || !Mode->IsCombatActive() || IsDown() || !IsValid(Target) || Target->IsDown()
        || Target->bIsEnemy == bIsEnemy || !FMath::IsFinite(Damage) || Damage <= 0) { return false; }
    const float Before = Target->Health();
    const float ShieldBefore = Target->Shield();
    const float ResolvedDamage = Damage * (1 - FMath::Clamp(Target->SpiritProtection, 0.f, .5f));
    const float Absorbed = FMath::Min(ShieldBefore, ResolvedDamage);
    if (Absorbed > 0) { Target->ApplyAttributeDelta(UDMHealthAttributes::GetShieldAttribute(), -Absorbed); }
    Target->ApplyAttributeDelta(UDMHealthAttributes::GetHealthAttribute(), -(ResolvedDamage - Absorbed));
    Target->Threat.FindOrAdd(EntityId) += Before - Target->Health();
    Target->LastDamageTick = Mode->GetCombatTick();
    Mode->NoteDamage(*this, *Target, ResolvedDamage);
    TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("actor_id"), EntityId);
    Data->SetStringField(TEXT("target_id"), Target->EntityId);
    Data->SetStringField(TEXT("ability_id"), AbilityId);
    Data->SetNumberField(TEXT("damage"), ResolvedDamage);
    Data->SetNumberField(TEXT("health_before"), Before);
    Data->SetNumberField(TEXT("health_after"), Target->Health());
    Data->SetNumberField(TEXT("shield_before"), ShieldBefore);
    Data->SetNumberField(TEXT("shield_after"), Target->Shield());
    Mode->Emit(TEXT("combat.damage"), Data);
    const bool bFinisher = bBasic && Investigator->OnHit(Target->EntityId, Mode->GetCombatTick());
    Target->Investigator->Pressure(Mode->GetCombatTick(), Before - Target->Health() + Absorbed);
    if (bBasic) { MulticastPresentation(0, Target->GetActorLocation() + FVector(0, 0, 15)); }
    Target->MulticastPresentation(2, Target->GetActorLocation() + FVector(0, 0, 15));
    RecordResources(TEXT("basic_hit")); Target->RecordResources(TEXT("incoming_pressure"));
    if (bFinisher && Target->bIsEnemy && Target->bCommonEnemy && !Target->IsDown())
    { Target->ApplyDisplacement((Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * 45); Target->NextAttackTick = FMath::Max(Target->NextAttackTick, Mode->GetCombatTick() + 3); }
    if (Target->IsDown())
    {
        Mode->NoteDowned(*Target);
        Target->Smuggler->Cancel(); Target->StopGoal(); Target->Primary->CancelChannel(); Target->Primary->ReleaseClinch();
        if (Target->HeldBy) { Target->HeldBy->Primary->ReleaseClinch(); }
        Target->bTelegraphActive = false;
        if (Target->bIsEnemy && Target->bHumanEnemy)
        { GetWorld()->SpawnActor<ADMScroungePickup>(Target->GetActorLocation() + FVector(0, 0, -65), FRotator::ZeroRotator); }
        for (ADMCombatant* Ally : Mode->GetCombatants())
        { Ally->Investigator->ThinPlace(Target->GetActorLocation(), Target->bIsEnemy ? 15 : 25); Ally->RecordResources(TEXT("thin_place")); }
        if (Target->bIsEnemy) { Target->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
        if (!Target->bIsEnemy)
        {
            if (Target->InjuryCount < 2) { ++Target->InjuryCount; } else { ++Target->GrievousCount; }
        }
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
    if (!HasAuthority() || !Mode || !Mode->IsCombatActive() || IsDown() || bIsEnemy || !Ally
        || Ally == this || Ally->bIsEnemy || !Ally->IsDown()
        || FVector::DistSquared(GetActorLocation(), Ally->GetActorLocation()) > FMath::Square(160.f)) { return false; }
    Ally->ApplyAttributeDelta(UDMHealthAttributes::GetHealthAttribute(), Ally->Attributes->GetMaxHealth() * .5f);
    for (ADMCombatant* Enemy : Mode->GetCombatants())
    {
        if (Enemy->bIsEnemy) { Enemy->Threat.FindOrAdd(Ally->EntityId) *= .25f; }
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
    DOREPLIFETIME(ADMCombatant, ReviveProgress);
    DOREPLIFETIME(ADMCombatant, AttackIntervalTicks);
    DOREPLIFETIME(ADMCombatant, NextAttackTick);
}

void ADMCombatant::InitializeInvestigator(EDMInvestigator Kind, bool bUseProfileTuning)
{
    Investigator->Initialize(Kind); bProfileRange = bUseProfileTuning;
    if (!bUseProfileTuning) { AttackDamage = Investigator->Damage(); AttackIntervalTicks = Investigator->Interval(); }
}
FString ADMCombatant::DisplayName() const
{ return bIsEnemy && Smuggler->Role != EDMSmuggler::None ? Smuggler->Name() : bIsEnemy ? Investigator->DisplayName() + TEXT(" ") + EntityId.Mid(EntityId.Find(TEXT(".")) + 1) : Investigator->DisplayName(); }
float ADMCombatant::GetAttackRange() const
{ return bProfileRange ? 420 : bIsEnemy && Smuggler->Role != EDMSmuggler::None ? Smuggler->Range() : Investigator->Range(); }
void ADMCombatant::StepInvestigator(int32 Tick)
{
    if (!HasAuthority()) { return; }
    Investigator->Step(Tick, !IsDown() && AttackTarget.IsValid() && !AttackTarget->IsDown() ? AttackTarget->EntityId : TEXT(""));
    RecordResources(TEXT("resource_step"));
    if (IsDown() || IsRestrained()) { bTelegraphActive = false; }
    if (Tick >= SlowUntilTick) { SlowFraction = 0; }
    ReplicatedMoveSpeed = (bIsEnemy && Smuggler->Role != EDMSmuggler::None ? Smuggler->Speed() : 420) * (1 + Investigator->Stickiness() * .25f) * (1 - FMath::Max(SlowFraction, SpiritSlow) * (1 - Investigator->Resistance()));
}
void ADMCombatant::SetAttackTarget(ADMCombatant* Target)
{ AttackTarget = Target; if (HasAuthority()) { AttackTargetId = Target ? Target->EntityId : FString(); } }
void ADMCombatant::SetAttackHold(bool bHold)
{ bAttackHold = bHold; if (bHold) { bTelegraphActive = false; } }
void ADMCombatant::SetReviveChannel(const FString& Reviver, float Progress)
{ if (HasAuthority()) { ReviverId = Reviver; ReviveProgress = FMath::Clamp(Progress, 0.f, 1.f); } }
void ADMCombatant::ApplySlow(float Fraction, int32 UntilTick)
{ if (HasAuthority()) { SlowFraction = FMath::Clamp(Fraction, 0.f, 1.f); SlowUntilTick = UntilTick; } }
void ADMCombatant::ApplyDisplacement(FVector Delta)
{ if (HasAuthority() && !IsDown()) { SetActorLocation(GetActorLocation() + Delta * (1 - Investigator->Resistance()), true); } }
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
