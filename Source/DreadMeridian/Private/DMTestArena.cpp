#include "DMTestArena.h"
#include "DMCombatant.h"
#include "DMEncounterLayout.h"
#include "DMEditorPlaySelection.h"
#include "DMVision.h"
#include "DMGameState.h"
#include "DMSquadController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Canvas.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerInput.h"
#include "PlaytraceCaptureSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"

namespace DMArena
{
FString EnemyName(EDMArenaEnemy Type)
{
    static const TCHAR* Names[] = {TEXT("Gunman"),TEXT("Bruiser"),TEXT("Lookout"),TEXT("Bomber"),TEXT("Gang Boss"),TEXT("Crawler"),TEXT("Lurker"),TEXT("Spitter"),TEXT("Grasper"),TEXT("Old Thing")};
    return Type < EDMArenaEnemy::Count ? Names[static_cast<uint8>(Type)] : TEXT("Invalid");
}
FString HeroName(EDMInvestigator Kind)
{
    static const TCHAR* Names[] = {TEXT("None"),TEXT("Sapper"),TEXT("Photographer"),TEXT("Medium"),TEXT("Smuggler")};
    return Names[FMath::Clamp(static_cast<int32>(Kind),0,4)];
}
TArray<EDMArenaEnemy> Preset(int32 Index)
{
    TArray<EDMArenaEnemy> Result;
    if (Index >= 0 && Index < 3)
    { for (int32 I=0; I<3; ++I) { Result.Add(static_cast<EDMArenaEnemy>(static_cast<uint8>(DMEncounterLayout::EnemyRole(Index*3+I))-1)); } }
    else if (Index == 3) { Result = {EDMArenaEnemy::Gunman, EDMArenaEnemy::Lookout}; }
    else if (Index == 4) { Result = {EDMArenaEnemy::GangBoss,EDMArenaEnemy::Gunman,EDMArenaEnemy::Bruiser,EDMArenaEnemy::Lookout,EDMArenaEnemy::Bomber}; }
    else if (Index == 5) { Result = {EDMArenaEnemy::Crawler,EDMArenaEnemy::Lurker,EDMArenaEnemy::Spitter,EDMArenaEnemy::Grasper}; }
    return Result;
}
FString PresetName(int32 Index)
{
    static const TCHAR* Names[] = {TEXT("Smuggler camp 1"),TEXT("Smuggler camp 2"),TEXT("Smuggler camp 3"),TEXT("Patrol pair"),TEXT("Boss posse"),TEXT("Swamp group")};
    return Names[FMath::Clamp(Index,0,5)];
}
FVector FormationOffset(int32 Index, int32 Count)
{
    const int32 Columns = FMath::Min(4,Count);
    return FVector((Index/Columns)*150.f, ((Index%Columns)-(FMath::Min(Columns,Count-(Index/Columns)*Columns)-1)*.5f)*150.f, 0);
}
}

bool FDMArenaConfig::IsValid() const
{
    if (Player < EDMInvestigator::Sapper || Player > EDMInvestigator::Smuggler || Companions.Num()>3 || Placements.Num()>DMArena::MaxEnemies) { return false; }
    TSet<EDMInvestigator> Seen; Seen.Add(Player);
    for (auto Kind : Companions)
    { if (Kind<EDMInvestigator::Sapper || Kind>EDMInvestigator::Smuggler || Seen.Contains(Kind)) { return false; } Seen.Add(Kind); }
    for (const auto& P : Placements)
    { if (P.Type>=EDMArenaEnemy::Count || P.Position.ContainsNaN() || FMath::Abs(P.Position.X)>2800 || FMath::Abs(P.Position.Y)>2300 || !FMath::IsNearlyEqual(P.Position.Z,95.,1.) || P.Batch<0) { return false; } }
    return true;
}

