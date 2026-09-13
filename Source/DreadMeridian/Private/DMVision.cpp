#include "DMVision.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
ADMVisionArea::ADMVisionArea()
{ bReplicates=true; bAlwaysRelevant=true; SetReplicateMovement(true); SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Area"))); }
bool ADMVisionArea::Contains(FVector P) const
{ return bEnabled && FMath::IsFinite(Radius) && Radius>0 && !P.ContainsNaN() && FVector::DistSquared2D(P,GetActorLocation())<=FMath::Square(Radius); }
void ADMVisionArea::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADMVisionArea,bReeds); DOREPLIFETIME(ADMVisionArea,bEnabled); DOREPLIFETIME(ADMVisionArea,Radius); DOREPLIFETIME(ADMVisionArea,SourceId); }
bool DMVision::InReeds(UWorld* W,FVector P)
{ if (!W) { return false; } for (TActorIterator<ADMVisionArea> It(W);It;++It) { if (It->bReeds && It->Contains(P)) { return true; } } return false; }
bool DMVision::CanSee(const ADMCombatant* Observer,const ADMCombatant* Target)
{
    if (!IsValid(Observer) || !IsValid(Target) || Observer->GetWorld()!=Target->GetWorld()) { return false; }
    if (!Target->bRequiresVision || Observer==Target || !Observer->IsHostileTo(Target)) { return true; }
    auto* W=Target->GetWorld();
    // Clients only select/render the server's owner-delivered projection.
    if (!Target->HasAuthority()) { return !Target->IsHidden(); }
    const auto S=Target->Swamp->Capture(); const auto* M=W->GetAuthGameMode<ADMCombatGameMode>();
    const bool Concealed=InReeds(W,Target->GetActorLocation()) && S.Role==EDMSwampThing::Lurker && S.ResolveAt==0 && M && M->GetCombatTick()>=S.ReadyAt && M->GetCombatTick()>=Target->VisionRevealUntil;
    if (!Observer->bIsEnemy)
    {
        // Lighthouse sources reveal reeds as well as open ground. Geometry still limits investigator sight.
        for (TActorIterator<ADMVisionArea> It(W);It;++It) { if (!It->bReeds && It->Contains(Target->GetActorLocation())) { return true; } }
    }
    for (TActorIterator<ADMCombatant> It(W);It;++It)
    {
        const auto* Eye=*It;
        if (Eye->IsDown() || (Observer->bIsEnemy ? Eye!=Observer : Eye->bIsEnemy)) { continue; }
        const float Reach=Concealed ? 140.f : 1500.f;
        if (FVector::DistSquared2D(Eye->GetActorLocation(),Target->GetActorLocation())<=FMath::Square(Reach)
            && Eye->Smuggler->Sight(Eye->GetActorLocation(),Target->GetActorLocation())) { return true; }
    }
    return false;
}
bool DMVision::Relevant(const ADMCombatant* Target,const AActor* Viewer)
{
    const auto* PC=Cast<APlayerController>(Viewer);
    const auto* Observer=PC ? Cast<ADMCombatant>(PC->GetPawn()) : Cast<ADMCombatant>(Viewer);
    return Observer && CanSee(Observer,Target);
}
void DMVision::Lighthouse(UWorld* W,const FString& Id,FVector P,bool bEnabled)
{
    if (!W || !W->GetAuthGameMode() || Id.IsEmpty() || P.ContainsNaN()) { return; }
    for (TActorIterator<ADMVisionArea> It(W);It;++It) { if (It->SourceId==Id) { It->bEnabled=bEnabled; It->ForceNetUpdate(); return; } }
    if (!bEnabled) { return; }
    auto* Source=W->SpawnActor<ADMVisionArea>(P,FRotator::ZeroRotator); Source->bReeds=false; Source->Radius=5000; Source->SourceId=Id;
}
