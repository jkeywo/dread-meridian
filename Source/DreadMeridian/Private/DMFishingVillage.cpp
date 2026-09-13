#include "DMFishingVillage.h"
#include "DMObjective.h"
#include "DMVision.h"
#include "DMBossArena.h"
#include "DMShubEncounter.h"
#include "DMElderOne.h"
#include "DMCombatant.h"
#include "DMGameState.h"
#include "DMRecoverySupply.h"
#include "DMSquadController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ADMFishingVillageGameMode::ADMFishingVillageGameMode() { bFishingVillage=true; }
ADMFishingVillage::ADMFishingVillage()
{
    bReplicates=true; bAlwaysRelevant=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Village")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    auto Block=[&](const TCHAR* Name,FVector P,FVector Size,bool Solid=true)
    {
        auto* C=CreateDefaultSubobject<UStaticMeshComponent>(Name); C->SetupAttachment(RootComponent);
        C->SetStaticMesh(Cube.Object); C->SetRelativeLocation(P); C->SetRelativeScale3D(Size/100);
        const FString N(Name);
        const TCHAR* Group=N==TEXT("MarshGround") ? TEXT("Ground") : N==TEXT("BasinBed") ? TEXT("Mud") : N.StartsWith(TEXT("Reeds")) ? TEXT("Reeds") : (N==TEXT("FloodedBasin") || N==TEXT("DeepMarsh")) ? TEXT("Water") : (N.Contains(TEXT("Hut")) || N.Contains(TEXT("Store")) || N.Contains(TEXT("Boathouse")) || N.Contains(TEXT("Causeway")) || N.Contains(TEXT("Approach")) || N.Contains(TEXT("Jetty"))) ? TEXT("Wood") : TEXT("Stone");
        if (auto* Material=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/DreadMeridian/Maps/VillageMaterials/M_Village%s.M_Village%s"),Group,Group))) { C->SetMaterial(0,Material); }
        Geometry.Add(C);
        C->SetCollisionEnabled(Solid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        if (Solid && P.Z>0) { Obstacles.Add(FBox(P-Size*.5f,P+Size*.5f)); }
        return C;
    };
    Block(TEXT("MarshGround"),FVector(0,0,-30),FVector(6000,5000,60));
    // The flooded basin blocks traversal until the pump completes. Ground underneath is permanent.
    Flood=Block(TEXT("FloodedBasin"),Basin()+FVector(0,0,55),FVector(1850,1850,110));
    Block(TEXT("WestBoundary"),FVector(-2980,0,120),FVector(40,5000,240));
    Block(TEXT("EastBoundary"),FVector(2980,0,120),FVector(40,5000,240));
    Block(TEXT("NorthBoundary"),FVector(0,2480,120),FVector(6000,40,240));
    Block(TEXT("SouthBoundary"),FVector(0,-2480,120),FVector(6000,40,240));
    Block(TEXT("VillageStore"),FVector(-1900,-700,140),FVector(300,350,280));
    Block(TEXT("FishingHut"),FVector(-1250,-1500,110),FVector(250,260,220));
    Block(TEXT("Boathouse"),FVector(-300,-1500,150),FVector(400,300,300));
    Block(TEXT("Church"),FVector(-500,1650,220),FVector(350,400,440));
    Block(TEXT("LighthouseTower"),FVector(-2250,1550,350),FVector(150,150,700));
    Block(TEXT("PumpMachinery"),FVector(50,250,110),FVector(200,200,220));
    Block(TEXT("DeepMarsh"),FVector(-1400,450,40),FVector(450,700,80));
    for (int32 I=0;I<5;++I) { Block(*FString::Printf(TEXT("Grave%d"),I),FVector(-800+I*110,2150,25),FVector(50,90,50)); }
    // Raised-looking, flush walkable planks distinguish fast routes without stairs.
    Block(TEXT("VillageCauseway"),FVector(-1250,-1100,2),FVector(2900,180,4),false);
    Block(TEXT("ChurchCauseway"),FVector(-950,600,2),FVector(180,3200,4),false);
    Block(TEXT("PumpCauseway"),FVector(250,-450,2),FVector(180,1500,4),false);
    Block(TEXT("BasinSouthApproach"),FVector(1250,-650,2),FVector(2200,180,4),false);
    Block(TEXT("Jetty"),FVector(-2200,-1500,2),FVector(160,800,4),false);
    Block(TEXT("BasinBed"),Basin()+FVector(0,0,1),FVector(1850,1850,2),false);
    for (int32 I=0;I<3;++I)
    {
        const FVector ReedSites[]={FVector(-1100,-300,3),FVector(300,650,3),FVector(900,-1750,3)};
        Block(*FString::Printf(TEXT("Reeds%d"),I),ReedSites[I],FVector(540,540,6),false);
        const FVector Ruins[]={FVector(1200,200,10),FVector(1700,200,10),FVector(1450,650,10)};
        Block(*FString::Printf(TEXT("RitualStone%d"),I),Ruins[I],FVector(150,150,20),false);
    }
    auto Sign=[&](const TCHAR* Name,FVector P)
    {
        auto* T=CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Sign_%s"),Name)); T->SetupAttachment(RootComponent);
        T->SetRelativeLocation(P+FVector(0,0,8)); T->SetRelativeRotation(FRotator(90,180,0));
        T->SetText(FText::FromString(Name)); T->SetWorldSize(55); T->SetHorizontalAlignment(EHTA_Center); T->SetTextRenderColor(FColor(220,190,120));
    };
    Sign(TEXT("FISHING VILLAGE"),FVector(-1750,-1000,0)); Sign(TEXT("BOATHOUSE"),FVector(-300,-1750,0));
    Sign(TEXT("CHURCH & GRAVEYARD"),FVector(-400,1300,0)); Sign(TEXT("LIGHTHOUSE"),FVector(-2000,1800,0));
    Sign(TEXT("WATERWORKS"),FVector(0,-200,0)); Sign(TEXT("RITUAL BASIN"),Basin());
    auto* Light=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Daylight")); Light->SetupAttachment(RootComponent);
    Light->SetRelativeRotation(FRotator(-65,-30,0)); Light->SetIntensity(5); Light->SetForwardShadingPriority(1);
    auto* Fill=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Fill")); Fill->SetupAttachment(RootComponent);
    Fill->SetRelativeRotation(FRotator(-35,150,0)); Fill->SetIntensity(2); Fill->SetCastShadows(false);
}
ADMObjective* ADMFishingVillage::SpawnObjective(const FString& Id,FVector Location)
{
    auto* O=GetWorld()->SpawnActor<ADMObjective>();
    if (!O || !O->ConfigureAuthored(Id,Location,0,TEXT("village.")+Id)) { if (O) { O->Destroy(); } return nullptr; }
    O->bAlwaysRelevant=true; return O;
}
void ADMFishingVillage::SpawnCore()
{
    const TCHAR* Ids[]={TEXT("impossible_catch"),TEXT("waterworks"),TEXT("main_pump"),TEXT("counter_sigil")};
    const FVector Sites[]={FVector(-2300,-1400,20),FVector(-600,-150,20),FVector(300,0,20),FVector(1200,200,20)};
    if (CoreStage<4) { Core=SpawnObjective(Ids[CoreStage],Sites[CoreStage]); }
}
void ADMFishingVillage::StartScenario()
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!HasAuthority() || !M || bStarted) { return; } bStarted=true;
    SpawnCore();
    Disruptions={SpawnObjective(TEXT("bell_sequence"),FVector(-600,1150,20)),SpawnObjective(TEXT("counter_ritualist"),FVector(-1500,1000,20)),SpawnObjective(TEXT("marsh_idols"),FVector(300,-1900,20))};
    Optional={SpawnObjective(TEXT("lighthouse"),FVector(-2400,1150,20)),SpawnObjective(TEXT("surgery"),FVector(-1600,-500,20)),SpawnObjective(TEXT("smuggler_cache"),FVector(-250,-1150,20))};
    struct FEnemy { FVector At; EDMSwampThing Swamp; EDMSmuggler Smuggler; };
    const FEnemy Enemies[]={
        {FVector(-1900,-1750,95),EDMSwampThing::Crawler,EDMSmuggler::None},
        {FVector(-1100,-300,95),EDMSwampThing::Lurker,EDMSmuggler::None},
        {FVector(300,650,95),EDMSwampThing::Spitter,EDMSmuggler::None},
        {FVector(900,-1750,95),EDMSwampThing::Grasper,EDMSmuggler::None},
        {FVector(2150,1950,95),EDMSwampThing::OldThing,EDMSmuggler::None},
        {FVector(-100,-1000,95),EDMSwampThing::None,EDMSmuggler::Gunman},
        {FVector(100,-1250,95),EDMSwampThing::None,EDMSmuggler::Bruiser},
        {FVector(-350,950,95),EDMSwampThing::None,EDMSmuggler::Lookout},
        {FVector(-100,1100,95),EDMSwampThing::None,EDMSmuggler::Bomber}};
    for (int32 I=0;I<UE_ARRAY_COUNT(Enemies);++I)
    {
        const auto& E=Enemies[I]; auto* A=M->SpawnEncounterActor(FString::Printf(TEXT("village.enemy.%d"),I),E.At,E.Swamp==EDMSwampThing::OldThing ? 350:120,8);
        if (!A) { continue; } A->bRequiresVision=true;
        if (E.Swamp!=EDMSwampThing::None) { A->Swamp->Initialize(E.Swamp); }
        else { A->bHumanEnemy=true; A->Smuggler->Initialize(E.Smuggler); if (auto* B=Cast<ADMSquadController>(A->GetController())) { B->SetProfile(M->ProfileFor(*A)); B->ConfigureEncounter(0,E.At,false,0); } }
    }
    for (FVector P : {FVector(-1100,-300,0),FVector(300,650,0),FVector(900,-1750,0)})
    { auto* R=GetWorld()->SpawnActor<ADMVisionArea>(P,FRotator::ZeroRotator); R->Radius=280; }
    for (FVector P : {FVector(-2100,-900,40),FVector(-450,-1100,40),FVector(-900,1200,40)})
    { auto* Food=GetWorld()->SpawnActor<ADMRecoverySupply>(P,FRotator::ZeroRotator); Food->bFood=true; Food->Charges=2; }
    StepScenario();
}
void ADMFishingVillage::Drain() { if (HasAuthority() && !bDrained) { bDrained=true; OnRep_Drained(); ForceNetUpdate(); } }
void ADMFishingVillage::OnRep_Drained() { Flood->SetVisibility(!bDrained); Flood->SetCollisionEnabled(bDrained ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics); }
ADMObjective* ADMFishingVillage::NextObjective() const
{
    if (Core && !Core->IsTerminal()) { return Core; }
    for (ADMObjective* O : Disruptions) { if (O && !O->IsTerminal()) { return O; } } return nullptr;
}
void ADMFishingVillage::StepScenario()
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!M || !M->IsCombatActive() || bManifested) { return; }
    if (Core && Core->PublicState.State==EDMObjectiveState::Completed && CoreStage<4)
    {
        if (Core->bBasinDrained) { Drain(); }
        ++CoreStage; auto D=MakeShared<FJsonObject>(); D->SetNumberField(TEXT("core_stages_completed"),CoreStage); M->Emit(TEXT("village.progress"),D);
        SpawnCore();
    }
    const int32 Done=Disruptions.FilterByPredicate([](const ADMObjective* O){return O && O->PublicState.State==EDMObjectiveState::Completed;}).Num();
    if (auto* G=GetWorld()->GetGameState<ADMGameState>())
    { G->SetEncounterObjective(FString::Printf(TEXT("Fishing Village | Core %d/4 | Disruptions %d/3 | %s"),CoreStage,Done,NextObjective()?*NextObjective()->DisplayTitle:TEXT("Manifestation"))); }
    if (CoreStage==4 && Done==3 && bDrained)
    {
        auto* Arena=GetWorld()->SpawnActor<ADMBossArena>(); Arena->BuildFixture(Basin());
        Encounter=GetWorld()->SpawnActor<ADMShubEncounter>();
        if (Encounter->Begin(Arena)) { bManifested=true; M->Summon(); auto D=MakeShared<FJsonObject>(); D->SetStringField(TEXT("trigger"),TEXT("core_and_disruptions_complete")); M->Emit(TEXT("village.manifested"),D); }
        else { Encounter->Destroy(); Arena->Destroy(); Encounter=nullptr; }
    }
}
bool ADMFishingVillage::DriveBot(ADMCombatant* Hero)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!M || bManifested || Hero->bIsEnemy || Hero->IsDown()) { return false; }
    for (ADMCombatant* A : M->GetCombatants())
    { if (!A->bIsEnemy && A->IsPlayerControlled() && !A->IsDown()) { return false; }
      if (A->bIsEnemy && !A->IsDown() && DMVision::CanSee(Hero,A) && FVector::DistSquared2D(A->GetActorLocation(),Hero->GetActorLocation())<FMath::Square(800.f)) { return false; } }
    auto* O=NextObjective(); if (!O || !O->Current()) { return false; }
    const auto* S=O->Current(); const FVector At=O->PublicState.PayloadLocation;
    if (S->Verb==EDMObjectiveVerb::Destroy) { Hero->SetAttackTarget(O->Destructible); Hero->MoveToward(At); return false; }
    Hero->SetAttackTarget(nullptr); Hero->SetAttackHold(true);
    if (FVector::DistSquared2D(Hero->GetActorLocation(),At)>FMath::Square(100.f)) { Hero->MoveToward(At); return true; }
    Hero->StopGoal();
    if (S->Verb==EDMObjectiveVerb::Sequence) { if (O->bSequenceReady && S->Sequence.IsValidIndex(O->PublicState.Progress)) { O->Interact(Hero,S->Sequence[O->PublicState.Progress]); } }
    else if (!O->IsInteracting(Hero)) { O->Interact(Hero); }
    return true;
}
bool ADMFishingVillage::RouteClear(FVector From,FVector To) const
{
    From.Z=To.Z=80;
    for (int32 I=0;I<Obstacles.Num();++I)
    { if (I==0 && bDrained) { continue; } const FBox B=Obstacles[I].ExpandBy(FVector(45,45,200));
      // A manually steered pawn can enter the clearance margin. Permit an outward
      // route that does not cross the actual solid rather than trapping it there.
      if (B.IsInsideOrOn(From) && !B.IsInsideOrOn(To) && !FMath::LineBoxIntersection(Obstacles[I].ExpandBy(FVector(0,0,200)),From,To,To-From)) { continue; }
      if (FMath::LineBoxIntersection(B,From,To,To-From)) { return false; } }
    return true;
}
FVector ADMFishingVillage::Waypoint(FVector From,FVector Goal) const
{
    if (RouteClear(From,Goal)) { return Goal; }
    TArray<FVector> Nodes={From,Goal};
    for (int32 I=0;I<Obstacles.Num();++I)
    {
        if (I==0 && bDrained) { continue; } const FBox B=Obstacles[I].ExpandBy(65);
        for (auto P : {FVector(B.Min.X,B.Min.Y,From.Z),FVector(B.Min.X,B.Max.Y,From.Z),FVector(B.Max.X,B.Min.Y,From.Z),FVector(B.Max.X,B.Max.Y,From.Z)})
        { if (FMath::Abs(P.X)<2850 && FMath::Abs(P.Y)<2350) { Nodes.Add(P); } }
    }
    TArray<float> Cost; Cost.Init(MAX_flt,Nodes.Num()); Cost[0]=0;
    TArray<int32> Parent; Parent.Init(INDEX_NONE,Nodes.Num()); TArray<bool> Visited; Visited.Init(false,Nodes.Num());
    for (int32 N=0;N<Nodes.Num();++N)
    {
        int32 Best=INDEX_NONE; for (int32 I=0;I<Nodes.Num();++I) { if (!Visited[I] && (Best==INDEX_NONE || Cost[I]<Cost[Best])) { Best=I; } }
        if (Best==INDEX_NONE || Cost[Best]==MAX_flt) { break; } if (Best==1) { int32 P=1; while (Parent[P]>0) { P=Parent[P]; } return Nodes[P]; }
        Visited[Best]=true;
        for (int32 I=0;I<Nodes.Num();++I) { const float C=Cost[Best]+FVector::Dist2D(Nodes[Best],Nodes[I]); if (!Visited[I] && C<Cost[I] && RouteClear(Nodes[Best],Nodes[I])) { Cost[I]=C; Parent[I]=Best; } }
    }
    return From;
}
void ADMFishingVillage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADMFishingVillage,CoreStage); DOREPLIFETIME(ADMFishingVillage,bDrained); DOREPLIFETIME(ADMFishingVillage,bManifested); DOREPLIFETIME(ADMFishingVillage,Core); DOREPLIFETIME(ADMFishingVillage,Disruptions); DOREPLIFETIME(ADMFishingVillage,Optional); }
