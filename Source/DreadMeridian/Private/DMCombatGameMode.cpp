#include "DMCombatGameMode.h"
#include "DMFishingVillage.h"
#include "DMEditorPlaySelection.h"
#include "DMCombatant.h"
#include "DMRecoverySupply.h"
#include "DMObjective.h"
#include "DMVision.h"
#include "DMElderOne.h"
#include "DMCorruption.h"
#include "DMGrowthNetwork.h"
#include "DMCorpse.h"
#include "DMShubMinion.h"
#include "DMShubEncounter.h"
#include "DMRelicDrop.h"
#include "DMCombatPlayerController.h"
#include "DMCombatHUD.h"
#include "DMSquadController.h"
#include "DMSandboxArena.h"
#include "DMEncounterLayout.h"
#include "DMGameState.h"
#include "DMAIProfile.h"
#include "DMScroungePickup.h"
#include "DMAbilityMarker.h"
#include "DMKitRules.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "PlaytraceCaptureSubsystem.h"
#include "TimerManager.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

ADMCombatGameMode::ADMCombatGameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = ADMCombatPlayerController::StaticClass();
    HUDClass = ADMCombatHUD::StaticClass();
}

void ADMCombatGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
#if !UE_BUILD_SHIPPING
    FParse::Value(FCommandLine::Get(), TEXT("DMCombatSmoke="), SmokeOutcome);
    FParse::Value(FCommandLine::Get(), TEXT("DMAIWeights="), AIWeightsPath);
    bNetworkTest = FParse::Param(FCommandLine::Get(), TEXT("DMNetworkTest"));
    bSwampTest = FParse::Param(FCommandLine::Get(), TEXT("DMSwampProbe"));
    if (FParse::Param(FCommandLine::Get(),TEXT("DMShellProbe"))) { bFishingVillage=false; }
#endif
}

void ADMCombatGameMode::ConfigureCaptureMetadata(const TSharedRef<FJsonObject>& Metadata)
{
    Metadata->SetStringField(TEXT("scenario_id"), TEXT("combat-sandbox"));
    Metadata->SetStringField(TEXT("run_kind"), TEXT("combat_sandbox"));
    Metadata->SetStringField(TEXT("capture_version"), TEXT("0.14.0"));
    if (UsesEncounterLayout()) { Metadata->SetStringField(TEXT("native_faction"), TEXT("smugglers")); }
    Metadata->SetStringField(TEXT("combat_rules_version"), TEXT("swamp-vision-v1"));
    if (bSwampTest) { Metadata->SetStringField(TEXT("native_faction"),TEXT("swamp_things")); }
    Metadata->SetStringField(TEXT("bot_policy"), TEXT("squad-utility-v5"));
    Metadata->SetStringField(TEXT("test_profile"), bNetworkTest ? TEXT("network_probe") : (SmokeOutcome.IsEmpty() ? TEXT("interactive") : SmokeOutcome));
    Metadata->SetNumberField(TEXT("initial_bot_count"), 4);
    Metadata->RemoveField(TEXT("production_bots"));
    Metadata->SetNumberField(TEXT("logical_step_seconds"), .1);
    TArray<TSharedPtr<FJsonValue>> Omissions;
    // All four sandbox families are implemented; hidden boss resonance is separate.
    for (const TCHAR* Missing : { TEXT("objectives_htn"), TEXT("madness_resonance"),
        TEXT("mythos_boss_encounters"), TEXT("medical_objectives"),
        TEXT("host_migration"), TEXT("deterministic_physics_navigation") })
    { Omissions.Add(MakeShared<FJsonValueString>(Missing)); }
    Metadata->SetArrayField(TEXT("omissions"), Omissions);
    if (bFishingVillage)
    {
        Metadata->SetStringField(TEXT("scenario_id"),TEXT("fishing-village-fixed-v1"));
        Metadata->SetStringField(TEXT("run_kind"),TEXT("fixed_scenario"));
        Metadata->SetStringField(TEXT("capture_version"),TEXT("0.15.0"));
        Metadata->SetStringField(TEXT("combat_rules_version"),TEXT("fishing-village-v1"));
        Metadata->SetNumberField(TEXT("boss_selection_version"),2);
        TArray<TSharedPtr<FJsonValue>> Missing;
        for (const TCHAR* S : {TEXT("htn_generation"),TEXT("timed_ritual_escalation"),TEXT("nyarlathotep_scenario"),TEXT("host_migration"),TEXT("full_map_fog")}) { Missing.Add(MakeShared<FJsonValueString>(S)); }
        Metadata->SetArrayField(TEXT("omissions"),Missing);
    }
}

void ADMCombatGameMode::StartPlay()
{
    Super::StartPlay();
    if (ShouldBeginEncounterOnStartPlay()) { BeginEncounter(); }
}

