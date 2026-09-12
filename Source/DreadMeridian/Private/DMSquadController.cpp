#include "DMSquadController.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMEncounterLayout.h"
#include "DMAIProfile.h"
#include "DMAbilityMarker.h"
#include "DMScroungePickup.h"
#include "DMPing.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
    /** Ticks a rejected cast option stays vetoed ('cooldown') after the world refused it. */
    constexpr int32 RejectCooldownTicks = 10;
    /**
     * Directions sampled per companion per tick for the positional scorer. Twelve is 30 degrees apart: fine enough
     * to find a way past a crate, coarse enough that four bots cost about 480 traces a second against the two per
     * bot the controller already spends. Revisit before enemies start using it and the count triples.
     */
    constexpr int32 WhiskerRays = 12;
}

void ADMSquadController::ConfigureEncounter(int32 Group, FVector Home, bool bPatrol, int32 Slot)
{
    EncounterGroup = Group; HomePosition = Home; bPatrolMember = bPatrol; PatrolSlot = Slot;
}

void ADMSquadController::Think(ADMCombatGameMode& Mode)
{
    ADMCombatant* Self = Cast<ADMCombatant>(GetPawn());
    if (!Self || Self->IsDown() || Self->IsRestrained()) { return; }
    if (!Profile) { Profile = Mode.ProfileFor(*Self); }
    if (!Profile) { UE_LOG(LogTemp, Warning, TEXT("ADMSquadController: no AI profile for %s"), *Self->EntityId); return; }
    FDMAIContext Context;
    BuildContext(Mode, Context);
    FDMAIDecision Decision = DMUtilityAI::Decide(Context);
    Memory = Decision.Memory;
    const FString Rejected = Execute(Mode, Context, Decision);
    Trace(Mode, Context, Decision, Rejected);
    LastContext = MoveTemp(Context);
    LastDecision = MoveTemp(Decision);
}