ADMTestArenaGeometry::ADMTestArenaGeometry()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    auto Block = [&](const TCHAR* Name, FVector Position, FVector Size)
    {
        auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>(Name); Mesh->SetupAttachment(RootComponent);
        Mesh->SetStaticMesh(Cube.Object); Mesh->SetRelativeLocation(Position); Mesh->SetRelativeScale3D(Size);
    };
    Block(TEXT("Floor"),FVector(0,0,-25),FVector(60,50,.5));
    Block(TEXT("WestWall"),FVector(-3000,0,100),FVector(.3,50,2));
    Block(TEXT("EastWall"),FVector(3000,0,100),FVector(.3,50,2));
    Block(TEXT("NorthWall"),FVector(0,2500,100),FVector(60,.3,2));
    Block(TEXT("SouthWall"),FVector(0,-2500,100),FVector(60,.3,2));
    Block(TEXT("CoverA"),FVector(200,-1550,100),FVector(5,1,2));
    Block(TEXT("CoverB"),FVector(1500,1300,100),FVector(1,5,2));
    for (int32 I=0;I<24;++I)
    {
        const float Angle=I*2*PI/24;
        auto* Edge=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ReedsBoundary%d"),I));
        Edge->SetupAttachment(RootComponent); Edge->SetStaticMesh(Cube.Object);
        Edge->SetRelativeLocation(FVector(1700+450*FMath::Cos(Angle),-1700+450*FMath::Sin(Angle),3));
        Edge->SetRelativeScale3D(FVector(.7,.15,.02)); Edge->SetRelativeRotation(FRotator(0,FMath::RadiansToDegrees(Angle)+90,0));
        Edge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    auto* Light=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Light")); Light->SetupAttachment(RootComponent); Light->SetRelativeRotation(FRotator(-60,-30,0)); Light->SetIntensity(5); Light->ForwardShadingPriority=1;
    auto* Fill=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Fill")); Fill->SetupAttachment(RootComponent); Fill->SetRelativeRotation(FRotator(-35,150,0)); Fill->SetIntensity(2); Fill->SetCastShadows(false);
    auto* Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("ReedLabel")); Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(1200,-1800,5)); Label->SetRelativeRotation(FRotator(90,0,0)); Label->SetWorldSize(65); Label->SetText(FText::FromString(TEXT("REEDS / AMBUSH")));
}
void ADMTestArenaGeometry::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) { auto* Reeds=GetWorld()->SpawnActor<ADMVisionArea>(FVector(1700,-1700,0),FRotator::ZeroRotator); if (Reeds) { Reeds->Radius=450; Reeds->SourceId=TEXT("arena.reeds"); } }
}

ADMTestArenaGameMode::ADMTestArenaGameMode()
{
    PlayerControllerClass=ADMTestArenaController::StaticClass(); HUDClass=ADMTestArenaHUD::StaticClass();
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true;
}
void ADMTestArenaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName,Options,ErrorMessage);
    if (GetNetMode()!=NM_Standalone || UGameplayStatics::HasOption(Options,TEXT("listen"))) { ErrorMessage=TEXT("Test Arena supports one local player only. Use Standalone PIE, not a network session."); return; }
#if UE_BUILD_SHIPPING
    ErrorMessage=TEXT("Test Arena is available in developer builds only.");