void ADMCombatGameMode::BeginEncounter()
{
    if (bCombatActive || Combatants.Num() > 0) { return; }
    if (bFishingVillage)
    { for (TActorIterator<ADMFishingVillage> It(GetWorld());It;++It) { Village=*It; break; } if (!Village) { Village=GetWorld()->SpawnActor<ADMFishingVillage>(); } }
    else if (!TActorIterator<ADMSandboxArena>(GetWorld())) { GetWorld()->SpawnActor<ADMSandboxArena>(); }
    int32 Seed = 1927;
    FParse::Value(FCommandLine::Get(), TEXT("DMSeed="), Seed);
    FDMRandomStreams Layout(Seed);
    const bool bSmoke = !SmokeOutcome.IsEmpty();
    for (int32 Index = 0; Index < (bFishingVillage ? 4 : bSwampTest ? 9 : UsesEncounterLayout() ? 4 + DMEncounterLayout::EnemyCount : 7); ++Index)
    {
        const bool bEnemy = Index >= 4;
        const float X = bEnemy ? (bSmoke ? 170.f : 650.f) : (bSmoke ? -170.f : -650.f);
        const float Y = ((bEnemy ? Index - 4 : Index) - 1.5f) * 100.f;
        const float Jitter = bSmoke ? 0.f : static_cast<float>(Layout.Next(EDMRandomStream::RunGeneration) % 40);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        FVector Position = UsesEncounterLayout()
            ? (bEnemy ? DMEncounterLayout::EnemyPosition(Index - 4) : FVector(-2100, Y, 95))
            : FVector(X + Jitter, Y, 95);
        if (UsesEncounterLayout())
        {
            // Seed-driven placement jitter (+-30 per axis) so different -DMSeed runs diverge; the encounter test tolerates 100u.
            Position.X += static_cast<float>(static_cast<int32>(Layout.Next(EDMRandomStream::RunGeneration) % 61) - 30);
            Position.Y += static_cast<float>(static_cast<int32>(Layout.Next(EDMRandomStream::RunGeneration) % 61) - 30);
        }
        if (bFishingVillage) { Position=FVector(-2550,-1100+Index*90,95); }
        ADMCombatant* Actor = GetWorld()->SpawnActor<ADMCombatant>(Position, FRotator::ZeroRotator, Params);
        check(Actor);
        const bool bLoss = SmokeOutcome == TEXT("Defeat");
        const bool bReviveTest = SmokeOutcome == TEXT("Revive");
        const float HP = bEnemy ? (bLoss ? 2000.f : (bSmoke && !bReviveTest ? 60.f : (bSmoke ? 150.f : 350.f))) : 100.f;
        const float Damage = bEnemy ? (bLoss ? 500.f : (bReviveTest ? 150.f : (bSmoke ? 3.f : 7.f))) : (bSmoke && !bReviveTest ? 35.f : 12.f);
        Actor->InitializeCombatant(FString::Printf(TEXT("%s.%d"), bEnemy ? TEXT("enemy") : TEXT("investigator"), bEnemy ? Index - 4 : Index), bEnemy, HP, Damage, bEnemy ? 0 : 20);
        if (bEnemy && UsesEncounterLayout())
        {
            Actor->Smuggler->Initialize(DMEncounterLayout::EnemyRole(Index - 4));
            Actor->InitializeCombatant(Actor->EntityId, true, Actor->Smuggler->BaseHealth(), Actor->Smuggler->BaseDamage());
        }
        Actor->bProfileRange = bSmoke;
        if (bSwampTest && bEnemy)
        {
            Actor->InitializeCombatant(Actor->EntityId,true,Index==8 ? 350 : 90,Index==8 ? 14 : 8);
            Actor->Swamp->Initialize(static_cast<EDMSwampThing>(Index-3));
            if (Index==5) { auto* Reeds=GetWorld()->SpawnActor<ADMVisionArea>(Position,FRotator::ZeroRotator); Reeds->Radius=250; }
        }
        if (!bEnemy) { Actor->InitializeInvestigator(static_cast<EDMInvestigator>(Index + 1), bSmoke); }
        if (bReviveTest && bEnemy)
        {
            Actor->AttackIntervalTicks = 100;
            Actor->NextAttackTick = Index == 4 ? 0 : 1000;
        }
        if (!bEnemy)
        {
            const uint32 Roll=DrawRandom(EDMRandomStream::Madness);
            // The network fixture covers all supported families; normal runs use the named stream.
            Actor->MadnessCore->AssignFamily(static_cast<EDMMadnessFamily>(1+(bNetworkTest ? (Index+2) % UDMMadnessComponent::SupportedFamilies : Roll % UDMMadnessComponent::SupportedFamilies)));
        }
        Combatants.Add(Actor);
        TSharedRef<FJsonObject> Spawn = MakeShared<FJsonObject>();
        Spawn->SetStringField(TEXT("entity_id"), Actor->EntityId);
        Spawn->SetStringField(TEXT("team"), bEnemy ? TEXT("enemy") : TEXT("investigator"));
        Spawn->SetNumberField(TEXT("health"), Actor->Health());
        Spawn->SetNumberField(TEXT("shield"), Actor->Shield());
        Spawn->SetNumberField(TEXT("attack_damage"), Actor->AttackDamage);
        Spawn->SetStringField(TEXT("display_name"), Actor->DisplayName());
        Spawn->SetNumberField(TEXT("attack_range"), Actor->GetAttackRange());
        Spawn->SetNumberField(TEXT("attack_interval_ticks"), Actor->AttackIntervalTicks);
        Spawn->SetNumberField(TEXT("initial_next_attack_tick"), Actor->NextAttackTick);
        Spawn->SetStringField(TEXT("control"), TEXT("bot"));
        Emit(TEXT("combat.spawned"), Spawn);
        AttachBot(Actor);
    }
    if (UsesEncounterLayout())
    {
        GetWorld()->SpawnActor<ADMRecoverySupply>(FVector(-2200, 350, 40), FRotator::ZeroRotator);
        auto* Food = GetWorld()->SpawnActor<ADMRecoverySupply>(FVector(-1900, -350, 40), FRotator::ZeroRotator);
        Food->bFood = true; Food->Charges = 1;
    }
    if (!bNetworkTest && !bSmoke)
    {
        TArray<ADMCombatant*> Roster; TArray<uint32> Draws;
        for (ADMCombatant* A : Combatants) { if (!A->bIsEnemy) { Roster.Add(A); Draws.Add(DrawRandom(EDMRandomStream::Madness)); } }
        ElderOne->AssignResonance(Roster,DrawRandom(EDMRandomStream::Madness),Draws);
    }
    if (bNetworkTest && bSwampTest)
    {
        // Out-of-sight replication sentinel; deliberately outside the gameplay roster/outcome.
        auto* Hidden=GetWorld()->SpawnActor<ADMCombatant>(FVector(0,5000,95),FRotator::ZeroRotator);
        Hidden->InitializeCombatant(TEXT("vision.hidden_probe"),true,100,0); Hidden->Swamp->Initialize(EDMSwampThing::Lurker); Hidden->SetAttackHold(true);
    }
    bCombatActive = true;
    PublishEncounter();
    if (bSmoke) { bGuardChecksPassed = !Combatants[0]->TryAttack(Combatants[0]) && !Combatants[0]->TryAttack(nullptr); }
    // Sandbox combat starts immediately; the village stays in Expedition until its objective gate.
    if (Village) { Village->StartScenario(); } else { Summon(); }
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    { AssignInvestigator(It->Get()); }
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(),TEXT("DMSmugglerProbe")) || FParse::Param(FCommandLine::Get(), TEXT("DMPresentationProbe")) || FParse::Param(FCommandLine::Get(), TEXT("DMLocomotionProbe"))) { return; }
#endif
    GetWorldTimerManager().SetTimer(CombatTimer, this, &ADMCombatGameMode::StepCombat, bSmoke ? .01f : .1f, true);
}

void ADMCombatGameMode::Emit(const FString& Type, const TSharedRef<FJsonObject>& Data)
{
    Data->SetNumberField(TEXT("tick"), CombatTick);
    GetGameInstance()->GetSubsystem<UPlaytraceCaptureSubsystem>()->RecordEvent(Type, Data);
}

ADMCombatant* ADMCombatGameMode::FindCombatant(const FString& EntityId) const
{
    if (EntityId.IsEmpty()) { return nullptr; }
    for (ADMCombatant* Actor : Combatants) { if (Actor && Actor->EntityId == EntityId) { return Actor; } }
    return nullptr;
}

