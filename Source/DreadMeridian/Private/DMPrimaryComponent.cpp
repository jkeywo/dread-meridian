#include "DMPrimaryComponent.h"
#include "DMPrimaryAbility.h"
#include "DMAbilityMarker.h"
#include "DMCombatant.h"
#include "DMKitComponent.h"
#include "DMCombatGameMode.h"
#include "DMEncounterLayout.h"
#include "DMScroungePickup.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

UDMPrimaryComponent::UDMPrimaryComponent() { SetIsReplicatedByDefault(true); }
ADMCombatant* UDMPrimaryComponent::Self() const { return CastChecked<ADMCombatant>(GetOwner()); }
int32 UDMPrimaryComponent::Now() const
{ auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); return M ? M->GetCombatTick() : 0; }
void UDMPrimaryComponent::Initialize()
{ if (Self()->HasAuthority()) { AbilityHandle = Self()->GetAbilitySystemComponent()->GiveAbility(FGameplayAbilitySpec(UDMPrimaryAbility::StaticClass(), 1)); } }
void UDMPrimaryComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Self()->HasAuthority())
    { ReleaseClinch(); for (ADMAbilityMarker* M : Satchels) { if (IsValid(M)) { M->Destroy(); } } for (ADMAbilityMarker* M : Bindings) { if (IsValid(M)) { M->Destroy(); } } }
    Super::EndPlay(Reason);
}
float UDMPrimaryComponent::Range() const
{ return Self()->Investigator->Kind == EDMInvestigator::Smuggler ? (HeldTarget ? 650 : 180) : Self()->Investigator->Kind == EDMInvestigator::Photographer ? 850 : 650; }
FString UDMPrimaryComponent::Name() const
{
    switch (Self()->Investigator->Kind) {
    case EDMInvestigator::Sapper: return TEXT("Satchel Charge");
    case EDMInvestigator::Photographer: return TEXT("Frame the Subject");
    case EDMInvestigator::Medium: return TEXT("Bind Spirit");
    case EDMInvestigator::Smuggler: return HeldTarget ? TEXT("Throw") : TEXT("Clinch");
    default: return TEXT(""); }
}
FString UDMPrimaryComponent::Status() const
{
    if (FrameTarget) { return TEXT("Framing - move to cancel"); }
    if (HeldTarget) { return TEXT("Holding - Q to aim throw"); }
    if (Cooldown > 0) { return FString::Printf(TEXT("%s %.1fs"), *Name(), Cooldown); }
    if (Self()->Investigator->Kind == EDMInvestigator::Sapper) { return FString::Printf(TEXT("%d satchels | proximity armed | F / Y detonate"), Satchels.Num()); }
    return Name() + TEXT(" ready");
}
bool UDMPrimaryComponent::Sight(FVector Point, ADMCombatant* Target) const
{
    FCollisionQueryParams Q(SCENE_QUERY_STAT(PrimarySight), false, Self());
    for (TActorIterator<ADMCombatant> It(GetWorld()); It; ++It) { Q.AddIgnoredActor(*It); }
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, Self()->GetActorLocation(), Point, ECC_Visibility, Q);
}
bool UDMPrimaryComponent::Ground(FVector Point, FVector& Out) const
{
    if (Point.ContainsNaN() || FMath::Abs(Point.X) > DMEncounterLayout::PlayableX || FMath::Abs(Point.Y) > DMEncounterLayout::PlayableY) { return false; }
    FCollisionQueryParams Q(SCENE_QUERY_STAT(PrimaryGround), false, Self());
    for (TActorIterator<ADMCombatant> It(GetWorld()); It; ++It) { Q.AddIgnoredActor(*It); }
    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Point + FVector(0, 0, 500), Point - FVector(0, 0, 1000), ECC_Visibility, Q) || Hit.ImpactNormal.Z < .8f) { return false; }
    Out = Hit.ImpactPoint + FVector(0, 0, 18); return Sight(Out + FVector(0, 0, 40));
}
FString UDMPrimaryComponent::Validate(ADMCombatant* Target, FVector Point, bool bDetonate) const
{
    auto* Actor = Self();
    if (!Actor->Injuries->CanCast()) { return TEXT("Concussion: pause before casting again"); }
    if (Actor->IsDown() || (Actor->IsRestrained() || Actor->IsStunned()) || Actor->bIsEnemy) { return TEXT("Cannot cast in this state"); }
    if (Point.ContainsNaN()) { return TEXT("Invalid aim point"); }
    if (Target && (!IsValid(Target) || Target->GetWorld() != GetWorld())) { return TEXT("Invalid target"); }
    const auto Kind = Actor->Investigator->Kind;
    if (bDetonate)
    { return Kind == EDMInvestigator::Sapper && Satchels.ContainsByPredicate([&](const ADMAbilityMarker* M) { return IsValid(M) && M->ArmedTick <= Now(); }) ? TEXT("") : TEXT("No armed satchels"); }
    if (!HeldTarget && (Cooldown > 0 || (Actor->HasAuthority() && NextCastTick > Now()))) { return TEXT("Q is cooling down"); }
    if (Kind == EDMInvestigator::Photographer && FrameTarget) { return TEXT("Already framing - move to cancel"); }
    if (Kind == EDMInvestigator::Smuggler && HeldTarget)
    { return FVector::DistSquared2D(Point, Actor->GetActorLocation()) > 25 ? TEXT("") : TEXT("Choose a throw direction"); }
    if (Kind == EDMInvestigator::Photographer || Kind == EDMInvestigator::Smuggler)
    {
        if (!Target || !Target->bIsEnemy || Target->IsDown()) { return TEXT("Choose a living enemy"); }
        // Drowned Man Walking does not waive the Break layer (GDD 4.4 is LOCKED); it lets the grab land on an
        // unbroken elite as pure Break pressure instead, so the R still has an elite use without stun-locking one.
        if (Kind == EDMInvestigator::Smuggler && Target->IsRestrained()) { return TEXT("Target must be common or Break-vulnerable"); }
        if (Kind == EDMInvestigator::Smuggler && !Target->bCommonEnemy && !Target->bBreakVulnerable && !Actor->Kit->IsRActive())
        { return TEXT("Target must be common or Break-vulnerable"); }
    }
    if (Target && Target->IsDown() && Kind != EDMInvestigator::Medium) { return TEXT("Target is down"); }
    const FVector Aim = Target ? Target->GetActorLocation() : Point;
    if (FVector::DistSquared2D(Actor->GetActorLocation(), Aim) > FMath::Square(Range())) { return TEXT("Out of Q range"); }
    if (!Sight(Aim + (Target ? FVector::ZeroVector : FVector(0, 0, 40)), Target)) { return TEXT("Line of sight blocked"); }
    FVector Floor;
    if ((Kind == EDMInvestigator::Sapper || (Kind == EDMInvestigator::Medium && !Target)) && !Ground(Point, Floor)) { return TEXT("Choose clear ground in the arena"); }
    if (Kind == EDMInvestigator::Sapper && Actor->Investigator->Charges < 1) { return TEXT("No Prepared Charges - collect components"); }
    return Kind == EDMInvestigator::None ? TEXT("No Q available") : TEXT("");
}
bool UDMPrimaryComponent::Request(ADMCombatant* Target, FVector Point, bool bDetonate)
{
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Self()->HasAuthority() || !Mode || !Mode->IsCombatActive()) { LastFailure = TEXT("Combat is not active"); return false; }
    LastFailure = Validate(Target, Point, bDetonate);
    if (!LastFailure.IsEmpty()) { return false; }
    RequestedTarget = Target; RequestedPoint = Point; bRequestedDetonate = bDetonate; bResolved = false;
    Self()->GetAbilitySystemComponent()->TryActivateAbility(AbilityHandle);
    return bResolved;
}
bool UDMPrimaryComponent::Resolve()
{
    auto* Actor = Self(); auto* Target = RequestedTarget.Get();
    auto* ActiveMode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Actor->HasAuthority() || !ActiveMode || !ActiveMode->IsCombatActive()) { return false; }
    LastFailure = Validate(Target, RequestedPoint, bRequestedDetonate);
    if (!LastFailure.IsEmpty()) { return false; }
    auto* R = Actor->Investigator.Get();
    if (bRequestedDetonate)
    {
        for (int32 I = Satchels.Num() - 1; I >= 0; --I) { DetonateSatchel(I); }
    }
    else if (R->Kind == EDMInvestigator::Sapper)
    {
        FVector Floor; if (!Ground(RequestedPoint, Floor)) { return false; }
        auto* Charge = GetWorld()->SpawnActor<ADMAbilityMarker>(Floor, FRotator::ZeroRotator);
        if (!Charge) { LastFailure = TEXT("Placement failed"); return false; }
        Charge->SetOwner(Actor); Charge->ArmedTick = Now() + 5; Charge->Radius = SatchelRadius; Charge->Serial = ++ChargeSerial;
        Satchels.Add(Charge); --R->Charges; NextCastTick = Now() + 8; Emit(TEXT("place_satchel"));
    }
    else if (R->Kind == EDMInvestigator::Photographer)
    {
        FrameTarget = Target; FrameEndTick = Now() + 30;
        Actor->StopGoal(); Actor->GetCharacterMovement()->StopMovementImmediately(); Emit(TEXT("frame_started"), Target);
    }
    else if (R->Kind == EDMInvestigator::Medium)
    {
        FVector Location = Target ? Target->GetActorLocation() - FVector(0, 0, 70) : RequestedPoint;
        if (!Target && !Ground(RequestedPoint, Location)) { return false; }
        auto* Spirit = GetWorld()->SpawnActor<ADMAbilityMarker>(Location, FRotator::ZeroRotator);
        if (!Spirit) { LastFailure = TEXT("Binding failed"); return false; }
        if (Bindings.Num() >= 3)
        {
            const FString Old = Bindings[0]->SpiritId;
            const int32 I = R->Spirits.IndexOfByPredicate([&](const auto& S) { return S.Id == Old; });
            if (I != INDEX_NONE) { R->Spirits.RemoveAt(I); R->SpiritTargets.RemoveAt(I); }
            Bindings[0]->Destroy(); Bindings.RemoveAt(0);
        }
        Spirit->SetOwner(Actor); Spirit->bSpirit = true; Spirit->BoundTarget = Target; Spirit->Radius = SpiritRadius;
        Spirit->SpiritId = FString::Printf(TEXT("spirit.%d"), ++SpiritSerial); Bindings.Add(Spirit);
        R->BindSpirit(Spirit->SpiritId, Target ? Target->EntityId : TEXT(""), Location);
        NextCastTick = Now() + 25; Emit(TEXT("bind_spirit"), Target);
    }
    else if (R->Kind == EDMInvestigator::Smuggler)
    {
        const bool bDrowned = Actor->Kit->IsRActive();
        if (HeldTarget)
        {
            auto* Victim = HeldTarget.Get();
            const FVector Direction = (RequestedPoint - Actor->GetActorLocation()).GetSafeNormal2D();
            ReleaseClinch(); FDMControl Control; Control.Displacement = Direction * (bDrowned ? 450 : 300);
            Control.StaggerTicks = 5; Victim->ApplyControl(Control, Actor, TEXT("ability.q.throw"));
            R->Pressure(Now(), 10); Emit(TEXT("throw"), Victim);
            Actor->MulticastAttackFX(Actor->GetActorLocation(), Victim->GetActorLocation(), R->Color(), 4);
        }
        else if (bDrowned && !Target->bCommonEnemy && !Target->bBreakVulnerable)
        {
            // An unbroken elite cannot be held, but the grab still counts against its Resolve.
            FDMControl Control;
            Control.StaggerTicks = 5; Control.BreakPressure = 40;
            Target->ApplyControl(Control, Actor, TEXT("ability.q.clinch"));
            NextCastTick = Now() + 20; R->Pressure(Now(), 10); Emit(TEXT("clinch_break"), Target);
        }
        else
        {
            HeldTarget = Target; Target->HeldBy = Actor; Target->StopGoal();
            Target->GetCharacterMovement()->StopMovementImmediately(); Target->InterruptControl(); Actor->StopGoal();
            HoldEndTick = Now() + (bDrowned ? 25 : 15);
            if (!Target->bCommonEnemy) { HoldEndTick = FMath::Min(HoldEndTick, Target->BrokenUntilTick); }
            R->Pressure(Now(), 10); Emit(TEXT("clinch"), Target);
        }
    }
    if (!bRequestedDetonate) { ActiveMode->NoteQCast(); Actor->MulticastPresentation(R->Kind == EDMInvestigator::Smuggler && !HeldTarget ? 3 : 1, Target ? Target->GetActorLocation() : RequestedPoint); }
    Actor->RecordResources(TEXT("q")); Cooldown = FMath::Max(0, NextCastTick - Now()) * .1f; Actor->ForceNetUpdate();
    Actor->Injuries->OnCast();
    if (!bRequestedDetonate) { Actor->MadnessCore->ScheduleEcho(0, Target, RequestedPoint); }
    bResolved = true; return true;
}
bool UDMPrimaryComponent::SatchelCanHit(const ADMAbilityMarker* Charge, ADMCombatant* Enemy) const
{
    if (!IsValid(Charge) || !IsValid(Enemy) || !Enemy->bIsEnemy || Enemy->IsDown()
        || FVector::DistSquared(Charge->GetActorLocation() + FVector(0, 0, 70), Enemy->GetActorLocation()) > FMath::Square(SatchelRadius)) { return false; }
    const auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Mode) { return false; }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SatchelBlast), false, Self());
    for (ADMCombatant* Other : Mode->GetCombatants()) { Query.AddIgnoredActor(Other); }
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, Charge->GetActorLocation() + FVector(0, 0, 50), Enemy->GetActorLocation(), ECC_Visibility, Query);
}
bool UDMPrimaryComponent::DetonateSatchel(int32 Index)
{
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Self()->HasAuthority() || !Mode || !Mode->IsCombatActive() || !Satchels.IsValidIndex(Index)) { return false; }
    auto* Charge = Satchels[Index].Get();
    if (!IsValid(Charge) || Charge->ArmedTick > Now()) { return false; }
    const FVector Center = Charge->GetActorLocation();
    // Detonating by hand is this trap's resolution, so any Dead Ground tags it holds must not fire again later.
    Self()->Kit->DropTrap(FDMDeadGroundLedger::Satchel, Charge->Serial);
    Emit(TEXT("detonate"));
    for (ADMCombatant* Enemy : Mode->GetCombatants())
    { if (SatchelCanHit(Charge, Enemy)) { Self()->DealCombatDamage(Enemy, 55, TEXT("ability.q.satchel")); } }
    Self()->MulticastAttackFX(Center, Center + FVector(0, 0, 60), Self()->Investigator->Color(), 4);
    Self()->MulticastPresentation(4, Center);
    Charge->Destroy(); Satchels.RemoveAt(Index); Self()->ForceNetUpdate();
    return true;
}
void UDMPrimaryComponent::ConsumeSatchels(const TSet<int32>& Serials)
{
    if (!Self()->HasAuthority() || Serials.IsEmpty()) { return; }
    for (int32 I = Satchels.Num() - 1; I >= 0; --I)
    {
        ADMAbilityMarker* Charge = Satchels[I].Get();
        if (!IsValid(Charge)) { Satchels.RemoveAt(I); continue; }
        if (!Serials.Contains(Charge->Serial)) { continue; }
        Self()->MulticastPresentation(4, Charge->GetActorLocation());
        Charge->Destroy(); Satchels.RemoveAt(I);
    }
    Self()->ForceNetUpdate();
}
void UDMPrimaryComponent::CancelChannel()
{
    if (!Self()->HasAuthority() || !FrameTarget) { return; }
    Emit(TEXT("frame_ended"), FrameTarget); FrameTarget = nullptr; NextCastTick = Now() + 20;
    Cooldown = 2; Self()->ForceNetUpdate();
}
void UDMPrimaryComponent::ReleaseClinch()
{
    if (!Self()->HasAuthority() || !HeldTarget) { return; }
    if (IsValid(HeldTarget) && HeldTarget->HeldBy == Self()) { HeldTarget->HeldBy = nullptr; HeldTarget->ForceNetUpdate(); }
    HeldTarget = nullptr; NextCastTick = Now() + 40; Cooldown = 4; Self()->ForceNetUpdate();
}
void UDMPrimaryComponent::Step(int32 Tick)
{
    if (!Self()->HasAuthority()) { return; }
    auto* Actor = Self(); auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    Cooldown = FMath::Max(0, NextCastTick - Tick) * .1f;
    if (HeldTarget && (Actor->IsDown() || HeldTarget->IsDown() || Tick >= HoldEndTick || FVector::DistSquared2D(Actor->GetActorLocation(), HeldTarget->GetActorLocation()) > FMath::Square(240.f))) { ReleaseClinch(); }
    if (FrameTarget)
    {
        if (Actor->IsDown() || FrameTarget->IsDown() || (Actor->IsRestrained() || Actor->IsStunned()) || Tick >= FrameEndTick || Actor->GetVelocity().SizeSquared2D() > 100
            || FVector::DistSquared2D(Actor->GetActorLocation(), FrameTarget->GetActorLocation()) > FMath::Square(Range()) || !Sight(FrameTarget->GetActorLocation(), FrameTarget)) { CancelChannel(); }
        else
        {
            Actor->Investigator->AddExposure(FrameTarget->EntityId, 2, FrameTarget->bTelegraphActive, Tick);
            if (Tick % 5 == 0) { Actor->MulticastAttackFX(Actor->GetActorLocation(), FrameTarget->GetActorLocation(), Actor->Investigator->Color(), 2); }
            Actor->RecordResources(TEXT("frame"));
        }
    }
    if (Actor->IsDown() || !Mode || !Mode->IsCombatActive()) { return; }
    // Persistent charges trigger on the authority regardless of player/bot control.
    const bool bDeferring = Actor->Kit->IsDeadGroundActive();
    for (int32 I = Satchels.Num() - 1; I >= 0; --I)
    {
        const auto* Charge = Satchels[I].Get();
        if (!IsValid(Charge) || Charge->ArmedTick > Tick) { continue; }
        if (bDeferring)
        {
            // Dead Ground records the triggers that would have happened instead of resolving them. A satchel is an
            // area trap, so every enemy in its blast is tagged; the charge stays armed until the batch resolves.
            for (ADMCombatant* Enemy : Mode->GetCombatants())
            { if (SatchelCanHit(Charge, Enemy)) { Actor->Kit->TagTrap(FDMDeadGroundLedger::Satchel, Charge->Serial, Enemy); } }
            continue;
        }
        for (ADMCombatant* Enemy : Mode->GetCombatants())
        { if (SatchelCanHit(Charge, Enemy)) { DetonateSatchel(I); break; } }
    }
    // Open Seance fully manifests the bound spirits: their passive presence intensifies and a spirit crossing the
    // field on a Beckon stays active on the way, instead of going quiet until it lands.
    const bool bManifest = Actor->Kit->IsRActive() && Actor->Investigator->Kind == EDMInvestigator::Medium;
    for (ADMAbilityMarker* Spirit : Bindings)
    {
        if (!IsValid(Spirit)) { continue; }
        if (Spirit->bTravelling && !bManifest) { continue; }
        auto* Bound = Spirit->BoundTarget.Get();
        // The marker anchors itself to BoundTarget every frame in its own Tick(). Writing the position again
        // here at combat-tick rate fought that, snapping the spirit off its drift ten times a second.
        if (!Bound) { Actor->Investigator->UpdateSpiritLocationById(Spirit->SpiritId, Spirit->GetActorLocation()); }
        if (Bound && Bound->IsDown() && !Bound->bIsEnemy && Tick % 10 == 0) { Actor->Investigator->ThinPlace(Bound->GetActorLocation(), 2); }
        const auto* Resource = Actor->Investigator->Spirits.FindByPredicate([&](const auto& S) { return S.Id == Spirit->SpiritId; });
        Spirit->Attention = Resource ? Resource->Value : 0;
        const float Strength = (.1f + Spirit->Attention * .002f) * (bManifest ? 2.f : 1.f);
        if (Bound && Bound->bIsEnemy && !Bound->IsDown()) { Bound->SpiritSlow = FMath::Max(Bound->SpiritSlow, Strength); }
        else if (!Bound || !Bound->bIsEnemy)
        {
            for (ADMCombatant* Ally : Mode->GetCombatants())
            { if (!Ally->bIsEnemy && !Ally->IsDown() && (Bound == Ally || (!Bound && FVector::DistSquared2D(Spirit->GetActorLocation(), Ally->GetActorLocation()) <= FMath::Square(SpiritRadius))))
                { Ally->SpiritProtection = FMath::Max(Ally->SpiritProtection, Strength); } }
        }
    }
}
ADMScroungePickup* UDMPrimaryComponent::NearestPickup(float MaxDistance) const
{
    ADMScroungePickup* Best = nullptr; float Distance = FMath::Square(MaxDistance);
    for (TActorIterator<ADMScroungePickup> It(GetWorld()); It; ++It)
    { const float D = FVector::DistSquared2D(Self()->GetActorLocation(), It->GetActorLocation()); if (D < Distance) { Best = *It; Distance = D; } }
    return Best;
}
int32 UDMPrimaryComponent::CastDelayTicks() const
{ const auto Kind = Self()->Investigator->Kind; return Kind == EDMInvestigator::Sapper ? 5 : Kind == EDMInvestigator::Photographer ? 30 : 0; }
int32 UDMPrimaryComponent::CooldownTicks() const
{
    switch (Self()->Investigator->Kind) {
    case EDMInvestigator::Sapper: return 8;
    case EDMInvestigator::Photographer: return 20;
    case EDMInvestigator::Medium: return 25;
    case EDMInvestigator::Smuggler: return 40;
    default: return 0; }
}
bool UDMPrimaryComponent::IsReady(int32 Tick) const { return !HeldTarget && !FrameTarget && NextCastTick <= Tick; }
void UDMPrimaryComponent::Emit(const FString& Action, ADMCombatant* Target)
{
    if (auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    {
        auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("actor_id"), Self()->EntityId);
        D->SetStringField(TEXT("ability"), Name()); D->SetStringField(TEXT("action"), Action);
        if (Target) { D->SetStringField(TEXT("target_id"), Target->EntityId); }
        Mode->Emit(TEXT("ability.q"), D);
    }
}
void UDMPrimaryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDMPrimaryComponent, Cooldown); DOREPLIFETIME(UDMPrimaryComponent, FrameTarget);
    DOREPLIFETIME(UDMPrimaryComponent, HeldTarget); DOREPLIFETIME(UDMPrimaryComponent, Satchels); DOREPLIFETIME(UDMPrimaryComponent, Bindings);
}

FString UDMPrimaryComponent::ReplicationSummary() const
{
    FString Result = FString::Printf(TEXT("satchels=%d bindings=%d frame=%s hold=%s"), Satchels.Num(), Bindings.Num(),
        FrameTarget ? *FrameTarget->EntityId : TEXT("none"), HeldTarget ? *HeldTarget->EntityId : TEXT("none"));
    for (const auto& R : Self()->Investigator->Exposure) { Result += FString::Printf(TEXT(" E:%s=%.3f"), *R.Id, R.Value); }
    for (const auto& R : Self()->Investigator->Spirits) { Result += FString::Printf(TEXT(" A:%s=%.3f"), *R.Id, R.Value); }
    return Result;
}