#endif
    auto* Session=GetGameInstance()->GetSubsystem<UDMArenaSession>();
    if (Session->bRestore && Session->Config.IsValid()) { Config=Session->Config; Session->bRestore=false; }
    else
    {
        FString Preferred=Super::PreferredInvestigator();
        for (int32 I=1;I<=4;++I) { if (DMArena::HeroName(static_cast<EDMInvestigator>(I)).Equals(Preferred,ESearchCase::IgnoreCase)) { Config.Player=static_cast<EDMInvestigator>(I); } }
        FParse::Value(FCommandLine::Get(),TEXT("DMSeed="),Config.Seed);
    }
    bProbe=FParse::Param(FCommandLine::Get(),TEXT("DMArenaProbe"));
}
void ADMTestArenaGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{ Super::PreLogin(Options,Address,UniqueId,ErrorMessage); ErrorMessage=TEXT("Test Arena is local-only; multiplayer connections are disabled."); }
void ADMTestArenaGameMode::ConfigureCaptureMetadata(const TSharedRef<FJsonObject>& Metadata)
{
    Super::ConfigureCaptureMetadata(Metadata);
    Metadata->SetStringField(TEXT("scenario_id"),TEXT("test-arena-v1")); Metadata->SetStringField(TEXT("run_kind"),TEXT("test_arena"));
    Metadata->SetStringField(TEXT("capture_version"),TEXT("0.16.0")); Metadata->SetStringField(TEXT("test_profile"),TEXT("developer_arena"));
    Metadata->SetNumberField(TEXT("investigator_slots"),1+Config.Companions.Num()); Metadata->SetNumberField(TEXT("initial_bot_count"),Config.Companions.Num());
    Metadata->SetStringField(TEXT("controlled_investigator"),DMArena::HeroName(Config.Player));
    Metadata->SetNumberField(TEXT("arena_config_version"),1);
    TArray<TSharedPtr<FJsonValue>> Party;
    for (auto K : Config.Companions) { Party.Add(MakeShared<FJsonValueString>(DMArena::HeroName(K))); }
    Metadata->SetArrayField(TEXT("arena_companions"),Party);
}
void ADMTestArenaGameMode::StartPlay()
{
    Super::StartPlay();
    if (GetNetMode()!=NM_Standalone) { Feedback=TEXT("Test Arena requires Standalone mode."); return; }
    if (!TActorIterator<ADMTestArenaGeometry>(GetWorld())) { GetWorld()->SpawnActor<ADMTestArenaGeometry>(); }
    TArray<EDMInvestigator> Party={Config.Player}; Party.Append(Config.Companions);
    for (int32 I=0;I<Party.Num();++I)
    { CreateFixtureCombatant(FString::Printf(TEXT("investigator.%d"),static_cast<int32>(Party[I])-1),FVector(-650,(I-1.5f)*120,95),Party[I],EDMSmuggler::None); }
    for (const auto& P : Config.Placements) { if (!SpawnPlacement(P)) { Feedback=TEXT("Could not restore an enemy. Reset or clear the setup."); } }
    ActivateFixture(); RecordSetup(TEXT("restored"));
    UGameplayStatics::SetGamePaused(this,true);
    ProbeAt=FPlatformTime::Seconds();
}
bool ADMTestArenaGameMode::SpawnPlacement(const FDMArenaPlacement& P)
{
    const uint8 Type=static_cast<uint8>(P.Type);
    auto* A=CreateFixtureCombatant(FString::Printf(TEXT("arena.enemy.%d"),NextEntity++),P.Position,EDMInvestigator::None,Type<5 ? static_cast<EDMSmuggler>(Type+1) : EDMSmuggler::None,Type>=5 ? Type-4 : 0);
    if (!A) { return false; }
    if (auto* Bot=Cast<ADMSquadController>(A->GetController())) { Bot->ConfigureEncounter(P.Batch,P.Position,false,0); }
    PlacedActors.Add(A); return true;
}
bool ADMTestArenaGameMode::ValidatePlacement(const TArray<EDMArenaEnemy>& Types, FVector Center, TArray<FDMArenaPlacement>& Out, FString& Error) const
{
    Out.Reset();
    if (!HasAuthority() || !CanPlace() || Types.IsEmpty() || Types.Num()+Config.Placements.Num()>DMArena::MaxEnemies || Center.ContainsNaN())
    { Error=TEXT("Place 1-32 enemies while in Setup or Paused (32 total maximum)."); return false; }
    const int32 Batch=Config.Placements.IsEmpty() ? 0 : Config.Placements.Last().Batch+1;
    for (int32 I=0;I<Types.Num();++I)
    {
        const FVector P=FVector(Center.X,Center.Y,95)+DMArena::FormationOffset(I,Types.Num());
        if (Types[I]>=EDMArenaEnemy::Count || FMath::Abs(P.X)>2800 || FMath::Abs(P.Y)>2300)
        { Error=TEXT("The entire formation must fit inside the arena."); Out.Reset(); return false; }
        FCollisionQueryParams Query(SCENE_QUERY_STAT(ArenaPlacement),false);
        if (GetWorld()->OverlapBlockingTestByChannel(P,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(40,89),Query))
        { Error=TEXT("Formation overlaps cover, a wall, or a combatant."); Out.Reset(); return false; }
        for (const ADMCombatant* A : GetCombatants())
        { if (IsValid(A) && FVector::DistSquared2D(A->GetActorLocation(),P)<FMath::Square(85.f)) { Error=TEXT("Formation overlaps a combatant."); Out.Reset(); return false; } }
        // Keep authored placements disjoint as well, so Reset can recreate the complete setup safely.
        for (const auto& Existing : Config.Placements)
        { if (FVector::DistSquared2D(Existing.Position,P)<FMath::Square(85.f)) { Error=TEXT("Formation overlaps an existing reset position."); Out.Reset(); return false; } }
        for (int32 Slot=0;Slot<4;++Slot)
        { if (FVector::DistSquared2D(FVector(-650,(Slot-1.5f)*120,95),P)<FMath::Square(85.f)) { Error=TEXT("Keep the investigator starting area clear for party changes."); Out.Reset(); return false; } }
        FDMArenaPlacement Entry; Entry.Type=Types[I]; Entry.Position=P; Entry.Batch=Batch; Out.Add(Entry);
    }
    Error.Reset(); return true;
}
bool ADMTestArenaGameMode::Place(const TArray<EDMArenaEnemy>& Types, FVector Center)
{
    TArray<FDMArenaPlacement> New;
    if (!ValidatePlacement(Types,Center,New,Feedback)) { return false; }
    const int32 Before=PlacedActors.Num();
    for (const auto& P : New)
    {
        if (!SpawnPlacement(P))
        {
            while (PlacedActors.Num()>Before) { RemoveFixtureCombatant(PlacedActors.Pop()); }
            Feedback=TEXT("Spawn failed; the entire placement was rolled back."); RecordSetup(TEXT("placement_failed")); return false;
        }
    }
    Config.Placements.Append(New); Feedback=FString::Printf(TEXT("Placed %d enemies. %d / 32 total."),New.Num(),Config.Placements.Num()); RecordSetup(TEXT("placed")); return true;
}
void ADMTestArenaGameMode::Undo(bool bAll)
{
    if (Phase!=EDMArenaPhase::Setup || Config.Placements.IsEmpty()) { return; }
    const int32 Batch=Config.Placements.Last().Batch;
    while (!Config.Placements.IsEmpty() && (bAll || Config.Placements.Last().Batch==Batch))
    { Config.Placements.Pop(); RemoveFixtureCombatant(PlacedActors.Pop()); }
    Feedback=TEXT("Setup updated."); RecordSetup(bAll ? TEXT("cleared") : TEXT("undone"));
}
void ADMTestArenaGameMode::ChangeParty(EDMInvestigator Player, TArray<EDMInvestigator> Companions)
{
    if (!HasAuthority() || bReloading) { return; }
    Companions.Remove(Player); Companions.Sort([](auto A,auto B){return A<B;});
    FDMArenaConfig Candidate=Config; Candidate.Player=Player; Candidate.Companions=Companions;
    if (!Candidate.IsValid()) { Feedback=TEXT("Choose one player and up to three different companions."); return; }
    Reload(Candidate);
}
void ADMTestArenaGameMode::FightOrPause()
{
    if (!HasAuthority() || bReloading) { return; }
    if (Phase==EDMArenaPhase::Setup && Config.Placements.IsEmpty()) { Feedback=TEXT("Place at least one enemy before starting."); return; }
    if (Phase==EDMArenaPhase::Setup || Phase==EDMArenaPhase::Paused)
    { Phase=EDMArenaPhase::Fighting; UGameplayStatics::SetGamePaused(this,false); }
    else if (Phase==EDMArenaPhase::Fighting) { Phase=EDMArenaPhase::Paused; UGameplayStatics::SetGamePaused(this,true); }
    else { return; }
    RecordSetup(TEXT("phase_changed"));
}
bool ADMTestArenaGameMode::HandleCustomOutcome(bool bInvestigatorsUp, bool bEnemiesUp)
{
    if (Phase==EDMArenaPhase::Fighting && (!bInvestigatorsUp || !bEnemiesUp))
    {
        Phase=bInvestigatorsUp ? EDMArenaPhase::Victory : EDMArenaPhase::Defeat;
        RecordSetup(TEXT("completed")); FinishRun(bInvestigatorsUp); UGameplayStatics::SetGamePaused(this,true);
        Feedback=bInvestigatorsUp ? TEXT("All enemies defeated. Reset Test to repeat.") : TEXT("Party defeated. Reset Test to repeat.");
    }
    return true;
}
void ADMTestArenaGameMode::ResetTest()
{ Reload(Config); }
void ADMTestArenaGameMode::Reload(const FDMArenaConfig& NextConfig)
{
    if (!HasAuthority() || bReloading || !NextConfig.IsValid()) { return; }
    bReloading=true;
    auto* Session=GetGameInstance()->GetSubsystem<UDMArenaSession>(); Session->Config=NextConfig; Session->bRestore=true;
    RecordSetup(TEXT("reset")); GetGameInstance()->GetSubsystem<UPlaytraceCaptureSubsystem>()->EndCapture(TEXT("aborted"));
    UGameplayStatics::SetGamePaused(this,false);
    UGameplayStatics::OpenLevel(this,FName(*UGameplayStatics::GetCurrentLevelName(this,true)));
}
void ADMTestArenaGameMode::RecordSetup(const FString& Action)
{
    auto Data=MakeShared<FJsonObject>(); Data->SetStringField(TEXT("action"),Action);
    Data->SetStringField(TEXT("phase"),StaticEnum<EDMArenaPhase>()->GetNameStringByValue(static_cast<int64>(Phase)));
    Data->SetStringField(TEXT("player"),DMArena::HeroName(Config.Player)); Data->SetNumberField(TEXT("seed"),Config.Seed);
    TArray<TSharedPtr<FJsonValue>> Entries;
    for (const auto& P : Config.Placements)
    {
        auto Entry=MakeShared<FJsonObject>(); Entry->SetStringField(TEXT("type"),DMArena::EnemyName(P.Type));
        Entry->SetNumberField(TEXT("x"),P.Position.X); Entry->SetNumberField(TEXT("y"),P.Position.Y); Entry->SetNumberField(TEXT("batch"),P.Batch);
        Entries.Add(MakeShared<FJsonValueObject>(Entry));
    }
    Data->SetArrayField(TEXT("placements"),Entries); Emit(TEXT("arena.setup"),Data);
}
void ADMTestArenaGameMode::Tick(float DeltaSeconds)
{ Super::Tick(DeltaSeconds); if (bProbe && !bReloading) { Probe(); } }