UDMAIProfile* ADMCombatGameMode::ProfileFor(const ADMCombatant& Actor)
{
    // Enemies resolve by smuggler role (Kind None); investigators by kind (Role None). Smoke enemies land on Raider.
    const EDMSmuggler SmugglerRole = Actor.bIsEnemy && Actor.Smuggler ? Actor.Smuggler->Role : EDMSmuggler::None;
    const EDMInvestigator Kind = !Actor.bIsEnemy && Actor.Investigator ? Actor.Investigator->Kind : EDMInvestigator::None;
    const FString Name = UDMAIProfile::ProfileName(SmugglerRole, Kind);
    if (TObjectPtr<UDMAIProfile>* Found = Profiles.Find(Name)) { if (*Found) { return *Found; } }
    UDMAIProfile* Profile = UDMAIProfile::Resolve(this, SmugglerRole, Kind, AIWeightsPath);
    Profiles.Add(Name, Profile);
    return Profile;
}

void ADMCombatGameMode::AttachBot(ADMCombatant* Actor)
{
    ADMSquadController* Bot = GetWorld()->SpawnActor<ADMSquadController>();
    Bot->SetProfile(ProfileFor(*Actor));
    Bot->Possess(Actor);
    Actor->SetAttackHold(false);
    // A handoff never inherits a half-placed wire or an in-flight charge; zones, wires and windows persist like satchels.
    Actor->Kit->CancelWire();
    if (UsesEncounterLayout() && Actor->bIsEnemy)
    {
        const int32 EnemyIndex = Combatants.IndexOfByKey(Actor) - 4;
        const bool bPatrol = EnemyIndex >= DMEncounterLayout::CampCount * DMEncounterLayout::CampSize;
        Bot->ConfigureEncounter(bPatrol ? 3 : EnemyIndex / 3, Actor->GetActorLocation(), bPatrol, bPatrol ? EnemyIndex - 9 : 0);
    }
    Actor->ControlKind = TEXT("bot");
    Actor->ForceNetUpdate();
}

void ADMCombatGameMode::AssignInvestigator(APlayerController* Player)
{
    if (!Player || Player->GetPawn() || Cast<ADMCombatant>(Player->GetViewTarget()) || !bCombatActive) { return; }
    FString Preferred;
    FParse::Value(FCommandLine::Get(), TEXT("DMInvestigator="), Preferred);
#if WITH_EDITOR
    // The toolbar owns PIE selection, including when the editor was launched with a CLI default.
    if (GetWorld()->WorldType == EWorldType::PIE) { Preferred = DMEditorPlaySelection::Load(); }
#endif
    const TArray<FString> Names = { TEXT("Sapper"), TEXT("Photographer"), TEXT("Medium"), TEXT("Smuggler") };
    const int32 Start = FMath::Max(0, Names.IndexOfByPredicate([&](const FString& Name) { return Name.Equals(Preferred, ESearchCase::IgnoreCase); }));
    for (int32 Offset = 0; Offset < 4; ++Offset)
    {
        if (Combatants.Num() < 4) { return; }
        ADMCombatant* Actor = Combatants[(Start + Offset) % 4];
        if (Actor->bIsEnemy || Actor->IsPlayerControlled()) { continue; }
#if WITH_EDITOR
        if (GetWorld()->WorldType == EWorldType::PIE && DMEditorPlaySelection::LoadBotControl())
        {
            // Keep the real squad controller in charge; this player only observes.
            Player->SetViewTarget(Actor);
            Player->ClientSetViewTarget(Actor);
            return;
        }
#endif
        AController* Old = Actor->GetController();
        if (Old) { Old->UnPossess(); Old->Destroy(); }
        Actor->StopGoal();
        Actor->SetAttackTarget(nullptr);
        Actor->SetAttackHold(false);
        Actor->ControlKind = TEXT("human");
        Player->Possess(Actor);
        TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
        Data->SetStringField(TEXT("entity_id"), Actor->EntityId);
        Data->SetStringField(TEXT("control"), TEXT("human"));
        Emit(TEXT("control.changed"), Data);
        return;
    }
    Player->ClientMessage(TEXT("All four investigator slots are occupied."));
}
void ADMCombatGameMode::RestartPlayer(AController* Player) { AssignInvestigator(Cast<APlayerController>(Player)); }
void ADMCombatGameMode::PostLogin(APlayerController* Player) { Super::PostLogin(Player); AssignInvestigator(Player); }
void ADMCombatGameMode::Logout(AController* Exiting)
{
    ReleaseInvestigator(Exiting);
    Super::Logout(Exiting);
}
void ADMCombatGameMode::ReleaseInvestigator(AController* Exiting)
{
    ADMCombatant* Actor = Cast<ADMCombatant>(Exiting->GetPawn());
    // Unpossess before Super so PlayerController teardown does not destroy the shared investigator.
    if (Actor) { Exiting->UnPossess(); Actor->StopGoal(); Actor->Primary->CancelChannel(); Actor->Primary->ReleaseClinch(); Actor->Kit->CancelWire(); Actor->SetAttackTarget(nullptr); Actor->SetAttackHold(false); }
    if (Actor && bCombatActive)
    {
        AttachBot(Actor);
        TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
        Data->SetStringField(TEXT("entity_id"), Actor->EntityId);
        Data->SetStringField(TEXT("control"), TEXT("bot"));
        Emit(TEXT("control.changed"), Data);
    }
}

void ADMCombatGameMode::RequestRevive(ADMCombatant* Actor, ADMCombatant* Ally)
{
    if (!bCombatActive || !IsValid(Actor) || !IsValid(Ally) || Actor == Ally || Actor->bIsEnemy || Ally->bIsEnemy
        || Actor->IsDown() || Actor->IsStunned() || Actor->IsRestrained() || !Ally->IsDown()) { return; }
    auto* Existing = Revives.Find(Actor);
    if (!Existing || Existing->Key.Get() != Ally) { Revives.Add(Actor, {Ally, CombatTick}); }
}

// ---- Pings

