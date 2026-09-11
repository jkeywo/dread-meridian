#include "DMAttackFX.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
ADMAttackFX::ADMAttackFX()
{
    PrimaryActorTick.bCanEverTick = true;
    Particles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Particles")); SetRootComponent(Particles);
    Particles->SetCollisionEnabled(ECollisionEnabled::NoCollision); Particles->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Game/DreadMeridian/Presentation/M_AttackGlow"));
    Particles->SetStaticMesh(MeshAsset.Object); Particles->SetMaterial(0, Glow.Object);
}
void ADMAttackFX::Initialize(FVector From, FVector To, FLinearColor Color, uint8 Style)
{
    Start = From; End = To; EffectStyle = Style;
    Material = Particles->CreateDynamicMaterialInstance(0);
    if (Material) { Material->SetVectorParameterValue(TEXT("Tint"), Color); }
    for (int32 I = 0; I < 12; ++I) { Particles->AddInstance(FTransform(FQuat::Identity, From, FVector(.04f)), true); }
    SetLifeSpan(Style == 6 ? 1.2f : Style == 7 || Style == 8 ? .8f : .4f);
}
void ADMAttackFX::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); Age += DeltaSeconds;
    const float T = FMath::Clamp(Age / (EffectStyle == 6 ? 1.2f : EffectStyle == 7 || EffectStyle == 8 ? .8f : .4f), 0.f, 1.f);
    for (int32 I = 0; I < 12; ++I)
    {
        const float Angle = I * UE_TWO_PI / 12;
        const FVector Ray(FMath::Cos(Angle), FMath::Sin(Angle), FMath::Sin(Angle * 3) * .5f);
        FVector Position;
        if (EffectStyle == 6)
        {
            const float Travel=FMath::Min(1.f,T*1.4f);
            Position=FMath::Lerp(Start,End,Travel)+FVector(0,0,160*4*Travel*(1-Travel))+Ray*4;
        }
        else if (EffectStyle == 7) { Position=End+FVector(FMath::Cos(Angle)*(55-20*T),FMath::Sin(Angle)*(55-20*T),65); }
        else if (EffectStyle == 8) { Position=End+FVector(FMath::Cos(Angle)*75*(1-T),FMath::Sin(Angle)*75*(1-T),90+I%3*14); }
        else if (EffectStyle == 9) { Position=End+Ray*(15+T*100)-FVector(0,0,65); }
        else if (EffectStyle == 5) { Position = End + Ray * (8 + T * 45) - FVector(0, 0, T * T * 65); }
        else if (EffectStyle == 3) { Position = FMath::Lerp(Start, End, FMath::Min(1.f, T * 2)) + Ray * (8 + I); }
        else if (EffectStyle == 4) { Position = End + Ray * (12 + T * 65); }
        else { Position = T < .35f ? FMath::Lerp(Start, End, I / 11.f) : End + Ray * T * 45; }
        const float Size = (EffectStyle == 3 ? .075f : .045f) * (1 - T);
        Particles->UpdateInstanceTransform(I, FTransform(FQuat::Identity, Position, FVector(Size)), true, I == 11);
    }
}
