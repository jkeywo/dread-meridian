#include "DMScroungePickup.h"
#include "DMCombatant.h"
#include "DMInvestigatorComponent.h"
#include "DMCombatGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
ADMScroungePickup::ADMScroungePickup()
{
    bReplicates = true; PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Component")); SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Asset(TEXT("/Engine/BasicShapes/Cube"));
    Mesh->SetStaticMesh(Asset.Object); Mesh->SetRelativeScale3D(FVector(.2f)); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label")); Label->SetupAttachment(Mesh);
    Label->SetAbsolute(false, true, true); Label->SetWorldSize(16); Label->SetText(FText::FromString(TEXT("Components")));
    Label->SetTextRenderColor(FColor::Orange); Label->SetRelativeLocation(FVector(0, 0, 130));
}
void ADMScroungePickup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority()) { return; }
    auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    // Collection stops with combat: the final kill drops a pickup, and collecting it after CompleteCombat has published
    // the end state would change the Sapper's replicated resources behind the network probe's back.
    if (!Mode || !Mode->IsCombatActive()) { return; }
    for (ADMCombatant* Actor : Mode->GetCombatants())
    {
        if (!Actor->IsDown() && FVector::DistSquared2D(GetActorLocation(), Actor->GetActorLocation()) <= FMath::Square(160.f)
            && Actor->Investigator->CollectComponent())
        { Actor->RecordResources(TEXT("scrounger")); Destroy(); return; }
    }
}
