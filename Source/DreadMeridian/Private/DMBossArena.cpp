#include "DMBossArena.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"
ADMBossArena::ADMBossArena()
{ bReplicates = true; bAlwaysRelevant = true; SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Arena"))); }
TArray<FVector> ADMBossArena::Locations(FName Kind) const
{ TArray<FVector> Out; for (const auto& S : Sites) { if (S.Kind==Kind) { Out.Add(S.Location); } } return Out; }
bool ADMBossArena::Contains(FName Kind,FVector Point) const
{ if (Point.ContainsNaN()) { return false; } for (const auto& S : Sites) { if (S.Kind==Kind && FVector::DistSquared2D(Point,S.Location)<=FMath::Square(S.Radius)) { return true; } } return false; }
bool ADMBossArena::Validate(EDMElderOne Boss,FString& Error) const
{
    Error.Reset(); if (static_cast<uint8>(Boss)>1) { Error=TEXT("Unsupported Elder One"); return false; }
    for (const auto& S : Sites)
    { if (S.Kind.IsNone() || S.Location.ContainsNaN() || !FMath::IsFinite(S.Radius) || S.Radius<=0) { Error=TEXT("Invalid arena affordance"); return false; } }
    TArray<FName> Required = {TEXT("BossSite"),TEXT("ManifestationPoint"),TEXT("SpawnRoute"),TEXT("ObjectiveAnchor"),TEXT("HazardRegion"),TEXT("MovementLane"),TEXT("ArenaEntrance")};
    if (Boss==EDMElderOne::Shub) { Required.Append({TEXT("GrowthSite"),TEXT("CorruptionValid")}); }
    for (const FName& Kind : Required) { if (Locations(Kind).IsEmpty()) { Error=TEXT("Missing boss-map affordance: ")+Kind.ToString(); return false; } }
    if (Locations(TEXT("ArenaEntrance")).Num()<2 || Locations(TEXT("ManifestationPoint")).Num()<3)
    { Error=TEXT("Arena needs multiple entrances and three manifestation points"); return false; }
    if (Boss==EDMElderOne::Shub)
    { for (const FVector& P : Locations(TEXT("GrowthSite"))) { if (!Contains(TEXT("CorruptionValid"),P)) { Error=TEXT("Growth lies outside corruption-valid ground"); return false; } } }
    return true;
}
void ADMBossArena::BuildFixture(FVector Origin)
{
    if (!HasAuthority() || Origin.ContainsNaN()) { return; } Sites.Reset(); SetActorLocation(Origin);
    auto Add = [&](FName Kind,FVector Offset,float Radius=150.f) { FDMBossAffordance S; S.Kind=Kind; S.Location=Origin+Offset; S.Radius=Radius; Sites.Add(S); };
    Add(TEXT("BossSite"),FVector::ZeroVector); Add(TEXT("HazardRegion"),FVector::ZeroVector,1300);
    Add(TEXT("CorruptionValid"),FVector::ZeroVector,1300); Add(TEXT("MovementLane"),FVector::ZeroVector,1100);
    for (FVector P : {FVector(-600,-400,0),FVector(600,-400,0),FVector(0,600,0)})
    { Add(TEXT("ManifestationPoint"),P); Add(TEXT("GrowthSite"),P); Add(TEXT("ObjectiveAnchor"),P); Add(TEXT("SpawnRoute"),P); }
    Add(TEXT("ArenaEntrance"),FVector(-1100,0,0)); Add(TEXT("ArenaEntrance"),FVector(1100,0,0)); ForceNetUpdate();
}
void ADMBossArena::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADMBossArena,Sites); }