int32 ADMCombatGameMode::CreatePing(EDMPingKind Kind, const FString& AuthorId, bool bAuthorBot, FVector Location, const FString& TargetId)
{
    if (!bCombatActive || AuthorId.IsEmpty() || Kind >= EDMPingKind::Count) { return INDEX_NONE; }
    FString Target = TargetId;
    FVector Point = Location;
    switch (Kind)
    {
    case EDMPingKind::Enemy: case EDMPingKind::Focus: case EDMPingKind::Ignore:
    {
        const ADMCombatant* Hostile = FindCombatant(Target);
        if (!Hostile || !Hostile->bIsEnemy || Hostile->IsDown() || !DMVision::CanSee(FindCombatant(AuthorId),Hostile)) { return INDEX_NONE; }
        Point = Hostile->GetActorLocation();
        break;
    }
    case EDMPingKind::Help:
    {
        const ADMCombatant* Ally = FindCombatant(Target);
        if (!Ally || Ally->bIsEnemy) { return INDEX_NONE; }
        Point = Ally->GetActorLocation();
        break;
    }
    default:
        // Ground, pickup and Perceive pings carry a location only; it is clamped into the playable extent, never rejected.
        Target.Empty();
        if (Point.ContainsNaN()) { return INDEX_NONE; }
        Point.X = FMath::Clamp(Point.X, -DMEncounterLayout::PlayableX, DMEncounterLayout::PlayableX);
        Point.Y = FMath::Clamp(Point.Y, -DMEncounterLayout::PlayableY, DMEncounterLayout::PlayableY);
        break;
    }
    TArray<FDMPingEnded> Ended;
    const int32 Id = PingBoard.Create(Kind, AuthorId, bAuthorBot, Point, Target, CombatTick, Ended);
    EmitPingEnded(Ended);
    FDMPing* Ping = Id == INDEX_NONE ? nullptr : PingBoard.Find(Id);
    if (!Ping) { if (Ended.Num()) { PublishPings(); } return INDEX_NONE; }
    if (Kind == EDMPingKind::Perceive) { Ping->bSubjective = true; Ping->TargetId.Empty(); }
    ++Metrics.PingsCreated;
    TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetNumberField(TEXT("id"), Id);
    Data->SetStringField(TEXT("kind"), FDMPingBoard::KindName(Kind));
    Data->SetStringField(TEXT("author_id"), AuthorId);
    Data->SetBoolField(TEXT("bot"), bAuthorBot);
    Data->SetStringField(TEXT("target_id"), Ping->TargetId);
    TSharedPtr<FJsonObject> Where = MakeShared<FJsonObject>();
    Where->SetNumberField(TEXT("x"), Point.X);
    Where->SetNumberField(TEXT("y"), Point.Y);
    Data->SetObjectField(TEXT("location"), Where);
    Data->SetBoolField(TEXT("subjective"), Ping->bSubjective);
    Emit(TEXT("ping.created"), Data);
    PublishPings();
    return Id;
}

bool ADMCombatGameMode::CancelPing(int32 Id, const FString& AuthorId)
{
    TArray<FDMPingEnded> Ended;
    if (!PingBoard.Cancel(Id, AuthorId, Ended)) { return false; }
    EmitPingEnded(Ended);
    PublishPings();
    return true;
}

bool ADMCombatGameMode::AcknowledgePing(int32 Id, const FString& WhoId)
{
    if (!PingBoard.Acknowledge(Id, WhoId)) { return false; }
    PublishPings();
    return true;
}

bool ADMCombatGameMode::RespondToPing(int32 Id, const FString& ResponderId, bool bOnIt)
{
    if (!PingBoard.Respond(Id, ResponderId, bOnIt)) { return false; }
    TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetNumberField(TEXT("id"), Id);
    Data->SetStringField(TEXT("responder_id"), ResponderId);
    Data->SetStringField(TEXT("response"), bOnIt ? TEXT("on_it") : TEXT("busy"));
    Emit(TEXT("ping.responded"), Data);
    PublishPings();
    return true;
}

void ADMCombatGameMode::StepPings()
{
    if (PingBoard.Pings.IsEmpty()) { return; }
    TArray<FDMPingEnded> Ended;
    PingBoard.Step(CombatTick, [this](const FDMPing& Ping)
    {
        switch (Ping.Kind)
        {
        case EDMPingKind::Enemy: case EDMPingKind::Focus: case EDMPingKind::Ignore:
        { const ADMCombatant* Target = FindCombatant(Ping.TargetId); return !Target || Target->IsDown() || !DMVision::CanSee(FindCombatant(Ping.AuthorId),Target); }
        case EDMPingKind::Help:
            // A completed revive fulfils Help pings on the ally where it happens (StepCombat); here only a vanished ally ends one.
            return FindCombatant(Ping.TargetId) == nullptr;
        case EDMPingKind::Pickup:
            for (TActorIterator<ADMScroungePickup> It(GetWorld()); It; ++It)
            { if (FVector::DistSquared2D(It->GetActorLocation(), Ping.Location) <= FMath::Square(160.f)) { return false; } }
            return true;
        default: return false;
        }
    }, Ended);
    EmitPingEnded(Ended);
    PublishPings();
}

void ADMCombatGameMode::PublishPings()
{
    ADMGameState* Projection = GetGameState<ADMGameState>();
    if (!Projection) { return; }
    TArray<FDMPing> Live;
    for (const FDMPing& Ping : PingBoard.Pings) { if (Ping.IsLive(CombatTick)) { Live.Add(Ping); } }
    Projection->SetPings(Live);
}

void ADMCombatGameMode::EmitPingEnded(const TArray<FDMPingEnded>& Ended)
{
    for (const FDMPingEnded& Entry : Ended)
    {
        TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
        Data->SetNumberField(TEXT("id"), Entry.Ping.Id);
        Data->SetStringField(TEXT("reason"), FDMPingBoard::EndName(Entry.Reason));
        Emit(TEXT("ping.ended"), Data);
    }
}

// ---- Tuning metrics

namespace
{
    // Measurement thresholds for the tactical counters below. They describe play; they never steer it, so they
    // are deliberately plain constants rather than tunable weights the search could optimise against.
    constexpr float MetricIsolationRadius = 700.f;
    constexpr float MetricPerilHealth = 50.f;
    // A wire marker never sets Radius (it keeps the 220 default, which means nothing for a segment), so the
    // band is named here. Deliberately wider than the kit's 35-unit crossing slack: the stagger and the
    // follow-up that finishes the victim land slightly off the line, and those still count as the wire working.
    constexpr float MetricWireBand = 120.f;

    /** True when Point lies inside this marker's actual shape, not just its bounding radius. */
    bool MarkerCovers(const ADMAbilityMarker& Marker, const FVector& Point)
    {
        switch (Marker.Shape)
        {
        case EDMMarkerShape::Cone:
            return DMKitRules::PointInCone(Marker.GetActorLocation(), Marker.Direction, Marker.HalfAngle, Marker.Length, Point);
        case EDMMarkerShape::Wire:
            return DMKitRules::DistanceToSegment2D(Marker.GetActorLocation(), Marker.WireEnd, Point) <= MetricWireBand;
        default:
            return FVector::Dist2D(Marker.GetActorLocation(), Point) <= Marker.Radius;
        }
    }
}