void ADMSquadController::BuildContext(ADMCombatGameMode& Mode, FDMAIContext& Out) const
{
    ADMCombatant* Self = CastChecked<ADMCombatant>(GetPawn());
    const TArray<TObjectPtr<ADMCombatant>>& Roster = Mode.GetCombatants();
    const FDMAIWeights& W = Profile->Weights;
    const int32 Tick = Mode.GetCombatTick();
    const FVector SelfLoc = Self->GetActorLocation();
    auto IndexOf = [&](const ADMCombatant* Actor) -> int32
    {
        if (!Actor) { return INDEX_NONE; }
        for (int32 I = 0; I < Roster.Num(); ++I) { if (Roster[I].Get() == Actor) { return I; } }
        return INDEX_NONE;
    };
    auto IndexOfId = [&](const FString& Id) -> int32
    {
        if (Id.IsEmpty()) { return INDEX_NONE; }
        for (int32 I = 0; I < Roster.Num(); ++I) { if (Roster[I]->EntityId == Id) { return I; } }
        return INDEX_NONE;
    };

    Out.Tick = Tick;
    Out.W = &W;
    Out.Memory = Memory;
    Out.PlayableExtent = FVector(DMEncounterLayout::PlayableX, DMEncounterLayout::PlayableY, 0);

    // Leader: first living player-controlled investigator in roster order.
    ADMCombatant* Leader = nullptr;
    for (ADMCombatant* Ally : Roster)
    { if (!Ally->bIsEnemy && Ally->IsPlayerControlled() && !Ally->IsDown()) { Leader = Ally; break; } }
    Out.LeaderIndex = IndexOf(Leader);

    FDMAISelfView& S = Out.Self;
    S.Index = IndexOf(Self);
    S.EntityId = Self->EntityId;
    S.bEnemy = Self->bIsEnemy;
    S.bProfileRange = Self->bProfileRange;
    S.bLocalEnemy = Mode.UsesEncounterLayout() && Self->bIsEnemy && EncounterGroup >= 0 && EncounterGroup <= 3;
    S.bPatrolMember = bPatrolMember;
    S.bCompanionTethered = Mode.UsesEncounterLayout() && !Self->bIsEnemy && Leader != nullptr;
    S.bCasting = Self->Smuggler->IsCasting();
    S.bRestrained = Self->IsRestrained();
    S.bAttackReady = Self->NextAttackTick <= Tick;
    S.bSetPosition = Self->Smuggler->bSetPosition;
    S.bRanged = Self->Smuggler->IsRanged();
    S.Role = Self->Smuggler->Role;
    S.Kind = Self->Investigator->Kind;
    S.Health = Self->Health(); S.MaxHealth = Self->MaxHealth();
    S.AttackRange = Self->GetAttackRange(); S.AttackDamage = Self->AttackDamage; S.AttackInterval = Self->AttackIntervalTicks;
    S.Location = SelfLoc;
    S.Anchor = bPatrolMember ? DMEncounterLayout::PatrolPoint(PatrolWaypoint) + FVector(0, PatrolSlot * 120, 0) : HomePosition;
    S.EncounterGroup = EncounterGroup; S.PatrolWaypoint = PatrolWaypoint;
    S.Charges = Self->Investigator->Charges; S.ChargeCapacity = UDMInvestigatorComponent::ChargeCapacity;
    S.Satchels = 0; for (ADMAbilityMarker* M : Self->Primary->Satchels) { if (IsValid(M)) { ++S.Satchels; } }
    S.Bindings = 0; for (ADMAbilityMarker* M : Self->Primary->Bindings) { if (IsValid(M)) { ++S.Bindings; } }
    S.Momentum = Self->Investigator->Momentum;
    S.bQReady = Self->Primary->IsReady(Tick);
    S.QCooldownRemaining = Self->Primary->CooldownRemaining(Tick);
    S.FrameTarget = IndexOf(Self->Primary->FrameTarget.Get());
    S.HeldTarget = IndexOf(Self->Primary->HeldTarget.Get());
    const UDMKitComponent* Kit = Self->Kit;
    S.bWReady = Kit->IsReady(EDMKitSlot::W); S.bEReady = Kit->IsReady(EDMKitSlot::E); S.bRReady = Kit->IsReady(EDMKitSlot::R);
    S.WCooldownRemaining = Kit->CooldownRemaining(EDMKitSlot::W, Tick);
    S.ECooldownRemaining = Kit->CooldownRemaining(EDMKitSlot::E, Tick);
    S.RCooldownRemaining = Kit->CooldownRemaining(EDMKitSlot::R, Tick);
    S.bRActive = Kit->IsRActive(); S.bBraced = Kit->IsBraced(); S.bCharging = Kit->IsCharging(); S.bWirePending = Kit->bWirePending;
    S.Zones = 0; for (ADMAbilityMarker* M : Kit->Zones) { if (IsValid(M)) { ++S.Zones; } }
    S.Wires = 0; for (ADMAbilityMarker* M : Kit->Wires) { if (IsValid(M)) { ++S.Wires; } }
    S.MaxAttention = 0;
    for (const FDMSubjectResource& Spirit : Self->Investigator->Spirits) { S.MaxAttention = FMath::Max(S.MaxAttention, Spirit.Value); }
    S.Madness = Self->Investigator->Madness;
    S.bSignatureReady = Self->Smuggler->IsSignatureReady(Tick);
    S.bSignatureSight = false;

    const ADMCombatant* Forced = UDMSmugglerComponent::FocusTarget(Mode, Self, true);
    const ADMCombatant* Marked = UDMSmugglerComponent::FocusTarget(Mode, Self, false);
    const ADMCombatant* Diver = Self->Smuggler->Role == EDMSmuggler::Bruiser ? UDMSmugglerComponent::DiverTarget(Mode, Self) : nullptr;
    const ADMCombatant* Current = Self->GetAttackTarget();

    Out.Actors.Reset(Roster.Num());
    TArray<bool> Traced; Traced.Init(false, Roster.Num());
    for (int32 I = 0; I < Roster.Num(); ++I)
    {
        ADMCombatant* C = Roster[I].Get();
        const FVector Loc = C->GetActorLocation();
        FDMAIActorView& V = Out.Actors.AddDefaulted_GetRef();
        V.Index = I; V.EntityId = C->EntityId;
        V.bEnemy = C->bIsEnemy; V.bDown = C->IsDown(); V.bRestrained = C->IsRestrained();
        V.bPlayerControlled = C->IsPlayerControlled(); V.bCommon = C->bCommonEnemy; V.bBreakVulnerable = C->bBreakVulnerable;
        V.bForced = C == Forced; V.bMarked = C == Marked; V.bDiver = C == Diver;
        V.bBoundByMe = Self->Primary->Bindings.ContainsByPredicate([&](const ADMAbilityMarker* M) { return IsValid(M) && M->BoundTarget == C; });
        V.bHeldByMe = Self->Primary->HeldTarget == C;
        V.bRanged = C->Smuggler->IsRanged();
        const auto* Bot = Cast<ADMSquadController>(C->GetController());
        V.EncounterGroup = Bot ? Bot->EncounterGroup : INDEX_NONE;
        V.AttackTargetIndex = IndexOf(C->GetAttackTarget());
        V.Health = C->Health(); V.MaxHealth = C->MaxHealth(); V.Shield = C->Shield();
        V.Distance = FVector::Distance(SelfLoc, Loc);
        V.Distance2D = FVector::Dist2D(SelfLoc, Loc);
        V.AnchorDistance2D = FVector::Dist2D(S.Anchor, Loc);
        V.LeaderDistance2D = Leader ? FVector::Dist2D(Leader->GetActorLocation(), Loc) : 0.f;
        V.Threat = Self->Threat.FindRef(C->EntityId);
        V.ExposureMultiplier = Self->Investigator->DamageMultiplier(C->EntityId, false);
        V.AttackDamage = C->AttackDamage; V.AttackInterval = C->AttackIntervalTicks;
        V.NextAttackIn = FMath::Max(0, C->NextAttackTick - Tick);
        V.Speed2D = C->GetVelocity().Size2D();
        V.Velocity2D = C->GetVelocity() * FVector(1, 1, 0);
        V.Location = Loc;
        V.Exposure = Self->Investigator->PeekExposure(C->EntityId);
        V.bSuppressed = C->bSuppressed; V.bCommitted = C->bTelegraphActive; V.Break = C->Break;
        V.bGroupEngaged = false; V.bVisible = true;
        const bool bHostile = C->bIsEnemy != Self->bIsEnemy && !V.bDown;
        if (S.bLocalEnemy && bHostile)
        {
            // Group alert: same loop as the previous cascade, so camp-mates acquire together.
            for (ADMCombatant* Friend : Roster)
            {
                const auto* FriendBot = Cast<ADMSquadController>(Friend->GetController());
                if (Friend != Self && !Friend->IsDown() && FriendBot && FriendBot->EncounterGroup == EncounterGroup
                    && (Friend->GetAttackTarget() == C || Friend->Threat.FindRef(C->EntityId) > 0)) { V.bGroupEngaged = true; break; }
            }
            const bool bAlerted = V.bForced || V.bMarked || V.bDiver || V.Threat > 0 || C == Current || V.bGroupEngaged;
            // LOS trace only for unalerted hostiles inside sight range (character-appropriate perception, never omniscient).
            if (!bAlerted && V.Distance2D <= W.SightRange) { V.bVisible = LineOfSightTo(C); Traced[I] = true; }
        }
    }

    // Whiskers: how far this bot could actually walk in each direction before something stops it. The pure layer
    // cannot trace, and there is no nav mesh - a bot steers straight at its goal - so without these the positional
    // scorer would happily pick a point behind a crate and grind into the crate forever. Companions only: this is
    // the only per-tick cost the positioning work adds, and enemies do not use it yet.
    if (!Self->bIsEnemy && W.PositionStep > 0)
    {
        FCollisionQueryParams Query(SCENE_QUERY_STAT(DMWhisker), false, Self);
        for (ADMCombatant* Other : Roster) { Query.AddIgnoredActor(Other); }
        const float Reach = W.PositionStep + 60.f;
        Out.Clearance.SetNumUninitialized(WhiskerRays);
        for (int32 Ray = 0; Ray < WhiskerRays; ++Ray)
        {
            const float Angle = 2.f * PI * Ray / WhiskerRays;
            const FVector End = SelfLoc + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Reach;
            FHitResult Hit;
            Out.Clearance[Ray] = GetWorld()->LineTraceSingleByChannel(Hit, SelfLoc, End, ECC_Visibility, Query)
                ? static_cast<float>(FVector::Dist2D(SelfLoc, Hit.ImpactPoint)) : Reach;
        }
    }

    // The bot's own persistent markers, so kit options can reason about where its traps, zones and spirits are.
    // Only its own: another Sapper's wire is not this one's to plan around.
    auto AddMarker = [&](const ADMAbilityMarker* M, FDMAIMarkerView::EKind Kind)
    {
        if (!IsValid(M)) { return; }
        FDMAIMarkerView& MV = Out.Markers.AddDefaulted_GetRef();
        MV.Kind = Kind; MV.Location = M->GetActorLocation(); MV.WireEnd = M->WireEnd;
        MV.Direction = M->Direction; MV.HalfAngle = M->HalfAngle; MV.Length = M->Length; MV.Radius = M->Radius;
        MV.bArmed = M->IsArmed(); MV.bTravelling = M->bTravelling; MV.BoundIndex = IndexOf(M->BoundTarget.Get());
        MV.Attention = M->Attention; MV.Serial = M->Serial; MV.SpiritId = M->SpiritId;
    };
    for (const ADMAbilityMarker* M : Self->Primary->Satchels) { AddMarker(M, FDMAIMarkerView::Satchel); }
    for (const ADMAbilityMarker* M : Self->Primary->Bindings) { AddMarker(M, FDMAIMarkerView::Spirit); }
    for (const ADMAbilityMarker* M : Self->Kit->Wires) { AddMarker(M, FDMAIMarkerView::Wire); }
    for (const ADMAbilityMarker* M : Self->Kit->Zones) { AddMarker(M, FDMAIMarkerView::Zone); }

    for (TActorIterator<ADMScroungePickup> It(GetWorld()); It; ++It) { Out.Pickups.Add(It->GetActorLocation()); }
    for (TActorIterator<ADMAbilityMarker> It(GetWorld()); It; ++It)
    {
        if (!It->bHostile || It->BoundTarget) { continue; }
        FDMAIHazard& H = Out.Hazards.AddDefaulted_GetRef();
        const FVector Centre = It->GetActorLocation();
        H.Center = FVector(Centre.X, Centre.Y, SelfLoc.Z); H.Radius = It->Radius;
    }

    // Pings are a companion concern: enemies never read the board, so they can neither answer nor author pings.
    if (!Self->bIsEnemy)
    {
        const FDMPingBoard& Board = Mode.GetPingBoard();
        for (const FDMPing& P : Board.Pings)
        {
            if (!P.IsLive(Tick)) { continue; }
            FDMAIPingView& PV = Out.Pings.AddDefaulted_GetRef();
            PV.Id = P.Id; PV.Kind = P.Kind; PV.bHuman = !P.bAuthorBot;
            PV.bAuthoredBySelf = P.AuthorId == Self->EntityId;
            PV.bRespondedBySelf = Board.HasResponse(P.Id, Self->EntityId);
            PV.TargetIndex = IndexOfId(P.TargetId);
            // A ping on a combatant follows that combatant (the board stores where it was authored).
            PV.Location = Roster.IsValidIndex(PV.TargetIndex) ? Roster[PV.TargetIndex]->GetActorLocation() : P.Location;
            PV.AgeFrac = P.AgeFraction(Tick); PV.AgeTicks = Tick - P.CreatedTick;
            PV.Weight = FMath::Clamp((PV.bHuman ? 1.f : W.BotPingWeight) * W.PingCompliance, 0.f, 1.f);
        }

        // What teammates have said they are doing. Self's own claims are excluded: a bot that read its own
        // intent back would reinforce whatever it already chose, which is not coordination but an echo.
        for (const FDMSquadClaim& C : Mode.GetSquadBoard().Claims)
        {
            if (C.AuthorId == Self->EntityId || C.AgeTicks(Tick) >= FDMSquadBoard::LifetimeTicks) { continue; }
            const int32 AuthorIndex = IndexOfId(C.AuthorId);
            if (!Roster.IsValidIndex(AuthorIndex) || Roster[AuthorIndex]->IsDown()) { continue; }
            FDMAIClaimView& CV = Out.Claims.AddDefaulted_GetRef();
            CV.Kind = C.Kind; CV.AuthorIndex = AuthorIndex; CV.TargetIndex = C.TargetIndex;
            CV.Location = C.Location; CV.Location2 = C.Location2;
            CV.Radius = C.Radius; CV.Magnitude = C.Magnitude;
            CV.AgeTicks = C.AgeTicks(Tick); CV.ResolveIn = C.ResolveTick - Tick; CV.Serial = C.Serial;
        }
    }

    // One attack-sight trace per bot per tick against the provisional focus (the same world trace ResolveAttack applies):
    // Engage keeps closing on an occluded target and BasicAttack holds instead of telegraphing at a wall. The signature
    // reads the same result. Perception traces above are never overwritten.
    {
        const int32 Focus = DMUtilityAI::ChooseFocus(Out);
        if (Out.Actors.IsValidIndex(Focus))
        {
            FDMAIActorView& FV = Out.Actors[Focus];
            const bool bSight = Self->Smuggler->Sight(SelfLoc, FV.Location);
            if (!Traced[Focus]) { FV.bVisible = bSight; }
            if (S.bSignatureReady) { S.bSignatureSight = bSight; }
        }
    }
}

