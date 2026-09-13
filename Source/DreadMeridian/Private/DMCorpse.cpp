#include "DMCorpse.h"
#include "DMCombatant.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"
ADMCorpse::ADMCorpse() { bReplicates=true; SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("CorpseMarker"))); }
bool ADMCorpse::Initialize(const ADMCombatant* Source)
{
    if (!HasAuthority() || !IsValid(Source) || Source->GetWorld()!=GetWorld() || !Source->IsDown() || !State.SourceId.IsEmpty()) { return false; }
    State.SourceId=Source->EntityId; State.SourceKind=Source->ActorHasTag(TEXT("Broodling")) ? TEXT("broodling") : Source->DisplayName();
    State.Value=Source->ActorHasTag(TEXT("Broodling")) ? .1f : 1.f; State.Location=Source->GetActorLocation(); SetActorLocation(State.Location); ForceNetUpdate(); return true;
}
float ADMCorpse::Consume()
{ if (!HasAuthority() || State.SourceId.IsEmpty() || State.bConsumed) { return 0; } State.bConsumed=true; State.bCleanupEligible=true; ForceNetUpdate(); return State.Value; }
bool ADMCorpse::Restore(const FDMCorpseSnapshot& S)
{ if (!HasAuthority() || S.Version!=1 || S.SourceId.IsEmpty() || S.Location.ContainsNaN() || !FMath::IsFinite(S.Value) || S.Value<0 || S.Value>1 || (S.bConsumed && !S.bCleanupEligible)) { return false; } State=S; SetActorLocation(S.Location); ForceNetUpdate(); return true; }
void ADMCorpse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADMCorpse,State); }