void ADMCombatGameMode::NoteDamage(const ADMCombatant& From, const ADMCombatant& To, float Damage, float PoolBefore)
{
    if (!(Damage > 0)) { return; }
    if (To.bIsEnemy) { Metrics.DamageToEnemies += Damage; } else { Metrics.DamageToInvestigators += Damage; }
    Metrics.DamageDealtBy.FindOrAdd(From.EntityId) += Damage;
    // Everything past the target's remaining pool bought nothing: the cost of piling onto a target already dead.
    Metrics.OverkillDamage += FMath::Max(0.f, Damage - FMath::Max(0.f, PoolBefore));
}
void ADMCombatGameMode::NoteDowned(const ADMCombatant& Target)
{
    if (Target.bIsEnemy && !Target.ActorHasTag(TEXT("Destructible")))
    { auto* Corpse=GetWorld()->SpawnActor<ADMCorpse>(); Corpse->Initialize(&Target); }
    if (Target.bIsEnemy)
    {
        ++Metrics.EnemyKills;
        for (ADMCombatant* Hero : Combatants)
        { if (Hero && !Hero->bIsEnemy) { Hero->Progression->Award(TEXT("kill:") + Target.EntityId, 5); } }
        if (UsesEncounterLayout())
        {
            for (int32 Camp=0; Camp<DMEncounterLayout::CampCount; ++Camp)
            {
                bool bCleared=true;
                for (int32 Member=0; Member<DMEncounterLayout::CampSize; ++Member)
                {
                    const auto* Enemy=FindCombatant(FString::Printf(TEXT("enemy.%d"),Camp*DMEncounterLayout::CampSize+Member));
                    bCleared &= Enemy && Enemy->IsDown();
                }
                if(bCleared)
                { for(ADMCombatant* Hero : Combatants)
                  { if(Hero && !Hero->bIsEnemy) { Hero->Progression->Award(FString::Printf(TEXT("encounter:camp:%d"),Camp),250); } } }
            }
        }
        for (TActorIterator<ADMAbilityMarker> It(GetWorld()); It; ++It)
        {
            if (It->bHostile || It->bSpirit || !It->IsArmed() || !MarkerCovers(**It, Target.GetActorLocation())) { continue; }
            ++Metrics.TrapGroundKills;
            break;
        }
        return;
    }
    ++Metrics.InvestigatorDowns;
    // A downed companion is not doing what they said they were doing.
    SquadBoard.ClearAuthor(Target.EntityId);
    for (const ADMCombatant* Ally : Combatants)
    {
        if (!Ally || Ally == &Target || Ally->bIsEnemy || Ally->IsDown()) { continue; }
        if (FVector::Dist2D(Ally->GetActorLocation(), Target.GetActorLocation()) <= MetricIsolationRadius) { return; }
    }
    ++Metrics.IsolatedDowns;
}

void ADMCombatGameMode::StepMetrics()
{
    TMap<const ADMCombatant*, int32> OnTarget;
    for (const ADMCombatant* Actor : Combatants)
    {
        if (!Actor || Actor->bIsEnemy || Actor->IsDown()) { continue; }
        for (TActorIterator<ADMAbilityMarker> It(GetWorld()); It; ++It)
        {
            if (!It->bHostile || It->BoundTarget || !MarkerCovers(**It, Actor->GetActorLocation())) { continue; }
            ++Metrics.HazardTicks;
            break;
        }
        const ADMCombatant* Focus = Actor->GetAttackTarget();
        if (!Focus || Focus->IsDown()) { continue; }
        ++OnTarget.FindOrAdd(Focus);
        // Peeling: hitting the enemy that is beating a teammate who is in real trouble.
        const ADMCombatant* Victim = Focus->GetAttackTarget();
        if (Victim && Victim != Actor && !Victim->bIsEnemy && !Victim->IsDown() && Victim->Health() < MetricPerilHealth)
        { ++Metrics.PeelTicks; }
    }
    Metrics.FocusDistinctTicks += OnTarget.Num();
    for (const TPair<const ADMCombatant*, int32>& Pair : OnTarget) { Metrics.FocusHolderTicks += Pair.Value; }

    // A companion in peril whose attacker nobody is answering.
    for (const ADMCombatant* Victim : Combatants)
    {
        if (!Victim || Victim->bIsEnemy || Victim->IsDown() || Victim->Health() >= MetricPerilHealth) { continue; }
        for (const ADMCombatant* Attacker : Combatants)
        {
            if (!Attacker || !Attacker->bIsEnemy || Attacker->IsDown() || Attacker->GetAttackTarget() != Victim) { continue; }
            bool bAnswered = false;
            for (const ADMCombatant* Ally : Combatants)
            {
                if (!Ally || Ally->bIsEnemy || Ally->IsDown() || Ally == Victim) { continue; }
                if (Ally->GetAttackTarget() == Attacker) { bAnswered = true; break; }
            }
            if (!bAnswered) { ++Metrics.UnansweredPerilTicks; }
        }
    }
}
void ADMCombatGameMode::NoteRevive() { ++Metrics.Revives; }
void ADMCombatGameMode::NoteSignature() { ++Metrics.SignatureActivations; }
void ADMCombatGameMode::NoteQCast() { ++Metrics.QCasts; }
void ADMCombatGameMode::NoteKitCast(EDMKitSlot Slot, const ADMCombatant& Caster)
{
    const TCHAR* Key = nullptr;
    switch (Slot)
    {
    case EDMKitSlot::W: ++Metrics.WCasts; Key = TEXT("w"); break;
    case EDMKitSlot::E: ++Metrics.ECasts; Key = TEXT("e"); break;
    case EDMKitSlot::R: ++Metrics.RCasts; Key = TEXT("r"); break;
    default: return;
    }
    ++Metrics.KitCastsBy.FindOrAdd(Caster.EntityId + TEXT(".") + Key);
}