FString ADMSquadController::Execute(ADMCombatGameMode& Mode, const FDMAIContext& Context, const FDMAIDecision& Decision)
{
    ADMCombatant* Self = CastChecked<ADMCombatant>(GetPawn());
    const TArray<TObjectPtr<ADMCombatant>>& Roster = Mode.GetCombatants();
    const FDMAIWeights& W = Profile->Weights;
    auto ActorAt = [&](int32 Index) -> ADMCombatant* { return Roster.IsValidIndex(Index) ? Roster[Index].Get() : nullptr; };
    const int32 Tick = Mode.GetCombatTick();
    const FVector SelfLoc = Self->GetActorLocation();
    FString Rejected;
    auto Reject = [&](const FString& Reason, EDMAIAction Action)
    {
        if (Rejected.IsEmpty()) { Rejected = Reason.IsEmpty() ? FString(TEXT("rejected")) : Reason; }
        // A rejected cast is vetoed 'cooldown' for a few ticks so it cannot wedge its channel and re-trace every tick.
        Memory.CooldownUntil.Add(Action, Tick + RejectCooldownTicks);
    };
    auto Move = [&](const FVector& Goal)
    {
        // Enemies plant for their telegraph; a hold goal (Point = self) is a StopGoal.
        if ((Self->bTelegraphActive && Self->TelegraphEndTick > Tick) || FVector::DistSquared2D(Goal, SelfLoc) <= 1.f) { Self->StopGoal(); }
        else { Self->MoveToward(Goal); }
    };

    Self->SetAttackTarget(Decision.bClearFocus || Decision.Focus == INDEX_NONE ? nullptr : ActorAt(Decision.Focus));
    Self->SetAttackHold(Decision.bAttackHold);
    if (Decision.bClearThreat) { Self->Threat.Empty(); }
    if (Decision.bAdvanceWaypoint) { PatrolWaypoint = (PatrolWaypoint + 1) % 4; }
    if (!EnumHasAnyFlags(Decision.Taken, EDMAIChannel::Move)) { Self->StopGoal(); }

    for (const FDMAIOption& O : Decision.Chosen)
    {
        // Targeted options carry the candidate; casts that omit it act on the focus.
        ADMCombatant* Target = ActorAt(O.Target != INDEX_NONE ? O.Target : Decision.Focus);
        const FVector Point = O.Point.IsNearlyZero() && Target ? Target->GetActorLocation() : O.Point;
        switch (O.Action)
        {
        case EDMAIAction::Rescue:
        {
            Self->Primary->CancelChannel(); Self->Primary->ReleaseClinch(); Self->SetAttackTarget(nullptr);
            ADMCombatant* Ally = ActorAt(O.Target);
            const FVector AllyLoc = Ally ? Ally->GetActorLocation() : O.Point;
            if (FVector::DistSquared2D(SelfLoc, AllyLoc) > FMath::Square(W.RescueRadius)) { Move(AllyLoc); }
            else { Self->StopGoal(); if (Ally) { Mode.RequestRevive(Self, Ally); } }
            break;
        }
        case EDMAIAction::HoldCast: case EDMAIAction::Patrol: case EDMAIAction::Hold: case EDMAIAction::HoldFrame:
            Self->StopGoal(); break;
        case EDMAIAction::ReturnHome: case EDMAIAction::EvadeHazard: case EDMAIAction::Flee: case EDMAIAction::RetreatToPing:
        case EDMAIAction::KeepDistance: case EDMAIAction::SeekPickup: case EDMAIAction::SeekPingedPickup: case EDMAIAction::RallyToPing:
        case EDMAIAction::DefendPing: case EDMAIAction::HelpPing: case EDMAIAction::Strafe: case EDMAIAction::Engage:
        case EDMAIAction::InvestigatePing: case EDMAIAction::Anchor: case EDMAIAction::FollowLeader:
        case EDMAIAction::Reposition:
            Move(O.Point); break;
        case EDMAIAction::BasicAttack: break; // the game mode fires TryAttack after Think; the hold gate is already open
        case EDMAIAction::Signature:
            if (!Self->Smuggler->TrySignature(Mode, Target)) { Reject(TEXT("signature_rejected"), O.Action); }
            break;
        case EDMAIAction::Throw:
        {
            FVector Aim = O.Point;
            if (ADMCombatant* Held = Self->Primary->HeldTarget.Get(); Aim.IsNearlyZero() && Held)
            { Aim = Held->GetActorLocation() + (Held->GetActorLocation() - Self->GetActorLocation()).GetSafeNormal2D() * 300; }
            if (!Self->Primary->Request(nullptr, Aim)) { Reject(Self->Primary->LastFailure, O.Action); }
            break;
        }
        case EDMAIAction::PlaceSatchel:
            if (!Self->Primary->Request(nullptr, Point)) { Reject(Self->Primary->LastFailure, O.Action); }
            break;
        case EDMAIAction::BindSpirit: case EDMAIAction::Frame: case EDMAIAction::Clinch:
            if (!Target) { Reject(TEXT("no_target"), O.Action); }
            else if (!Self->Primary->Request(Target, Target->GetActorLocation())) { Reject(Self->Primary->LastFailure, O.Action); }
            break;
        case EDMAIAction::SuppressingFire:
            if (!Self->Kit->Request(EDMKitSlot::W, nullptr, Point)) { Reject(Self->Kit->LastFailure, O.Action); }
            break;
        case EDMAIAction::Tripwire:
            // Both ends in one call: a bot must never be left holding half a wire.
            if (!Self->Kit->RequestWire(O.Point, O.Point2)) { Reject(Self->Kit->LastFailure, O.Action); }
            break;
        case EDMAIAction::DeadGround: case EDMAIAction::ImpossiblePhotograph: case EDMAIAction::OpenSeance:
        case EDMAIAction::DrownedMan:
            if (!Self->Kit->Request(EDMKitSlot::R, nullptr, Self->GetActorLocation())) { Reject(Self->Kit->LastFailure, O.Action); }
            break;
        case EDMAIAction::Flashbulb: case EDMAIAction::Beckon: case EDMAIAction::ShoulderThrough:
            if (!Self->Kit->Request(EDMKitSlot::W, nullptr, Point)) { Reject(Self->Kit->LastFailure, O.Action); }
            break;
        case EDMAIAction::Intercession: case EDMAIAction::DigIn:
            if (!Self->Kit->Request(EDMKitSlot::E, nullptr, Self->GetActorLocation())) { Reject(Self->Kit->LastFailure, O.Action); }
            break;
        case EDMAIAction::Develop:
            if (!Target) { Reject(TEXT("no_target"), O.Action); }
            else if (!Self->Kit->Request(EDMKitSlot::E, Target, Target->GetActorLocation())) { Reject(Self->Kit->LastFailure, O.Action); }
            break;
        default: break;
        }
    }

    // Pings are a companion concern (the context carries none for enemies; this gate is belt and braces).
    if (!Self->bIsEnemy)
    {
        // Say what this decision committed to, so the teammates who Think after it can hear it this tick and
        // the ones that already thought hear it next tick. Published after execution, so a claim always
        // describes something the bot actually did rather than something it merely considered.
        FDMSquadBoard& Squad = Mode.GetSquadBoard();
        for (FDMSquadClaim Claim : Decision.Claims)
        {
            Claim.AuthorId = Self->EntityId;
            Claim.Tick = Tick;
            if (Claim.ResolveTick <= 0) { Claim.ResolveTick = Tick; }
            Squad.Publish(Claim);
        }

        // Bot-authored pings go through the same board as human ones so the HUD shows the callout.
        for (const FDMAIPingRequest& R : Decision.PingRequests)
        {
            ADMCombatant* T = ActorAt(R.TargetIndex);
            Mode.CreatePing(R.Kind, Self->EntityId, true, T ? T->GetActorLocation() : R.Location, T ? T->EntityId : FString());
        }
        const FDMPingBoard& Board = Mode.GetPingBoard();
        for (int32 Id : Decision.PingsOnIt)
        {
            const FDMPing* P = Board.Find(Id);
            if (P && !P->OnIt.Contains(Self->EntityId)) { Mode.RespondToPing(Id, Self->EntityId, true); }
        }
        // Busy only for tasks this bot could have taken: never for informational (Perceive) or modifier (Ignore) pings,
        // a Pickup ping unless it is the Sapper, or a Help ping about itself.
        for (const FDMAIPingView& P : Context.Pings)
        {
            if (P.bAuthoredBySelf || P.bRespondedBySelf || Decision.PingsOnIt.Contains(P.Id) || P.AgeTicks < FDMPingBoard::BusyAfterTicks) { continue; }
            if (P.Kind == EDMPingKind::Perceive || P.Kind == EDMPingKind::Ignore || P.TargetIndex == Context.Self.Index
                || (P.Kind == EDMPingKind::Pickup && Context.Self.Kind != EDMInvestigator::Sapper)) { continue; }
            Mode.RespondToPing(P.Id, Self->EntityId, false);
        }
    }
    return Rejected;
}