void ADMTestArenaGameMode::Probe()
{
#if !UE_BUILD_SHIPPING
    auto* Session=GetGameInstance()->GetSubsystem<UDMArenaSession>();
    auto Check=[&](bool bOK,const TCHAR* What)
    { if (!bOK) { UE_LOG(LogTemp,Error,TEXT("DREAD_ARENA_FAILED %s"),What); bProbe=false; FPlatformMisc::RequestExitWithStatus(false,1); } return bOK; };
    if (!Check(FPlatformTime::Seconds()-ProbeAt<30,TEXT("stage timeout"))) { return; }
    auto* PC=GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) { return; }
    auto* Hero=CastChecked<ADMCombatant>(PC->GetPawn());
    if (Session->ProbeStage==0)
    {
        if (FPlatformTime::Seconds()-ProbeAt<(FParse::Param(FCommandLine::Get(),TEXT("RenderOffscreen")) ? 3.0 : .3)) { return; }
        if (!Check(Phase==EDMArenaPhase::Setup && UGameplayStatics::IsGamePaused(this) && GetCombatTick()==0 && GetCombatants().Num()==1,TEXT("solo paused startup"))) { return; }
        FightOrPause(); if (!Check(Phase==EDMArenaPhase::Setup,TEXT("empty fight rejected"))) { return; }
        TArray<EDMArenaEnemy> All; for (uint8 I=0;I<10;++I) { All.Add(static_cast<EDMArenaEnemy>(I)); }
        if (!Check(!Place(All,FVector(2790,0,95)) && Config.Placements.IsEmpty(),TEXT("out of bounds atomic rejection"))) { return; }
        if (!Check(Place(All,FVector(200,0,95)) && GetCombatants().Num()==11,TEXT("ten types placed"))) { return; }
        if (!Check(!Place({EDMArenaEnemy::Gunman},FVector(200,-225,95)) && GetCombatants().Num()==11,TEXT("overlap atomic rejection"))) { return; }
        TArray<EDMArenaEnemy> Excess; Excess.Init(EDMArenaEnemy::Gunman,33);
        if (!Check(!Place(Excess,FVector(1200,0,95)),TEXT("count limit"))) { return; }
        Undo(); if (!Check(GetCombatants().Num()==1 && Config.Placements.IsEmpty(),TEXT("undo removes whole formation"))) { return; }
        Place({EDMArenaEnemy::Bomber},FVector(-200,-180,95));
        Session->ProbeStage=1; FightOrPause(); ProbeAt=FPlatformTime::Seconds();
    }
    else if (Session->ProbeStage==1 && GetCombatTick()>=5)
    {
        FightOrPause(); ProbeTick=GetCombatTick(); ProbeAt=FPlatformTime::Seconds(); Session->ProbeStage=2;
        if (!Check(Place({EDMArenaEnemy::Crawler},FVector(450,400,95)),TEXT("paused spawning"))) { return; }
    }
    else if (Session->ProbeStage==2 && FPlatformTime::Seconds()-ProbeAt>.4)
    {
        if (!Check(GetCombatTick()==ProbeTick && UGameplayStatics::IsGamePaused(this),TEXT("pause freezes logical combat"))) { return; }
        const int32 Before=GetCombatants().Num(); Undo(true);
        if (!Check(GetCombatants().Num()==Before,TEXT("cannot clear after fight"))) { return; }
        Session->ProbeStage=3;
        ChangeParty(EDMInvestigator::Photographer,{EDMInvestigator::Sapper,EDMInvestigator::Medium,EDMInvestigator::Smuggler});
    }
    else if (Session->ProbeStage==3 && FPlatformTime::Seconds()-ProbeAt>.3)
    {
        if (!Check(GetCombatTick()==0 && GetCombatants().Num()==6 && Config.Placements.Num()==2 && Hero->Investigator->Kind==EDMInvestigator::Photographer,TEXT("reload restores placements and four-person party"))) { return; }
        for (const ADMCombatant* A : GetCombatants()) { if (!Check(A->Health()>0 && A->NextAttackTick==0,TEXT("fresh health and cooldown"))) { return; } }
        FightOrPause();
        for (ADMCombatant* A : GetCombatants()) { if (A->bIsEnemy) { Hero->DealCombatDamage(A,10000,TEXT("arena.probe")); } }
        Session->ProbeStage=4; ProbeAt=FPlatformTime::Seconds();
    }
    else if (Session->ProbeStage==4 && Phase==EDMArenaPhase::Victory)
    {
        if (!Check(UGameplayStatics::IsGamePaused(this),TEXT("victory stays frozen"))) { return; }
        Session->ProbeStage=5; ChangeParty(EDMInvestigator::Medium,{});
    }
    else if (Session->ProbeStage==5 && FPlatformTime::Seconds()-ProbeAt>.3)
    {
        if (!Check(GetCombatants().Num()==3 && Hero->Investigator->Kind==EDMInvestigator::Medium && GetCombatTick()==0,TEXT("second reset and medium possession"))) { return; }
        FightOrPause(); GetCombatants()[1]->DealCombatDamage(Hero,10000,TEXT("arena.probe")); Session->ProbeStage=6;
    }
    else if (Session->ProbeStage==6 && Phase==EDMArenaPhase::Defeat)
    { Session->ProbeStage=7; ChangeParty(EDMInvestigator::Smuggler,{}); }
    else if (Session->ProbeStage==7 && FPlatformTime::Seconds()-ProbeAt>.3)
    {
        if (!Check(Hero->Investigator->Kind==EDMInvestigator::Smuggler && GetCombatTick()==0 && GetCombatants().Num()==3,TEXT("third reset and smuggler possession"))) { return; }
        RecordSetup(TEXT("probe_complete")); GetGameInstance()->GetSubsystem<UPlaytraceCaptureSubsystem>()->EndCapture(TEXT("aborted"));
        UE_LOG(LogTemp,Display,TEXT("DREAD_ARENA_SMOKE_PASSED")); bProbe=false; FPlatformMisc::RequestExitWithStatus(false,0);
    }