void ADMCombatGameMode::LogResult(const FString& Outcome)
{
    int32 InvestigatorsUp = 0, EnemiesUp = 0, Investigators = 0, Enemies = 0;
    for (ADMCombatant* Actor : Combatants)
    {
        if (!Actor) { continue; }
        if (Actor->bIsEnemy) { ++Enemies; EnemiesUp += Actor->IsDown() ? 0 : 1; }
        else { ++Investigators; InvestigatorsUp += Actor->IsDown() ? 0 : 1; }
    }
    TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("outcome"), Outcome);
    Data->SetNumberField(TEXT("tick"), CombatTick);
    Data->SetNumberField(TEXT("investigators_standing"), InvestigatorsUp);
    Data->SetNumberField(TEXT("enemies_standing"), EnemiesUp);
    Data->SetNumberField(TEXT("enemy_total"), Enemies);
    Data->SetNumberField(TEXT("investigator_total"), Investigators);
    Data->SetNumberField(TEXT("damage_to_investigators"), Metrics.DamageToInvestigators);
    Data->SetNumberField(TEXT("damage_to_enemies"), Metrics.DamageToEnemies);
    Data->SetNumberField(TEXT("investigator_downs"), Metrics.InvestigatorDowns);
    Data->SetNumberField(TEXT("enemy_kills"), Metrics.EnemyKills);
    Data->SetNumberField(TEXT("revives"), Metrics.Revives);
    Data->SetNumberField(TEXT("signatures"), Metrics.SignatureActivations);
    Data->SetNumberField(TEXT("q_casts"), Metrics.QCasts);
    Data->SetNumberField(TEXT("w_casts"), Metrics.WCasts);
    Data->SetNumberField(TEXT("e_casts"), Metrics.ECasts);
    Data->SetNumberField(TEXT("r_casts"), Metrics.RCasts);
    Data->SetNumberField(TEXT("pings"), Metrics.PingsCreated);
    Data->SetNumberField(TEXT("overkill_damage"), Metrics.OverkillDamage);
    Data->SetNumberField(TEXT("focus_distinct_ticks"), Metrics.FocusDistinctTicks);
    Data->SetNumberField(TEXT("focus_holder_ticks"), Metrics.FocusHolderTicks);
    Data->SetNumberField(TEXT("hazard_ticks"), Metrics.HazardTicks);
    Data->SetNumberField(TEXT("peel_ticks"), Metrics.PeelTicks);
    Data->SetNumberField(TEXT("unanswered_peril_ticks"), Metrics.UnansweredPerilTicks);
    Data->SetNumberField(TEXT("trap_ground_kills"), Metrics.TrapGroundKills);
    Data->SetNumberField(TEXT("isolated_downs"), Metrics.IsolatedDowns);
    TSharedPtr<FJsonObject> By = MakeShared<FJsonObject>();
    TArray<FString> Keys;
    Metrics.DamageDealtBy.GetKeys(Keys);
    Keys.Sort();
    for (const FString& Key : Keys) { By->SetNumberField(Key, Metrics.DamageDealtBy[Key]); }
    Data->SetObjectField(TEXT("damage_by"), By);
    TSharedPtr<FJsonObject> Casts = MakeShared<FJsonObject>();
    Keys.Reset();
    Metrics.KitCastsBy.GetKeys(Keys);
    Keys.Sort();
    for (const FString& Key : Keys) { Casts->SetNumberField(Key, Metrics.KitCastsBy[Key]); }
    Data->SetObjectField(TEXT("kit_casts_by"), Casts);
    FString Json;
    // Condensed: tune_ai.py reads the whole document from this one log line.
    FJsonSerializer::Serialize(Data, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json));
    UE_LOG(LogTemp, Display, TEXT("DREAD_AI_RESULT %s"), *Json);
}

