#pragma once
#include "CoreMinimal.h"
#include "DMRelicRuntime.generated.h"
USTRUCT()
struct FDMRelicWake
{
    GENERATED_BODY()
    UPROPERTY() FVector Location=FVector::ZeroVector;
    UPROPERTY() int32 Until=0;
};
/** Authority runtime snapshot. No private condition inputs are replicated through inventory. */
USTRUCT()
struct FDMRelicRuntime
{
    GENERATED_BODY()
    UPROPERTY() int32 Version=1;
    UPROPERTY() TArray<FString> Swaggered;
    UPROPERTY() TMap<FString,float> Contributions;
    UPROPERTY() FString PrimedTarget;
    UPROPERTY() int32 PrimeUntil=0;
    UPROPERTY() float OwnedShield=0;
    UPROPERTY() int32 ShieldHoldUntil=0;
    UPROPERTY() int32 RosaryUntil=0;
    UPROPERTY() int32 HasteUntil=0;
    UPROPERTY() int32 ControlUntil=0;
    UPROPERTY() FVector LastPosition=FVector::ZeroVector;
    UPROPERTY() bool bHasPosition=false;
    UPROPERTY() float Distance=0;
    UPROPERTY() int32 MovementWindow=0;
    UPROPERTY() TArray<FDMRelicWake> Wakes;
};