#endif
}

ADMTestArenaController::ADMTestArenaController()
{ PrimaryActorTick.bTickEvenWhenPaused=true; bShouldPerformFullTickWhenPaused=true; bShowMouseCursor=true; }
bool ADMTestArenaController::GroundPoint(FVector& Point) const
{
    FVector Origin,Direction;
    if (!DeprojectMousePositionToWorld(Origin,Direction) || Direction.Z>=-.001) { return false; }
    const double T=-Origin.Z/Direction.Z;
    if (T<0) { return false; } Point=Origin+Direction*T; Point.Z=95; return true;
}
bool ADMTestArenaController::InputKey(const FInputKeyEventArgs& Params)
{
    auto* Mode=GetWorld()->GetAuthGameMode<ADMTestArenaGameMode>();
    auto* ArenaHUD=Cast<ADMTestArenaHUD>(GetHUD());
    float X=0,Y=0; GetMousePosition(X,Y);
    const bool bPanel=ArenaHUD && ArenaHUD->OverPanel(X,Y);
    if (Params.Event==IE_Pressed && Params.Key==EKeys::LeftMouseButton)
    {
        if (bPanel) { ArenaHUD->Click(X,Y); return true; }
        if (bPlacing && Mode) { FVector P; if (GroundPoint(P) && Mode->Place(Selection,P)) { bPlacing=false; } return true; }
    }
    if (Params.Event==IE_Pressed && Params.Key==EKeys::RightMouseButton && bPlacing) { bPlacing=false; return true; }
    if (Mode && (Mode->Phase!=EDMArenaPhase::Fighting || bPanel || bPlacing)) { return true; }
    return Super::InputKey(Params);
}
void ADMTestArenaController::PlayerTick(float DeltaSeconds)
{
    auto* Mode=GetWorld()->GetAuthGameMode<ADMTestArenaGameMode>();
    if (Mode && GetPawn())
    {
        if (Mode->Phase!=EDMArenaPhase::Fighting)
        {
            if (!SetupCamera)
            {
                SetupCamera=GetWorld()->SpawnActor<ACameraActor>(FVector(1250,0,9000),FRotator(-90,-90,0));
                SetupCamera->GetCameraComponent()->SetFieldOfView(65);
                SetupCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
                SetupCamera->GetCameraComponent()->bOverrideAspectRatioAxisConstraint=true;
                SetupCamera->GetCameraComponent()->SetAspectRatioAxisConstraint(AspectRatio_MaintainXFOV);
            }
            int32 W=1280,H=800; GetViewportSize(W,H);
            // PIE can tick its controller before the viewport receives its first resize.
            // Keep the overview transform finite until its first real viewport size arrives.
            if (W<=0 || H<=0) { W=1280; H=800; }
            const float PanelWidth=350*FMath::Min(H/800.f,1.5f);
            const float Width=FMath::Max(6500.f*W/FMath::Max(1.f,W-PanelWidth),5500.f*W/FMath::Max(1,H));
            const float Height=Width/(2*FMath::Tan(FMath::DegreesToRadians(32.5f)));
            SetupCamera->SetActorLocation(FVector(Width*PanelWidth/(2*FMath::Max(1,W)),0,Height));
            SetViewTarget(SetupCamera);
        }
        else if (GetViewTarget()!=GetPawn()) { SetViewTarget(GetPawn()); }
    }
    float X=0,Y=0; GetMousePosition(X,Y);
    const auto* ArenaHUD=Cast<ADMTestArenaHUD>(GetHUD());
    if (Mode && (Mode->Phase!=EDMArenaPhase::Fighting || bPlacing || (ArenaHUD && ArenaHUD->OverPanel(X,Y))))
    { if (PlayerInput) { PlayerInput->FlushPressedKeys(); } APlayerController::PlayerTick(DeltaSeconds); }
    else { Super::PlayerTick(DeltaSeconds); }
}

