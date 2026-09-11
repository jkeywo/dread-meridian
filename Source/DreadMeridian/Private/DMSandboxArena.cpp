#include "DMSandboxArena.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/TextRenderComponent.h"
#include "DMEncounterLayout.h"

ADMSandboxArena::ADMSandboxArena()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    auto MakeBlock = [this](const TCHAR* Name, FVector Position, FVector Scale)
    {
        UStaticMeshComponent* Block = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Block->SetupAttachment(RootComponent);
        Block->SetStaticMesh(Cube.Object);
        Block->SetRelativeLocation(Position);
        Block->SetRelativeScale3D(Scale);
    };
    MakeBlock(TEXT("Floor"), FVector(0, 0, -25), FVector(60, 50, .5));
    MakeBlock(TEXT("NorthWall"), FVector(3000, 0, 100), FVector(.3, 50, 2));
    MakeBlock(TEXT("SouthWall"), FVector(-3000, 0, 100), FVector(.3, 50, 2));
    MakeBlock(TEXT("EastWall"), FVector(0, 2500, 100), FVector(60, .3, 2));
    MakeBlock(TEXT("WestWall"), FVector(0, -2500, 100), FVector(60, .3, 2));
    for (int32 Camp = 0; Camp < DMEncounterLayout::CampCount; ++Camp)
    {
        const FVector Center = DMEncounterLayout::Camp(Camp);
        MakeBlock(*FString::Printf(TEXT("Camp%dCrate"), Camp), Center + FVector(300, 260, -45), FVector(1.4, 1.4, 1));
        MakeBlock(*FString::Printf(TEXT("Camp%dSupplies"), Camp), Center + FVector(300, -260, -60), FVector(1, 1.6, .7));
        auto* Sign = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Camp%dSign"), Camp));
        Sign->SetupAttachment(RootComponent); Sign->SetRelativeLocation(Center + FVector(0, 0, -85));
        Sign->SetRelativeRotation(FRotator(90, 0, 0)); Sign->SetHorizontalAlignment(EHTA_Center);
        Sign->SetWorldSize(65); Sign->SetTextRenderColor(FColor(230, 170, 100));
        Sign->SetText(FText::FromString(FString::Printf(TEXT("CAMP %d"), Camp + 1)));
    }
    for (int32 Point = 0; Point < 4; ++Point)
    {
        const FVector A = DMEncounterLayout::PatrolPoint(Point);
        const FVector B = DMEncounterLayout::PatrolPoint(Point + 1);
        for (int32 Dash = 0; Dash < 10; ++Dash)
        {
            // Flat, non-obstructing route dashes give the patrol a readable circuit.
            MakeBlock(*FString::Printf(TEXT("Route%dDash%d"), Point, Dash), FMath::Lerp(A, B, Dash / 10.f) + FVector(0, 0, -94),
                Point % 2 == 0 ? FVector(.65, .12, .01) : FVector(.12, .65, .01));
        }
    }
    UDirectionalLightComponent* Light = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("KeyLight"));
    Light->SetupAttachment(RootComponent);
    Light->SetRelativeRotation(FRotator(-60, -30, 0));
    Light->SetIntensity(5);
    auto* Fill = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("CharacterFill"));
    Fill->SetupAttachment(RootComponent); Fill->SetRelativeRotation(FRotator(-35, 150, 0));
    Fill->SetIntensity(2); Fill->SetCastShadows(false);

}