namespace
{
    double Round2(float Value) { return FMath::RoundToDouble(Value * 100.0) / 100.0; }

    const FDMAIOption* FirstActOption(const FDMAIDecision& Decision)
    {
        for (const FDMAIOption& O : Decision.Chosen)
        { if (EnumHasAnyFlags(O.Channels, EDMAIChannel::Cast | EDMAIChannel::Attack)) { return &O; } }
        return nullptr;
    }

    TSharedRef<FJsonObject> OptionJson(const FDMAIContext& Context, const FDMAIOption& O, bool bAct)
    {
        TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
        J->SetStringField(TEXT("action"), DMUtilityAI::ActionName(O.Action));
        J->SetStringField(TEXT("target_id"), Context.Actors.IsValidIndex(O.Target) ? Context.Actors[O.Target].EntityId : FString());
        J->SetStringField(TEXT("rank"), DMUtilityAI::RankName(O.Rank));
        J->SetNumberField(TEXT("score"), Round2(O.Score));
        if (bAct)
        {
            J->SetNumberField(TEXT("value"), Round2(O.Value));
            J->SetNumberField(TEXT("threshold"), Round2(O.Threshold));
            J->SetNumberField(TEXT("p_hit"), Round2(O.PHit));
        }
        if (O.PingId != INDEX_NONE) { J->SetNumberField(TEXT("ping_id"), O.PingId); }
        if (O.Veto) { J->SetStringField(TEXT("veto"), O.Veto); }
        TArray<TSharedPtr<FJsonValue>> Considerations;
        for (const FDMAIScoredConsideration& C : O.Breakdown)
        {
            TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
            Item->SetStringField(TEXT("input"), DMUtilityAI::InputName(C.Input));
            Item->SetNumberField(TEXT("raw"), Round2(C.Raw));
            Item->SetNumberField(TEXT("score"), Round2(C.Score));
            Considerations.Add(MakeShared<FJsonValueObject>(Item));
        }
        J->SetArrayField(TEXT("considerations"), Considerations);
        return J;
    }
}

