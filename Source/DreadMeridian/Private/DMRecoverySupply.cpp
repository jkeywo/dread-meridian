#include "DMRecoverySupply.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
ADMRecoverySupply::ADMRecoverySupply()
{
    bReplicates = true; PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Supply")); SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Asset(TEXT("/Engine/BasicShapes/Cube"));
    Mesh->SetStaticMesh(Asset.Object); Mesh->SetRelativeScale3D(FVector(.5f)); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label")); Label->SetupAttachment(Mesh);
    Label->SetAbsolute(false, true, true); Label->SetRelativeLocation(FVector(0,0,150)); Label->SetWorldSize(18);
    Label->SetTextRenderColor(FColor::Green);
}
bool ADMRecoverySupply::TryUse(ADMCombatant* Actor)
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !M->IsCombatActive() || !IsValid(Actor) || Actor->GetWorld() != GetWorld()
        || Actor->bIsEnemy || Actor->IsDown() || Actor->IsStunned() || Actor->IsRestrained() || Charges <= 0
        || FVector::DistSquared2D(GetActorLocation(), Actor->GetActorLocation()) > FMath::Square(160.f)) { return false; }
    if (!Actor->Smuggler->Sight(GetActorLocation() + FVector(0,0,40), Actor->GetActorLocation())) { return false; }
    if (bFood)
    {
        if (Actor->Health() >= Actor->MaxHealth() || Actor->Injuries->HealingPulses > 0) { return false; }
        Actor->Injuries->StartFoodHealing();
    }
    else if (!Actor->Injuries->Treat()) { return false; }
    --Charges; ForceNetUpdate();
    auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"), Actor->EntityId);
    D->SetStringField(TEXT("kind"), bFood ? TEXT("food") : TEXT("treatment")); D->SetNumberField(TEXT("remaining"), Charges);
    M->Emit(TEXT("recovery.used"), D); return true;
}
void ADMRecoverySupply::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Label->SetText(FText::FromString(FString::Printf(TEXT("%s | %d left"), bFood ? TEXT("Food") : TEXT("Treatment | T / D-pad left"), Charges)));
}
void ADMRecoverySupply::Step()
{
    // Food auto-collects. Humans explicitly choose scarce treatment; nearby bots use the same acceptance API.
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !M->IsCombatActive() || Charges <= 0) { return; }
    for (ADMCombatant* A : M->GetCombatants()) { if ((bFood || !A->IsPlayerControlled()) && TryUse(A)) { break; } }
}
void ADMRecoverySupply::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADMRecoverySupply, bFood); DOREPLIFETIME(ADMRecoverySupply, Charges); }
