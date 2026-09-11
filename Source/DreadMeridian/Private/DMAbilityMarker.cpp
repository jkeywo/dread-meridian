#include "DMAbilityMarker.h"
#include "DMCombatant.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
ADMAbilityMarker::ADMAbilityMarker()
{
    bReplicates = true; SetReplicateMovement(true); PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Orb = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Orb")); Orb->SetupAttachment(GetRootComponent());
    Ring = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Radius")); Ring->SetupAttachment(GetRootComponent());
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/DreadMeridian/Presentation/M_AttackGlow"));
    Orb->SetStaticMesh(Sphere.Object); Orb->SetMaterial(0, Material.Object); Orb->SetRelativeScale3D(FVector(.2f));
    Ring->SetStaticMesh(Sphere.Object); Ring->SetMaterial(0, Material.Object);
    Orb->SetCollisionEnabled(ECollisionEnabled::NoCollision); Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Orb->SetCastShadow(false); Ring->SetCastShadow(false);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label")); Label->SetupAttachment(GetRootComponent());
    Label->SetRelativeLocation(FVector(0, 0, 45)); Label->SetHorizontalAlignment(EHTA_Center); Label->SetWorldSize(18);
}
void ADMAbilityMarker::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) { return; }
    Glow = Orb->CreateDynamicMaterialInstance(0); Ring->SetMaterial(0, Glow);
    for (int32 I = 0; I < 48; ++I) { Ring->AddInstance(FTransform::Identity); }
}
void ADMAbilityMarker::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority() && (bSpirit || bHostile) && IsValid(BoundTarget))
    { SetActorLocation(BoundTarget->GetActorLocation() - FVector(0, 0, 70)); }
    if (GetNetMode() == NM_DedicatedServer) { return; }
    const FLinearColor Color = bHostile ? FLinearColor(1,.08f,.03f) : bSpirit ? FLinearColor(.7f, .25f, 1) : FLinearColor(1, .5f, .05f);
    if (Glow) { Glow->SetVectorParameterValue(TEXT("Tint"), Color * 3); }
    Orb->SetRelativeLocation(FVector(0, 0, bSpirit ? 60 + FMath::Sin(GetWorld()->GetTimeSeconds() * 3) * 10 : 0));
    Label->SetText(FText::FromString(!CustomLabel.IsEmpty() ? CustomLabel : bSpirit ? FString::Printf(TEXT("Spirit %.0f"), Attention) : TEXT("Satchel")));
    Label->SetTextRenderColor(Color.ToFColor(true));
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
    { if (PC->PlayerCameraManager) { Label->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation() - Label->GetComponentLocation()).Rotation()); } }
    for (int32 I = 0; I < Ring->GetInstanceCount(); ++I)
    {
        if (I >= 24)
        {
            const bool bBurning = bHostile && CustomLabel == TEXT("BURNING GROUND");
            const float Phase = FMath::Frac(GetWorld()->GetTimeSeconds()*1.5f + I*.618f);
            const float Angle = I*2.39996f;
            const FVector Ember(FMath::Cos(Angle)*Radius*.8f*FMath::Sqrt((I-23)/24.f), FMath::Sin(Angle)*Radius*.8f*FMath::Sqrt((I-23)/24.f), Phase*65);
            Ring->UpdateInstanceTransform(I,FTransform(FQuat::Identity,Ember,bBurning ? FVector(.035f,.035f,.12f)*(1-Phase) : FVector::ZeroVector),false,I==47);
            continue;
        }
        const float A = I * UE_TWO_PI / 24;
        Ring->UpdateInstanceTransform(I, FTransform(FQuat::Identity, FVector(FMath::Cos(A) * Radius, FMath::Sin(A) * Radius, -10), FVector(.06f)), false, I == 23);
    }
}
void ADMAbilityMarker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADMAbilityMarker, bHostile); DOREPLIFETIME(ADMAbilityMarker, CustomLabel);
    DOREPLIFETIME(ADMAbilityMarker, bSpirit); DOREPLIFETIME(ADMAbilityMarker, BoundTarget);
    DOREPLIFETIME(ADMAbilityMarker, SpiritId); DOREPLIFETIME(ADMAbilityMarker, Radius); DOREPLIFETIME(ADMAbilityMarker, Attention);
}