void ADMSquadController::Trace(ADMCombatGameMode& Mode, const FDMAIContext& Context, const FDMAIDecision& Decision, const FString& Rejected)
{
    const FDMAIOption* Move = Decision.ChosenOn(EDMAIChannel::Move);
    const FDMAIOption* Act = FirstActOption(Decision);
    const EDMAIAction MoveAction = Move ? Move->Action : EDMAIAction::None;
    const EDMAIAction ActAction = Act ? Act->Action : EDMAIAction::None;
    const TCHAR* Reason = nullptr;
    if (!bTracedOnce) { Reason = TEXT("init"); }
    else if (Decision.Focus != TracedFocus) { Reason = TEXT("target_changed"); }
    else if (MoveAction != TracedMove) { Reason = TEXT("move_changed"); }
    else if (ActAction != TracedAct) { Reason = TEXT("act_changed"); }
    else if (!Rejected.IsEmpty()) { Reason = TEXT("rejected"); }
    if (!Reason) { return; }

    const FDMAISelfView& S = Context.Self;
    TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("actor_id"), S.EntityId);
    Data->SetStringField(TEXT("role"), S.bEnemy
        ? StaticEnum<EDMSmuggler>()->GetNameStringByValue(static_cast<int64>(S.Role))
        : StaticEnum<EDMInvestigator>()->GetNameStringByValue(static_cast<int64>(S.Kind)));
    Data->SetStringField(TEXT("reason"), Reason);
    Data->SetStringField(TEXT("target_id"), Context.Actors.IsValidIndex(Decision.Focus) ? Context.Actors[Decision.Focus].EntityId : FString());
    Data->SetStringField(TEXT("prev_target_id"), Context.Actors.IsValidIndex(TracedFocus) ? Context.Actors[TracedFocus].EntityId : FString());
    Data->SetStringField(TEXT("target_rank"), DMUtilityAI::RankName(Decision.FocusRank));
    Data->SetNumberField(TEXT("target_score"), Round2(Decision.FocusScore));
    if (Move) { Data->SetObjectField(TEXT("move"), OptionJson(Context, *Move, false)); }
    if (Act) { Data->SetObjectField(TEXT("act"), OptionJson(Context, *Act, true)); }
    Data->SetStringField(TEXT("channels"), DMUtilityAI::ChannelString(Decision.Taken));
    TArray<TSharedPtr<FJsonValue>> Top;
    bool bVetoedIncluded = false;
    for (int32 I = 0; I < Decision.Ranked.Num() && Top.Num() < 5; ++I)
    { Top.Add(MakeShared<FJsonValueObject>(OptionJson(Context, Decision.Ranked[I], true))); bVetoedIncluded |= Decision.Ranked[I].Veto != nullptr; }
    if (!bVetoedIncluded)
    {
        const FDMAIOption* Vetoed = Decision.Ranked.FindByPredicate([](const FDMAIOption& O) { return O.Veto != nullptr; });
        if (Vetoed)
        {
            if (Top.Num() >= 5) { Top.Pop(); }
            Top.Add(MakeShared<FJsonValueObject>(OptionJson(Context, *Vetoed, true)));
        }
    }
    Data->SetArrayField(TEXT("top"), Top);
    Data->SetStringField(TEXT("rejected"), Rejected);
    Mode.Emit(TEXT("ai.decision"), Data);

    TracedFocus = Decision.Focus; TracedMove = MoveAction; TracedAct = ActAction; bTracedOnce = true;
}