bool ADMTestArenaHUD::OverPanel(float X,float Y) const
{
    const auto* Mode=GetWorld()->GetAuthGameMode<ADMTestArenaGameMode>();
    const float Height=Mode && Mode->Phase==EDMArenaPhase::Fighting ? 100.f : 790.f;
    return X>=Left && X<=Left+350*Scale && Y>=8*Scale && Y<=Height*Scale;
}
void ADMTestArenaHUD::Button(const FString& Label,float X,float Y,float W,TFunction<void()> Action,bool bEnabled)
{
    const FBox2D Bounds(FVector2D(Left+X*Scale,Y*Scale),FVector2D(Left+(X+W)*Scale,(Y+25)*Scale));
    DrawRect(bEnabled ? FLinearColor(.16f,.23f,.27f,.97f) : FLinearColor(.09f,.10f,.11f,.97f),Bounds.Min.X,Bounds.Min.Y,W*Scale,25*Scale);
    DrawText(Label,bEnabled ? FLinearColor(.93f,.88f,.72f) : FLinearColor(.38f,.38f,.38f),Bounds.Min.X+5*Scale,Bounds.Min.Y+4*Scale,nullptr,Scale);
    if (bEnabled) { Buttons.Add({Bounds,MoveTemp(Action)}); }
}
bool ADMTestArenaHUD::Click(float X,float Y)
{
    // Copy the action: a level reload may replace the HUD after this call.
    for (const auto& B : Buttons) { if (B.Bounds.IsInside(FVector2D(X,Y))) { auto Action=B.Action; Action(); return true; } }
    return OverPanel(X,Y);
}
void ADMTestArenaHUD::DrawHUD()
{
    auto* Mode=GetWorld()->GetAuthGameMode<ADMTestArenaGameMode>(); auto* Player=Cast<ADMTestArenaController>(GetOwningPlayerController());
    if (Mode && Mode->Phase==EDMArenaPhase::Fighting) { Super::DrawHUD(); } else { AHUD::DrawHUD(); }
    if (!Canvas || !Mode || !Player) { return; }
    if (Mode->Phase!=EDMArenaPhase::Fighting)
    {
        for (const ADMCombatant* Actor : Mode->GetCombatants())
        {
            if (!Actor || Actor->IsHidden() || (Mode->Phase==EDMArenaPhase::Setup && Actor->bIsEnemy)) { continue; }
            FVector2D Screen;
            if (Player->ProjectWorldLocationToScreen(Actor->GetActorLocation(),Screen))
            { DrawText(Actor->DisplayName(),Actor->bIsEnemy ? FLinearColor(1,.6f,.4f) : FLinearColor(.5f,1,.7f),Screen.X+8,Screen.Y,nullptr,.85f); }
        }
        if (Mode->Phase==EDMArenaPhase::Setup)
        {
            // Authored setup markers remain visible even for enemies concealed by normal
            // gameplay vision. They expose only positions the developer explicitly placed.
            for (const auto& Placement : Mode->Config.Placements)
            {
                FVector2D Screen;
                if (Player->ProjectWorldLocationToScreen(Placement.Position,Screen))
                { DrawRect(FLinearColor(1,.45f,.2f,.7f),Screen.X-4,Screen.Y-4,8,8); DrawText(DMArena::EnemyName(Placement.Type),FLinearColor(1,.6f,.4f),Screen.X+8,Screen.Y,nullptr,.85f); }
            }
        }
        // These marks describe authored terrain, not perception or hidden combat state.
        FVector2D ReedsCenter;
        if (Player->ProjectWorldLocationToScreen(FVector(1700,-1700,8),ReedsCenter))
        {
            DrawText(TEXT("REEDS"),FLinearColor(.4f,.85f,.55f),ReedsCenter.X-20,ReedsCenter.Y,nullptr,.9f);
            FVector2D Previous;
            for (int32 I=0;I<=32;++I)
            {
                const float Angle=I*2*PI/32; FVector2D Point;
                if (Player->ProjectWorldLocationToScreen(FVector(1700+450*FMath::Cos(Angle),-1700+450*FMath::Sin(Angle),8),Point))
                { if (I>0) { DrawLine(Previous.X,Previous.Y,Point.X,Point.Y,FLinearColor(.4f,.85f,.55f,.8f),1.5f); } Previous=Point; }
            }
        }
    }
    Scale=FMath::Min(Canvas->ClipY/800.f,1.5f); Left=Canvas->ClipX-360*Scale; Buttons.Reset();
    DrawRect(FLinearColor(.025f,.035f,.045f,.97f),Left,8*Scale,350*Scale,(Mode->Phase==EDMArenaPhase::Fighting ? 92 : 782)*Scale);
    auto Text=[&](const FString& Line,float Y){DrawText(Line,FLinearColor(.85f,.88f,.9f),Left+10*Scale,Y*Scale,nullptr,Scale);};
    Text(TEXT("TEST ARENA | provisional tuning"),18);
    Text(FString::Printf(TEXT("%s | Seed %d | %d / 32 enemies"),*StaticEnum<EDMArenaPhase>()->GetNameStringByValue(static_cast<int64>(Mode->Phase)),Mode->Config.Seed,Mode->Config.Placements.Num()),40);
    const bool bDone=Mode->Phase==EDMArenaPhase::Victory || Mode->Phase==EDMArenaPhase::Defeat;
    Button(Mode->Phase==EDMArenaPhase::Fighting ? TEXT("Pause") : Mode->Phase==EDMArenaPhase::Paused ? TEXT("Resume") : TEXT("Fight"),10,65,155,[Mode,Player]{Player->bPlacing=false; Mode->FightOrPause();},!bDone);
    Button(TEXT("Reset Test"),175,65,165,[Mode]{Mode->ResetTest();});
    if (Mode->Phase==EDMArenaPhase::Fighting) { return; }
    Text(TEXT("Player (changes reload the test)"),105);
    for (int32 I=1;I<=4;++I)
    {
        const auto K=static_cast<EDMInvestigator>(I); const float Y=128+(I-1)*29;
        Button((Mode->Config.Player==K ? TEXT("(*) ") : TEXT("( ) "))+DMArena::HeroName(K),10,Y,170,[Mode,K]{Mode->ChangeParty(K,Mode->Config.Companions);});
        Button(Mode->Config.Player==K ? TEXT("Player") : Mode->Config.Companions.Contains(K) ? TEXT("[x] Companion") : TEXT("[ ] Companion"),190,Y,150,[Mode,K]{auto C=Mode->Config.Companions; if (C.Contains(K)) { C.Remove(K); } else { C.Add(K); } Mode->ChangeParty(Mode->Config.Player,C);},Mode->Config.Player!=K);
    }
    Text(TEXT("Enemy type / custom count"),253);
    for (int32 I=0;I<10;++I)
    {
        const float Y=275+I*28;
        Button(DMArena::EnemyName(static_cast<EDMArenaEnemy>(I)),10,Y,180,[this,I,Player]{Counts.Init(0,10); Counts[I]=1; Player->Selection={static_cast<EDMArenaEnemy>(I)};},Mode->CanPlace());
        Button(TEXT("-"),200,Y,28,[this,I]{Counts[I]=FMath::Max(0,Counts[I]-1);},Mode->CanPlace());
        DrawText(FString::FromInt(Counts[I]),FLinearColor::White,Left+240*Scale,(Y+4)*Scale,nullptr,Scale);
        Button(TEXT("+"),285,Y,45,[this,I]{if (Counts[I]<32) { ++Counts[I]; }},Mode->CanPlace());
    }
    Button(TEXT("<"),10,562,30,[this]{PresetIndex=(PresetIndex+5)%6;},Mode->CanPlace());
    Button(DMArena::PresetName(PresetIndex),47,562,248,[this,Player]{Player->Selection=DMArena::Preset(PresetIndex); Counts.Init(0,10); for (auto T : Player->Selection) { ++Counts[static_cast<uint8>(T)]; }},Mode->CanPlace());
    Button(TEXT(">"),302,562,30,[this]{PresetIndex=(PresetIndex+1)%6;},Mode->CanPlace());
    Button(Player->bPlacing ? TEXT("Click floor; right-click cancels") : TEXT("Place selected types / counts"),10,599,330,[this,Player]{Player->Selection.Reset(); for (int32 I=0;I<10;++I) { for (int32 N=0;N<Counts[I];++N) { Player->Selection.Add(static_cast<EDMArenaEnemy>(I)); } } Player->bPlacing=!Player->Selection.IsEmpty();},Mode->CanPlace());
    Button(TEXT("Undo Last Placement"),10,636,190,[Mode]{Mode->Undo();},Mode->Phase==EDMArenaPhase::Setup);
    Button(TEXT("Clear Enemies"),210,636,130,[Mode]{Mode->Undo(true);},Mode->Phase==EDMArenaPhase::Setup);
    Text(TEXT("Choose a preset by clicking its name."),674);
    Text(TEXT("Cover and reeds are at the arena edges."),696);
    // Wrap feedback into short lines so placement errors remain visible at small resolutions.
    FString Remaining=Mode->Feedback;
    for (int32 Row=0;Row<3 && !Remaining.IsEmpty();++Row)
    { int32 Cut=FMath::Min(45,Remaining.Len()); if (Cut<Remaining.Len()) { while (Cut>0 && Remaining[Cut]!=TCHAR(' ')) { --Cut; } if (!Cut) { Cut=45; } } Text(Remaining.Left(Cut),720+Row*19); Remaining=Remaining.Mid(Cut).TrimStart(); }
    if (Player->bPlacing)
    {
        Player->Selection.Reset();
        for (int32 I=0;I<10;++I) { for (int32 N=0;N<Counts[I];++N) { Player->Selection.Add(static_cast<EDMArenaEnemy>(I)); } }
        FVector Center; TArray<FDMArenaPlacement> Preview; FString Error;
        if (Player->GroundPoint(Center))
        {
            const bool bValid=Mode->ValidatePlacement(Player->Selection,Center,Preview,Error);
            for (int32 I=0;I<FMath::Min(Player->Selection.Num(),32);++I)
            {
                const FVector P=Center+DMArena::FormationOffset(I,Player->Selection.Num());
                FVector2D Screen; if (Player->ProjectWorldLocationToScreen(P,Screen))
                { DrawRect(bValid ? FLinearColor(0,.8f,.45f,.7f) : FLinearColor(.9f,.15f,.1f,.7f),Screen.X-15,Screen.Y-15,30,30); DrawText(DMArena::EnemyName(Player->Selection[I]),FLinearColor::White,Screen.X+18,Screen.Y,nullptr,.8f*Scale); }
            }
            if (!bValid) { DrawText(Error,FLinearColor(1,.5f,.3f),20,70,nullptr,Scale); }
        }
    }
    if (FParse::Param(FCommandLine::Get(),TEXT("DMArenaProbe")) && FParse::Param(FCommandLine::Get(),TEXT("RenderOffscreen")))
    {
        const int32 Stage=GetGameInstance()->GetSubsystem<UDMArenaSession>()->ProbeStage;
        if (Stage!=LastShotStage && (Stage==0 || Stage==2 || Stage==3) && ++ShotFrames>=5)
        { LastShotStage=Stage; ShotFrames=0; FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots"),FString::Printf(TEXT("arena-stage-%d.png"),Stage)),true,false); }
    }
}
