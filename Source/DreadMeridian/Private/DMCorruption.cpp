#include "DMCorruption.h"
#include "DMBossArena.h"
#include "DMCombatGameMode.h"
#include "DMCombatant.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
ADMCorruption::ADMCorruption()
{
    bReplicates=true; bAlwaysRelevant=true;
    Ground=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CorruptedGround")); SetRootComponent(Ground);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder")); Ground->SetStaticMesh(Shape.Object); Ground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
bool ADMCorruption::Initialize(ADMBossArena* A)
{ if (!HasAuthority() || Arena || !IsValid(A) || A->GetWorld()!=GetWorld()) { return false; } FString Error; if (!A->Validate(EDMElderOne::Shub,Error)) { return false; } Arena=A; return true; }
bool ADMCorruption::Spread(FVector Location)
{
    if (!HasAuthority() || !Arena || Location.ContainsNaN()) { return false; }
    const FVector Cell(FMath::GridSnap(Location.X,150.),FMath::GridSnap(Location.Y,150.),Arena->GetActorLocation().Z+3);
    if (!Arena->Contains(TEXT("CorruptionValid"),Cell) || State.Cells.Contains(Cell) || State.Cells.Num()>=1024) { return false; }
    State.Cells.Add(Cell); Rebuild(); ForceNetUpdate();
    if (auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>()) { auto D=MakeShared<FJsonObject>(); D->SetNumberField(TEXT("cells"),State.Cells.Num()); M->Emit(TEXT("boss.corruption_spread"),D); }
    return true;
}
bool ADMCorruption::Contains(FVector Location) const
{ if (Location.ContainsNaN()) { return false; } for (const auto& C : State.Cells) { if (FVector::DistSquared2D(C,Location)<=FMath::Square(110.f)) { return true; } } return false; }
void ADMCorruption::Step(int32 Tick)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!HasAuthority() || !M || !M->IsCombatActive() || Tick<=LastTick) { return; } LastTick=Tick;
    for (ADMCombatant* A : M->GetCombatants())
    { if (IsValid(A) && !A->IsDown() && Contains(A->GetActorLocation()))
      { if (!A->bIsEnemy) { A->MadnessCore->Add(.08f,TEXT("shub.corruption")); }
        else if (A->ActorHasTag(TEXT("ShubLinked"))) { A->SpiritProtection=FMath::Max(A->SpiritProtection,.3f); } } }
}
bool ADMCorruption::Restore(const FDMCorruptionSnapshot& S)
{
    if (!HasAuthority() || !Arena || S.Version!=1 || S.Cells.Num()>1024) { return false; }
    TSet<FVector> Unique;
    for (const auto& C : S.Cells) { if (C.ContainsNaN() || !Arena->Contains(TEXT("CorruptionValid"),C) || Unique.Contains(C)) { return false; } Unique.Add(C); }
    State=S; LastTick=-1; Rebuild(); ForceNetUpdate(); return true;
}
void ADMCorruption::Rebuild()
{ Ground->ClearInstances(); for (const auto& C : State.Cells) { Ground->AddInstance(FTransform(FRotator::ZeroRotator,C,FVector(2.2f,2.2f,.02f)),true); } }
void ADMCorruption::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADMCorruption,State); }
