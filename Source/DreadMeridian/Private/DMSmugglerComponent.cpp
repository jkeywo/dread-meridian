#include "DMSmugglerComponent.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMAbilityMarker.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UDMSmugglerComponent::UDMSmugglerComponent() { SetIsReplicatedByDefault(true); }
ADMCombatant* UDMSmugglerComponent::Self() const { return CastChecked<ADMCombatant>(GetOwner()); }
FString UDMSmugglerComponent::Name() const
{
    switch (Role) {
    case EDMSmuggler::Gunman: return TEXT("Smuggler Gunman");
    case EDMSmuggler::Bruiser: return TEXT("Smuggler Bruiser");
    case EDMSmuggler::Lookout: return TEXT("Smuggler Lookout");
    case EDMSmuggler::Bomber: return TEXT("Smuggler Bomber");
    case EDMSmuggler::GangBoss: return TEXT("Gang Boss");
    default: return TEXT("Enemy"); }
}
float UDMSmugglerComponent::Range() const { return Role == EDMSmuggler::Bruiser ? 155 : Role == EDMSmuggler::Lookout ? 600 : Role == EDMSmuggler::Bomber ? 550 : 700; }
float UDMSmugglerComponent::BaseHealth() const { return Role == EDMSmuggler::GangBoss ? 750 : Role == EDMSmuggler::Bruiser ? 260 : Role == EDMSmuggler::Gunman ? 180 : 150; }
float UDMSmugglerComponent::BaseDamage() const { return Role == EDMSmuggler::GangBoss ? 9 : Role == EDMSmuggler::Bruiser ? 10 : Role == EDMSmuggler::Gunman ? 7 : 5; }
int32 UDMSmugglerComponent::Interval() const { return Role == EDMSmuggler::GangBoss ? 9 : Role == EDMSmuggler::Bruiser ? 12 : 16; }
float UDMSmugglerComponent::Speed() const { return Role == EDMSmuggler::GangBoss ? 320 : Role == EDMSmuggler::Bruiser ? 330 : 285; }
void UDMSmugglerComponent::Initialize(EDMSmuggler NewRole)
{
    check(Self()->HasAuthority()); Cancel(); Role = NewRole;
    Self()->bCommonEnemy = Role != EDMSmuggler::GangBoss;
    Self()->bHumanEnemy = true; Self()->AttackDamage = BaseDamage(); Self()->AttackIntervalTicks = Interval();
    Self()->ForceNetUpdate();
}
FString UDMSmugglerComponent::Status() const
{
    if (Self()->IsDown()) { return TEXT("Defeated"); }
    if (Self()->IsRestrained()) { return TEXT("Restrained - signature interrupted"); }
    switch (Role) {
    case EDMSmuggler::Gunman: return bSetPosition ? TEXT("SET POSITION | +50% basic damage") : TEXT("Hold Position | disrupt his footing");
    case EDMSmuggler::Bruiser: return IsCasting() ? TEXT("SHOVE INCOMING | step back") : TEXT("Bodyguard Shove | protects the gunline");
    case EDMSmuggler::Lookout: return OrderTarget ? TEXT("MARKED: ") + OrderTarget->DisplayName() : TEXT("Spotter's Mark | calls a target");
    case EDMSmuggler::Bomber: return IsCasting() ? TEXT("FIREBOMB | leave the red circle") : TEXT("Firebomb | lingering area denial");
    case EDMSmuggler::GangBoss: return OrderTarget ? TEXT("FOCUS FIRE: ") + OrderTarget->DisplayName() : TEXT("Focus Fire | mobile commander");
    default: return TEXT(""); }
}
void UDMSmugglerComponent::ClearMarker() { if (IsValid(Marker)) { Marker->Destroy(); } Marker = nullptr; }
void UDMSmugglerComponent::Cancel()
{
    if (!Self()->HasAuthority()) { return; }
    ClearMarker(); OrderTarget = nullptr; PendingTarget = nullptr; OrderUntil = 0;
    ResolveTick = 0; FireUntil = 0; bSetPosition = false; StationarySince = -1;
}
void UDMSmugglerComponent::EndPlay(const EEndPlayReason::Type Reason) { Cancel(); Super::EndPlay(Reason); }
bool UDMSmugglerComponent::Sight(FVector From, FVector To) const
{
    FCollisionQueryParams Q(SCENE_QUERY_STAT(SmugglerSight), false, Self());
    if (auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    { for (ADMCombatant* A : Mode->GetCombatants()) { Q.AddIgnoredActor(A); } }
    FHitResult Hit; return !GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Q);
}
void UDMSmugglerComponent::Record(ADMCombatGameMode& Mode, const FString& Stage, ADMCombatant* Target)
{
    auto Data = MakeShared<FJsonObject>(); Data->SetStringField(TEXT("actor_id"), Self()->EntityId);
    Data->SetStringField(TEXT("enemy_role"), StaticEnum<EDMSmuggler>()->GetNameStringByValue(static_cast<int64>(Role)));
    Data->SetStringField(TEXT("stage"), Stage);
    if (Target) { Data->SetStringField(TEXT("target_id"), Target->EntityId); }
    Mode.Emit(TEXT("smuggler.signature"), Data);
}
ADMCombatant* UDMSmugglerComponent::FocusTarget(const ADMCombatGameMode& Mode, const ADMCombatant* Recipient, bool bBossOnly)
{
    if (!Recipient || !Recipient->bIsEnemy) { return nullptr; }
    // Stable roster order resolves simultaneous calls; a boss command always outranks a lookout mark.
    for (ADMCombatant* A : Mode.GetCombatants())
    {
        const auto* Kit = A->Smuggler.Get();
        if (!A->bIsEnemy || A->IsDown() || A->IsRestrained() || Kit->Role != (bBossOnly ? EDMSmuggler::GangBoss : EDMSmuggler::Lookout)
            || !IsValid(Kit->OrderTarget) || Kit->OrderTarget->IsDown() || Kit->OrderUntil <= Mode.GetCombatTick()
            || FVector::DistSquared2D(A->GetActorLocation(), Recipient->GetActorLocation()) > FMath::Square(900.f)) { continue; }
        return Kit->OrderTarget;
    }
    return nullptr;
}
float UDMSmugglerComponent::DamageMultiplier(const ADMCombatGameMode& Mode, const ADMCombatant* Target) const
{
    return (bSetPosition ? 1.5f : 1.f) * (FocusTarget(Mode, Self(), false) == Target ? 1.2f : 1.f);
}
ADMCombatant* UDMSmugglerComponent::DiverTarget(const ADMCombatGameMode& Mode, const ADMCombatant* Bodyguard)
{
    ADMCombatant* Best = nullptr; float Nearest = MAX_flt;
    for (ADMCombatant* Diver : Mode.GetCombatants())
    {
        if (Diver->bIsEnemy || Diver->IsDown()) { continue; }
        for (ADMCombatant* Ally : Mode.GetCombatants())
        {
            if (Ally == Bodyguard || !Ally->bIsEnemy || Ally->IsDown() || !Ally->Smuggler->IsRanged()
                || FVector::DistSquared2D(Ally->GetActorLocation(), Bodyguard->GetActorLocation()) > FMath::Square(500.f)) { continue; }
            const float Distance = FVector::DistSquared2D(Diver->GetActorLocation(), Ally->GetActorLocation());
            if (Distance < FMath::Square(240.f) && Distance < Nearest) { Best = Diver; Nearest = Distance; }
        }
    }
    return Best;
}
float UDMSmugglerComponent::SignatureRange() const
{ return Role == EDMSmuggler::Bruiser ? 200 : Role == EDMSmuggler::Bomber ? 650 : Role == EDMSmuggler::Lookout || Role == EDMSmuggler::GangBoss ? 900 : 0; }
int32 UDMSmugglerComponent::SignatureCooldownTicks() const
{ return Role == EDMSmuggler::Bruiser ? 65 : Role == EDMSmuggler::Bomber ? 90 : Role == EDMSmuggler::Lookout ? 85 : Role == EDMSmuggler::GangBoss ? 95 : 0; }
int32 UDMSmugglerComponent::SignatureCastDelayTicks() const
{ return Role == EDMSmuggler::Bruiser ? 6 : Role == EDMSmuggler::Bomber ? 12 : 0; }
bool UDMSmugglerComponent::CanSignature(const ADMCombatGameMode& Mode, const ADMCombatant* Target, const TCHAR** Reason) const
{
    const int32 Tick = Mode.GetCombatTick(); const ADMCombatant* A = Self();
    const TCHAR* Why = nullptr;
    if (!A->HasAuthority()) { Why = TEXT("authority"); }
    else if (!Mode.IsCombatActive()) { Why = TEXT("inactive"); }
    else if (!A->bIsEnemy || A->IsDown() || A->IsRestrained()) { Why = TEXT("state"); }
    else if (!IsValid(Target) || Target->bIsEnemy || Target->IsDown()) { Why = TEXT("target"); }
    else if (IsCasting()) { Why = TEXT("casting"); }
    else if (Tick < NextSignatureTick) { Why = TEXT("cooldown"); }
    else if (FireUntil > Tick) { Why = TEXT("burning"); }
    else if (!HasSignature()) { Why = TEXT("no_signature"); }
    else if (FVector::Dist2D(A->GetActorLocation(), Target->GetActorLocation()) > SignatureRange()) { Why = TEXT("out_of_range"); }
    else if (!Sight(A->GetActorLocation(), Target->GetActorLocation())) { Why = TEXT("no_sight"); }
    if (Reason) { *Reason = Why; }
    return Why == nullptr;
}
bool UDMSmugglerComponent::TrySignature(ADMCombatGameMode& Mode, ADMCombatant* Target)
{
    const int32 Tick = Mode.GetCombatTick(); auto* A = Self();
    if (!CanSignature(Mode, Target)) { return false; }
    if (Role == EDMSmuggler::Bruiser)
    { PendingTarget = Target; ResolveTick = Tick + 6; NextSignatureTick = Tick + 65; A->MulticastPresentation(5, Target->GetActorLocation()); }
    else if (Role == EDMSmuggler::Bomber)
    {
        BlastPoint = Target->GetActorLocation(); PendingTarget = Target;
        ResolveTick = Tick + 12; NextSignatureTick = Tick + 90;
        ClearMarker(); Marker = GetWorld()->SpawnActor<ADMAbilityMarker>(BlastPoint - FVector(0,0,70), FRotator::ZeroRotator);
        Marker->bHostile = true; Marker->Radius = 180; Marker->CustomLabel = TEXT("FIREBOMB - MOVE");
        A->MulticastPresentation(6, BlastPoint);
    }
    else if (Role == EDMSmuggler::Lookout || Role == EDMSmuggler::GangBoss)
    {
        OrderTarget = Target; OrderUntil = Tick + (Role == EDMSmuggler::GangBoss ? 45 : 55);
        NextSignatureTick = Tick + (Role == EDMSmuggler::GangBoss ? 95 : 85);
        ClearMarker(); Marker = GetWorld()->SpawnActor<ADMAbilityMarker>(Target->GetActorLocation() - FVector(0,0,70), FRotator::ZeroRotator);
        Marker->bHostile = true; Marker->BoundTarget = Target; Marker->Radius = Role == EDMSmuggler::GangBoss ? 75 : 55;
        Marker->CustomLabel = Role == EDMSmuggler::GangBoss ? TEXT("FOCUS FIRE") : TEXT("MARKED");
        A->MulticastPresentation(7, Target->GetActorLocation());
    }
    else { return false; }
    if (IsCasting()) { A->StopGoal(); A->GetCharacterMovement()->StopMovementImmediately(); A->bTelegraphActive = false; }
    Record(Mode, TEXT("activated"), Target); Mode.NoteSignature(); A->ForceNetUpdate(); return true;
}
void UDMSmugglerComponent::Step(ADMCombatGameMode& Mode)
{
    auto* A = Self(); const int32 Tick = Mode.GetCombatTick();
    if (!A->HasAuthority() || Role == EDMSmuggler::None) { return; }
    if (!Mode.IsCombatActive() || A->IsDown() || A->IsRestrained())
    { if (IsCasting() || OrderTarget || Marker) { Record(Mode, TEXT("interrupted")); } Cancel(); return; }
    if (Role == EDMSmuggler::Gunman)
    {
        const bool bSteady = A->GetAttackTarget() && !A->GetAttackTarget()->IsDown() && A->GetVelocity().Size2D() < 15
            && FVector::DistSquared2D(PositionAnchor, A->GetActorLocation()) < FMath::Square(20.f);
        if (!bSteady) { StationarySince = Tick; PositionAnchor = A->GetActorLocation(); }
        const bool bWasSet = bSetPosition;
        bSetPosition = bSteady && StationarySince >= 0 && Tick - StationarySince >= 20;
        if (bWasSet != bSetPosition) { Record(Mode, bSetPosition ? TEXT("position_set") : TEXT("position_broken")); }
    }
    if (OrderTarget && (OrderTarget->IsDown() || Tick >= OrderUntil || FVector::DistSquared2D(A->GetActorLocation(), OrderTarget->GetActorLocation()) > FMath::Square(1100.f)))
    { OrderTarget = nullptr; OrderUntil = 0; ClearMarker(); Record(Mode, TEXT("expired")); }
    if (ResolveTick > 0 && Tick >= ResolveTick)
    {
        ResolveTick = 0;
        if (Role == EDMSmuggler::Bruiser)
        {
            if (IsValid(PendingTarget) && !PendingTarget->IsDown() && FVector::DistSquared2D(A->GetActorLocation(), PendingTarget->GetActorLocation()) <= FMath::Square(220.f)
                && Sight(A->GetActorLocation(), PendingTarget->GetActorLocation()))
            {
                A->DealCombatDamage(PendingTarget, 8, TEXT("ability.smuggler.bodyguard_shove"));
                PendingTarget->ApplyDisplacement((PendingTarget->GetActorLocation() - A->GetActorLocation()).GetSafeNormal2D() * 230);
                A->MulticastPresentation(8, PendingTarget->GetActorLocation());
            }
        }
        else if (Role == EDMSmuggler::Bomber)
        {
            FireUntil = Tick + 40; NextFireTick = Tick + 10;
            if (Marker) { Marker->CustomLabel = TEXT("BURNING GROUND"); Marker->ForceNetUpdate(); }
            A->MulticastPresentation(4, BlastPoint - FVector(0,0,60));
            for (ADMCombatant* T : Mode.GetCombatants())
            { if (!T->bIsEnemy && !T->IsDown() && FVector::DistSquared2D(T->GetActorLocation(), BlastPoint) <= FMath::Square(180.f)
                && Sight(BlastPoint, T->GetActorLocation())) { A->DealCombatDamage(T, 14, TEXT("ability.smuggler.firebomb")); } }
        }
        Record(Mode, TEXT("resolved"), PendingTarget); PendingTarget = nullptr;
    }
    if (FireUntil > 0)
    {
        if (Tick >= FireUntil) { FireUntil = 0; ClearMarker(); }
        else if (Tick >= NextFireTick)
        {
            NextFireTick = Tick + 10;
            for (ADMCombatant* T : Mode.GetCombatants())
            { if (!T->bIsEnemy && !T->IsDown() && FVector::DistSquared2D(T->GetActorLocation(), BlastPoint) <= FMath::Square(180.f)
                && Sight(BlastPoint, T->GetActorLocation())) { A->DealCombatDamage(T, 4, TEXT("ability.smuggler.burning_ground")); } }
        }
    }
}
void UDMSmugglerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDMSmugglerComponent, Role); DOREPLIFETIME(UDMSmugglerComponent, bSetPosition);
    DOREPLIFETIME(UDMSmugglerComponent, OrderTarget); DOREPLIFETIME(UDMSmugglerComponent, OrderUntil);
    DOREPLIFETIME(UDMSmugglerComponent, ResolveTick); DOREPLIFETIME(UDMSmugglerComponent, NextSignatureTick);
}
