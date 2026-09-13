#include "DMAbilityMarker.h"
#include "DMCombatant.h"
#include "DMPrimaryComponent.h"
#include "DMCombatGameMode.h"
#include "DMGameState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    // 0..23 outline the shape, 24..47 are embers/detail. Both pools are reused by every shape.
    constexpr int32 OutlineCount = 24;
    constexpr int32 InstanceCount = 48;
    // How far an unbound spirit wanders from where it was cast.
    constexpr float SpiritWanderRadius = UDMPrimaryComponent::SpiritRadius * .9f;
}

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
    GhostBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GhostBody")); GhostBody->SetupAttachment(GetRootComponent());
    static ConstructorHelpers::FObjectFinder<UStaticMesh> GhostMesh(TEXT("/Game/DreadMeridian/Presentation/Props/SM_GhostlyFigure"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GhostMaterial(TEXT("/Game/DreadMeridian/Presentation/Props/M_GhostlyFigure"));
    GhostBody->SetStaticMesh(GhostMesh.Object); GhostBody->SetMaterial(0, GhostMaterial.Object);
    GhostBody->SetRelativeScale3D(FVector(.75f)); GhostBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GhostBody->SetCastShadow(false); GhostBody->SetVisibility(false);
}
void ADMAbilityMarker::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) { return; }
    Glow = Orb->CreateDynamicMaterialInstance(0); Ring->SetMaterial(0, Glow);
    for (int32 I = 0; I < InstanceCount; ++I) { Ring->AddInstance(FTransform::Identity); }
}
int32 ADMAbilityMarker::CurrentTick() const
{
    if (const auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ADMCombatGameMode>() : nullptr) { return Mode->GetCombatTick(); }
    const auto* State = GetWorld() ? GetWorld()->GetGameState<ADMGameState>() : nullptr;
    return State ? State->GetCombatTick() : 0;
}
void ADMAbilityMarker::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority() && bTravelling && bSpirit)
    {
        // Glide every frame instead of stepping once per combat tick (10Hz) - the discrete jumps otherwise
        // read as the spirit flickering/juddering rather than moving.
        const FVector At = GetActorLocation();
        const FVector Delta = TravelGoal - At;
        const float Step = TravelRate * DeltaSeconds;
        SetActorLocation(Delta.SizeSquared2D() <= FMath::Square(Step) ? TravelGoal : At + Delta.GetSafeNormal2D() * Step);
    }
    if (HasAuthority() && !bTravelling && (bSpirit || bHostile))
    {
        if (IsValid(BoundTarget))
        {
            FVector Anchor = BoundTarget->GetActorLocation() - FVector(0, 0, 70);
            if (bSpirit)
            {
                // Deterministic per-spirit drift so a bound spirit doesn't sit rigidly pinned to the corpse.
                const float Seed = (GetTypeHash(SpiritId) % 1000) * .01f;
                const float T = GetWorld()->GetTimeSeconds() + Seed;
                Anchor += FVector(FMath::Sin(T * .6f) * 22.f, FMath::Cos(T * .45f) * 22.f, 0);
            }
            SetActorLocation(Anchor);
        }
        else if (bSpirit)
        {
            // No corpse to orbit: wander around where the spirit was cast instead of sitting rigid.
            if (!bWanderOriginSet) { WanderOrigin = GetActorLocation(); bWanderOriginSet = true; }
            const float Seed = (GetTypeHash(SpiritId) % 1000) * .01f;
            const float T = GetWorld()->GetTimeSeconds() + Seed;
            FVector Offset(FMath::Sin(T * .17f) * SpiritWanderRadius * .5f + FMath::Sin(T * .053f + 1.7f) * SpiritWanderRadius * .5f,
                FMath::Cos(T * .13f) * SpiritWanderRadius * .5f + FMath::Cos(T * .071f + .9f) * SpiritWanderRadius * .5f, 0);
            SetActorLocation(WanderOrigin + Offset.GetClampedToMaxSize(SpiritWanderRadius));
        }
    }
    if (GetNetMode() == NM_DedicatedServer) { return; }
    const FLinearColor Color = bHostile ? FLinearColor(1,.08f,.03f) : bSpirit ? FLinearColor(.7f, .25f, 1)
        : Shape == EDMMarkerShape::Cone ? FLinearColor(1, .7f, .2f) : FLinearColor(1, .5f, .05f);
    const bool bArmed = IsArmed();
    if (Glow) { Glow->SetVectorParameterValue(TEXT("Tint"), Color * (bArmed ? 3 : 1.2f)); }
    Orb->SetVisibility(!bSpirit && Shape != EDMMarkerShape::Cone);
    GhostBody->SetVisibility(bSpirit);
    if (bSpirit)
    {
        // No animation on the ghostly figure itself, just a slow bob.
        GhostBody->SetRelativeLocation(FVector(0, 0, 60 + FMath::Sin(GetWorld()->GetTimeSeconds() * 1.1f) * 12.f));
    }
    FString Text = CustomLabel;
    if (Text.IsEmpty())
    {
        Text = bSpirit ? FString::Printf(TEXT("Spirit %.0f"), Attention)
            : Shape == EDMMarkerShape::Cone ? TEXT("SUPPRESSING")
            : Shape == EDMMarkerShape::Wire ? (bArmed ? TEXT("TRIPWIRE") : TEXT("ARMING"))
            : TEXT("Satchel");
    }
    Label->SetText(FText::FromString(Text));
    Label->SetTextRenderColor(Color.ToFColor(true));
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
    { if (PC->PlayerCameraManager) { Label->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation() - Label->GetComponentLocation()).Rotation()); } }
    switch (Shape)
    {
    case EDMMarkerShape::Cone: UpdateCone(); break;
    case EDMMarkerShape::Wire: UpdateWire(); break;
    default: UpdateCircle(); break;
    }
}
void ADMAbilityMarker::UpdateCircle()
{
    for (int32 I = 0; I < Ring->GetInstanceCount(); ++I)
    {
        if (I >= OutlineCount)
        {
            const bool bBurning = bHostile && CustomLabel == TEXT("BURNING GROUND");
            const float Phase = FMath::Frac(GetWorld()->GetTimeSeconds()*1.5f + I*.618f);
            const float Angle = I*2.39996f;
            const FVector Ember(FMath::Cos(Angle)*Radius*.8f*FMath::Sqrt((I-OutlineCount+1)/24.f), FMath::Sin(Angle)*Radius*.8f*FMath::Sqrt((I-OutlineCount+1)/24.f), Phase*65);
            Ring->UpdateInstanceTransform(I,FTransform(FQuat::Identity,Ember,bBurning ? FVector(.035f,.035f,.12f)*(1-Phase) : FVector::ZeroVector),false,I==InstanceCount-1);
            continue;
        }
        const float A = I * UE_TWO_PI / OutlineCount;
        Ring->UpdateInstanceTransform(I, FTransform(FQuat::Identity, FVector(FMath::Cos(A) * Radius, FMath::Sin(A) * Radius, -10), FVector(.06f)), false, I == InstanceCount-1);
    }
}
void ADMAbilityMarker::UpdateCone()
{
    // 0..15 sweep the far arc, 16..47 run the two straight edges out to Length.
    const FVector Dir = Direction.GetSafeNormal2D();
    const float Base = FMath::Atan2(Dir.Y, Dir.X);
    const float Half = FMath::DegreesToRadians(HalfAngle);
    constexpr int32 ArcCount = 16;
    const float Fade = ExpiresTick > 0 ? FMath::Clamp((ExpiresTick - CurrentTick()) / 10.f, .25f, 1.f) : 1.f;
    for (int32 I = 0; I < Ring->GetInstanceCount(); ++I)
    {
        FVector At = FVector::ZeroVector;
        if (I < ArcCount)
        {
            const float A = Base - Half + 2 * Half * I / (ArcCount - 1);
            At = FVector(FMath::Cos(A) * Length, FMath::Sin(A) * Length, -10);
        }
        else
        {
            const int32 Step = (I - ArcCount) / 2;
            const float A = Base + ((I - ArcCount) % 2 ? Half : -Half);
            const float D = Length * (Step + 1) / 16.f;
            At = FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, -10);
        }
        Ring->UpdateInstanceTransform(I, FTransform(FQuat::Identity, At, FVector(.06f * Fade)), false, I == InstanceCount-1);
    }
}
void ADMAbilityMarker::UpdateWire()
{
    const FVector Span = WireEnd - GetActorLocation();
    const bool bArmed = IsArmed();
    const float Pulse = bArmed ? .05f + .02f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6) : .03f;
    for (int32 I = 0; I < Ring->GetInstanceCount(); ++I)
    {
        const FVector At = Span * (static_cast<float>(I) / (InstanceCount - 1)) + FVector(0, 0, -10);
        Ring->UpdateInstanceTransform(I, FTransform(FQuat::Identity, At, FVector(Pulse)), false, I == InstanceCount-1);
    }
}
void ADMAbilityMarker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADMAbilityMarker, bHostile); DOREPLIFETIME(ADMAbilityMarker, CustomLabel);
    DOREPLIFETIME(ADMAbilityMarker, bSpirit); DOREPLIFETIME(ADMAbilityMarker, BoundTarget);
    DOREPLIFETIME(ADMAbilityMarker, SpiritId); DOREPLIFETIME(ADMAbilityMarker, Radius); DOREPLIFETIME(ADMAbilityMarker, Attention);
    DOREPLIFETIME(ADMAbilityMarker, Shape); DOREPLIFETIME(ADMAbilityMarker, Direction); DOREPLIFETIME(ADMAbilityMarker, HalfAngle);
    DOREPLIFETIME(ADMAbilityMarker, Length); DOREPLIFETIME(ADMAbilityMarker, WireEnd);
    DOREPLIFETIME(ADMAbilityMarker, bTravelling); DOREPLIFETIME(ADMAbilityMarker, TravelRate); DOREPLIFETIME(ADMAbilityMarker, TravelGoal);
    DOREPLIFETIME(ADMAbilityMarker, ArmedTick); DOREPLIFETIME(ADMAbilityMarker, ExpiresTick); DOREPLIFETIME(ADMAbilityMarker, Serial);
}