ADMCombatant* ADMCombatGameMode::SpawnEncounterActor(const FString& Id, FVector Location, float HP, float Damage, bool bStatic)
{
    if (!HasAuthority() || Id.IsEmpty() || FindCombatant(Id) || Location.ContainsNaN() || HP <= 0 || Damage < 0) { return nullptr; }
    FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* A = GetWorld()->SpawnActor<ADMCombatant>(Location,FRotator::ZeroRotator,P);
    if (!A) { return nullptr; }
    A->InitializeCombatant(Id,true,HP,Damage); A->bHumanEnemy = false; Combatants.Add(A);
    if (bStatic) { A->Tags.Add(TEXT("Objective")); A->Tags.Add(TEXT("Destructible")); A->GetCharacterMovement()->DisableMovement(); A->SetAttackHold(true); }
    else { AttachBot(A); }
    auto Spawn=MakeShared<FJsonObject>();
    Spawn->SetStringField(TEXT("entity_id"),A->EntityId); Spawn->SetStringField(TEXT("team"),TEXT("enemy"));
    Spawn->SetNumberField(TEXT("health"),A->Health()); Spawn->SetNumberField(TEXT("shield"),A->Shield());
    Spawn->SetNumberField(TEXT("attack_damage"),A->AttackDamage); Spawn->SetStringField(TEXT("display_name"),A->DisplayName());
    Spawn->SetNumberField(TEXT("attack_range"),A->GetAttackRange()); Spawn->SetNumberField(TEXT("attack_interval_ticks"),A->AttackIntervalTicks);
    Spawn->SetNumberField(TEXT("initial_next_attack_tick"),A->NextAttackTick); Spawn->SetStringField(TEXT("control"),TEXT("bot"));
    Emit(TEXT("combat.spawned"),Spawn);
    return A;
}
void ADMCombatGameMode::StepCombat()
{
    if (!bCombatActive) { return; }
    if (bNetworkTest)
    {
        int32 Humans = 0;
        for (ADMCombatant* Actor : Combatants) { Humans += Actor->IsPlayerControlled() ? 1 : 0; }
        if (Humans < 2) { return; }
    }
    ++CombatTick;
#if !UE_BUILD_SHIPPING
    if (bNetworkTest && CombatTick == 1)
    {
        auto* Drop=GetWorld()->SpawnActor<ADMRelicDrop>(); Drop->Initialize(TEXT("network:shared_relic"));
        // Distinct private test cues exercise real owner delivery, without claiming family content.
        for (ADMCombatant* A : Combatants)
        {
            if (A->bIsEnemy) { continue; }
            A->Progression->Award(TEXT("network:progression_probe"),300);
            A->Relics->Acquire(Drop->Roll.Relic==EDMRelic::Gloves ? EDMRelic::Rosary : EDMRelic::Gloves,TEXT("network:held_relic:")+A->EntityId);
            A->MadnessCore->Add(25, TEXT("network_privacy_probe"));
            A->MadnessCore->Manifest(TEXT("test.private.") + A->EntityId,
                TEXT("Private cue for ") + A->EntityId, 1, 10000);
        }
    }
    if (FParse::Param(FCommandLine::Get(),TEXT("DMSmugglerSoak")) && CombatTick > 1800)
    { UE_LOG(LogTemp,Error,TEXT("DREAD_SMUGGLER_SOAK_TIMEOUT")); LogResult(TEXT("timeout")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
    if (bSwampTest && CombatTick>1800)
    { UE_LOG(LogTemp,Error,TEXT("DREAD_SWAMP_TIMEOUT")); LogResult(TEXT("timeout")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
#endif
    for (ADMCombatant* Actor : Combatants) { Actor->SpiritProtection = 0; Actor->SpiritSlow = 0; }
    // Settle the ping board before any bot reads it this tick.
    StepPings();
    // Same for squad intent: claims older than their lifetime go before anyone reads the board.
    SquadBoard.Step(CombatTick);
    // Expire control for the whole roster before persistent abilities or signatures resolve.
    for (ADMCombatant* Actor : Combatants) { Actor->StepControl(CombatTick); }
    for (TActorIterator<ADMRecoverySupply> It(GetWorld()); It; ++It) { It->Step(); }
    for (TActorIterator<ADMCorruption> It(GetWorld()); It; ++It) { It->Step(CombatTick); }
    for (TActorIterator<ADMGrowthNetwork> It(GetWorld()); It; ++It) { It->Step(CombatTick); }
    const auto BeforeSpawns=Combatants;
    for (ADMCombatant* Actor : BeforeSpawns)
    { if (auto* Minion=Actor->FindComponentByClass<UDMShubMinion>()) { Minion->Step(CombatTick); } }
    for (TActorIterator<ADMShubEncounter> It(GetWorld()); It; ++It) { It->Step(CombatTick); }
    if (!bCombatActive) { return; }
    for (TActorIterator<ADMObjective> It(GetWorld()); It; ++It) { It->Step(CombatTick); }
    for (TActorIterator<ADMRelicDrop> It(GetWorld()); It; ++It) { It->Step(CombatTick); }
    for (ADMCombatant* Actor : Combatants) { Actor->Primary->Step(CombatTick); Actor->Kit->Step(CombatTick); Actor->Smuggler->Step(*this); Actor->Swamp->Step(CombatTick); }
    if (ADMGameState* Projection = GetGameState<ADMGameState>()) { Projection->SetCombatTick(CombatTick); }
    for (ADMCombatant* Actor : Combatants)
    {
        Actor->StepInvestigator(CombatTick);
        for (ADMCombatant* Subject : Combatants)
        { Actor->Investigator->UpdateSpiritLocation(Subject->EntityId, Subject->GetActorLocation()); }
    }
    // Stable roster ordering: decisions -> revive channels -> ability resolution -> end condition.
    // Character movement/physics remains Unreal-authoritative, outside a replay guarantee.
    for (ADMCombatant* Actor : Combatants)
    { if (ADMSquadController* Bot = Cast<ADMSquadController>(Actor->GetController())) { Bot->Think(*this); } }
    StepMetrics();
    for (ADMCombatant* Actor : Combatants)
    { if (!Actor->bIsEnemy) { Actor->SetReviveChannel(FString(), 0); } }
    for (ADMCombatant* Actor : Combatants)
    {
        const auto* Channel = Revives.Find(Actor);
        if (!Channel) { continue; }
        ADMCombatant* Ally = Channel->Key.Get();
        const int32 Started = Channel->Value;
        if (!Ally || !Ally->IsDown() || Actor->IsDown() || Actor->IsStunned() || Actor->IsRestrained() || Actor->LastDamageTick >= Started
            || FVector::DistSquared(Actor->GetActorLocation(), Ally->GetActorLocation()) > FMath::Square(160.f))
        { Revives.Remove(Actor); continue; }
        const int32 Duration = FMath::Max(1,FMath::CeilToInt(Ally->Progression->ReviveTicks(ReviveDurationTicks(Ally->GrievousCount), CombatTick)*Ally->Relics->ReviveFactor()*Actor->Relics->ReviveFactor()));
        Ally->SetReviveChannel(Actor->EntityId, static_cast<float>(CombatTick - Started) / Duration);
        if (CombatTick - Started >= Duration)
        {
            if (Actor->Revive(Ally))
            {
                ++RevivesCompleted;
                // A revived ally fulfils every Help ping on them.
                TArray<FDMPingEnded> Ended;
                PingBoard.Step(CombatTick, [Ally](const FDMPing& Ping) { return Ping.Kind == EDMPingKind::Help && Ping.TargetId == Ally->EntityId; }, Ended);
                if (Ended.Num()) { EmitPingEnded(Ended); PublishPings(); }
            }
            Revives.Remove(Actor);
        }
    }
    for (ADMCombatant* Actor : Combatants)
    { if (!Revives.Contains(Actor)) { Actor->TryAttack(Actor->GetAttackTarget()); } }
    bool bInvestigatorsUp = false, bEnemiesUp = false;
    for (ADMCombatant* Actor : Combatants)
    {
        if (!Actor->IsDown()) { if (Actor->bIsEnemy) { bEnemiesUp = true; } else { bInvestigatorsUp = true; } }
    }
    if (Village)
    { if (!bInvestigatorsUp) { CompleteCombat(false); return; } Village->StepScenario(); return; }
    if (bBossOutcome) { return; }
    if (UsesEncounterLayout())
    {
        const EDMWaveAction Action = SmugglerWave.Advance(bInvestigatorsUp,bEnemiesUp,CombatTick);
        if (Action == EDMWaveAction::Announce)
        { for (ADMCombatant* Hero : Combatants)
          { if (Hero && !Hero->bIsEnemy) { Hero->Progression->Award(TEXT("encounter:occupation"), 350); } }
          auto Data = MakeShared<FJsonObject>(); Data->SetStringField(TEXT("stage"),TEXT("boss_incoming")); Data->SetNumberField(TEXT("arrival_tick"),SmugglerWave.ArrivalTick); Emit(TEXT("encounter.wave"),Data); }
        else if (Action == EDMWaveAction::SpawnPosse) { SpawnBossPosse(); }
        else if (Action == EDMWaveAction::Victory || Action == EDMWaveAction::Defeat) { CompleteCombat(Action == EDMWaveAction::Victory); }
        PublishEncounter();
    }
    else if (!bInvestigatorsUp || !bEnemiesUp) { CompleteCombat(bInvestigatorsUp); }
    else if (!SmokeOutcome.IsEmpty() && CombatTick > 1000)
    {
        UE_LOG(LogTemp, Error, TEXT("DREAD_COMBAT_SMOKE_TIMEOUT"));
        FPlatformMisc::RequestExitWithStatus(false, 1);
    }
}

void ADMCombatGameMode::CompleteCombat(bool bVictory)
{
    if (bVictory && bCombatActive)
    { for (ADMCombatant* Hero : Combatants)
      { if (Hero && !Hero->bIsEnemy) { Hero->Progression->Award(TEXT("encounter:victory"), 650); } } }
    bCombatActive = false;
    GetWorldTimerManager().ClearTimer(CombatTimer);
    for (ADMCombatant* Actor : Combatants) { Actor->Swamp->Stop(); Actor->Smuggler->Cancel(); Actor->StopGoal(); Actor->Primary->CancelChannel(); Actor->Primary->ReleaseClinch(); Actor->Kit->Cancel(true); Actor->GetCharacterMovement()->StopMovementImmediately(); }
    FinishRun(bVictory);
    LogResult(bVictory ? TEXT("victory") : TEXT("defeat"));
    OnEncounterComplete(bVictory);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(),TEXT("DMSmugglerSoak")))
    { UE_LOG(LogTemp,Display,TEXT("DREAD_SMUGGLER_SOAK_COMPLETE outcome=%s tick=%d roster=%d"),bVictory?TEXT("victory"):TEXT("defeat"),CombatTick,Combatants.Num()); FPlatformMisc::RequestExit(false); }
    if (bSwampTest && !bNetworkTest)
    { UE_LOG(LogTemp,Display,TEXT("DREAD_SWAMP_COMPLETE outcome=%s tick=%d roster=%d"),bVictory?TEXT("victory"):TEXT("defeat"),CombatTick,Combatants.Num()); FPlatformMisc::RequestExit(false); }
#endif
    if (bNetworkTest)
    {
        TSharedRef<FJsonObject> Expected = MakeShared<FJsonObject>();
        Expected->SetNumberField(TEXT("actor_count"),Combatants.Num());
        Expected->SetBoolField(TEXT("vision_probe"),bSwampTest);
        for (ADMCombatant* Actor : Combatants)
        {
            Expected->SetNumberField(Actor->EntityId + TEXT(".health"), Actor->Health());
            Expected->SetNumberField(Actor->EntityId + TEXT(".shield"), Actor->Shield());
            Expected->SetStringField(Actor->EntityId + TEXT(".name"), Actor->DisplayName());
            Expected->SetStringField(Actor->EntityId + TEXT(".resources"), Actor->Investigator->ResourceSummary());
            Expected->SetStringField(Actor->EntityId + TEXT(".progression"), Actor->Progression->Summary());
            Expected->SetStringField(Actor->EntityId + TEXT(".primary"), Actor->Primary->ReplicationSummary());
            Expected->SetStringField(Actor->EntityId + TEXT(".injuries"), Actor->Injuries->Summary());
            Expected->SetStringField(Actor->EntityId + TEXT(".resolve"), Actor->Resolve->ReplicationSummary());
            Expected->SetStringField(Actor->EntityId + TEXT(".kit"), Actor->Kit->ReplicationSummary());
            Expected->SetStringField(Actor->EntityId + TEXT(".relics"), Actor->Relics->Summary());
            Expected->SetNumberField(Actor->EntityId + TEXT(".relic_capacity"), Actor->Relics->Capacity);
            Expected->SetNumberField(Actor->EntityId + TEXT(".swamp_role"),static_cast<uint8>(Actor->Swamp->Capture().Role));
            Expected->SetBoolField(Actor->EntityId + TEXT(".swamp_faction"),Actor->bSwampThing);
        }
        Expected->SetStringField(TEXT("phase"), bVictory ? TEXT("Victory") : TEXT("Defeat"));
        for (TActorIterator<ADMRelicDrop> It(GetWorld());It;++It)
        { if (It->Roll.AwardId==TEXT("network:shared_relic")) { Expected->SetStringField(TEXT("relic_winner"),It->Roll.Winner); } }
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        { if (auto* Player=Cast<ADMCombatPlayerController>(It->Get()))
          { if (auto* A=Cast<ADMCombatant>(Player->GetPawn()))
            {
                auto PrivateExpected=MakeShared<FJsonObject>(); PrivateExpected->Values=Expected->Values; int32 VisibleCount=0;
                for (ADMCombatant* Subject : Combatants)
                { if (DMVision::CanSee(A,Subject)) { ++VisibleCount; }
                  else { for (const TCHAR* Field : {TEXT("health"),TEXT("shield"),TEXT("name"),TEXT("resources"),TEXT("progression"),TEXT("primary"),TEXT("injuries"),TEXT("resolve"),TEXT("kit"),TEXT("relics"),TEXT("relic_capacity"),TEXT("swamp_role"),TEXT("swamp_faction")}) { PrivateExpected->RemoveField(Subject->EntityId+TEXT(".")+Field); } } }
                PrivateExpected->SetNumberField(TEXT("actor_count"),VisibleCount); FString Json; FJsonSerializer::Serialize(PrivateExpected,TJsonWriterFactory<>::Create(&Json));
                Player->PublishVision(); Player->ClientVerifyCombatState(Json); Player->ClientMadness(A->MadnessCore->View()); Player->ClientVerifyMadness(A->MadnessCore->View());
            } } }
        UE_LOG(LogTemp, Display, TEXT("DREAD_NETWORK_SERVER_COMPLETE"));
        FTimerHandle ExitTimer;
        GetWorldTimerManager().SetTimer(ExitTimer, [] { FPlatformMisc::RequestExit(false); }, 8.f, false);
    }
    if (!SmokeOutcome.IsEmpty())
    {
        const bool bPassed = Combatants.Num() == 7 && bGuardChecksPassed && (bVictory == (SmokeOutcome != TEXT("Defeat")))
            && (SmokeOutcome != TEXT("Revive") || RevivesCompleted > 0);
        UE_LOG(LogTemp, Display, TEXT("DREAD_COMBAT_SMOKE_%s outcome=%s tick=%d"), bPassed ? TEXT("PASSED") : TEXT("FAILED"), bVictory ? TEXT("victory") : TEXT("defeat"), CombatTick);
        FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
    }
}

void ADMCombatGameMode::SpawnBossPosse()
{
    for (int32 I=0; I<DMEncounterLayout::PosseCount; ++I)
    {
        FActorSpawnParameters P; P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* A=GetWorld()->SpawnActor<ADMCombatant>(DMEncounterLayout::PossePosition(I),FRotator(0,180,0),P);
        A->Smuggler->Initialize(DMEncounterLayout::PosseRole(I));
        A->InitializeCombatant(FString::Printf(TEXT("enemy.posse.%d"),I),true,A->Smuggler->BaseHealth(),A->Smuggler->BaseDamage());
        Combatants.Add(A); AttachBot(A);
        CastChecked<ADMSquadController>(A->GetController())->ConfigureEncounter(4,A->GetActorLocation(),false,0);
        auto Data=MakeShared<FJsonObject>(); Data->SetStringField(TEXT("entity_id"),A->EntityId);
        Data->SetStringField(TEXT("team"),TEXT("enemy")); Data->SetStringField(TEXT("control"),TEXT("bot"));
        Data->SetStringField(TEXT("display_name"),A->DisplayName()); Data->SetNumberField(TEXT("health"),A->Health());
        Data->SetNumberField(TEXT("shield"),0); Data->SetNumberField(TEXT("attack_damage"),A->AttackDamage);
        Data->SetNumberField(TEXT("attack_range"),A->GetAttackRange()); Data->SetNumberField(TEXT("attack_interval_ticks"),A->AttackIntervalTicks);
        Data->SetNumberField(TEXT("initial_next_attack_tick"),A->NextAttackTick); Emit(TEXT("combat.spawned"),Data);
    }
    auto Data=MakeShared<FJsonObject>(); Data->SetStringField(TEXT("stage"),TEXT("boss_and_posse"));
    Data->SetNumberField(TEXT("count"),DMEncounterLayout::PosseCount); Emit(TEXT("encounter.wave"),Data);
}
void ADMCombatGameMode::PublishEncounter()
{
    if (!UsesEncounterLayout()) { return; }
    FString Text;
    switch (SmugglerWave.Stage) {
    case EDMSmugglerWave::Occupation: Text=TEXT("Clear all camps and the patrol"); break;
    case EDMSmugglerWave::Arrival: Text=FString::Printf(TEXT("Gang Boss arrives in %.1fs"),FMath::Max(0,SmugglerWave.ArrivalTick-CombatTick)*.1f); break;
    case EDMSmugglerWave::Finale: Text=TEXT("Final wave: defeat boss and posse"); break;
    default: Text=TEXT("Smuggler encounter complete"); break; }
    if (auto* Projection=GetGameState<ADMGameState>()) { Projection->SetEncounterObjective(Text); }
}
