#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "DMArtRigLibrary.generated.h"

class USkeletalMesh;
class USkeleton;
class UAnimSequence;
class UStaticMesh;

/** Editor-only authoring helpers for the investigator skin and attachment assets. */
UCLASS()
class UDMArtRigLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Dread Meridian|Art")
    static bool CropAnimation(UAnimSequence* Animation, float StartSeconds, float EndSeconds);
    UFUNCTION(BlueprintCallable, Category="Dread Meridian|Art")
    static bool CopyAnimationWindow(UAnimSequence* Source, UAnimSequence* Destination, float StartSeconds, float EndSeconds);
    UFUNCTION(BlueprintCallable, Category="Dread Meridian|Art")
    static bool SetMeshSocket(USkeletalMesh* Mesh, FName SocketName, FName BoneName, FTransform LocalTransform);

    UFUNCTION(BlueprintCallable, Category="Dread Meridian|Art")
    static bool SetStaticMeshSocket(UStaticMesh* Mesh, FName SocketName, FVector Location);

    UFUNCTION(BlueprintCallable, Category="Dread Meridian|Art")
    static bool BakeConstantBonePose(UAnimSequence* Animation, const TArray<FName>& Bones, const TArray<FTransform>& Transforms);

    UFUNCTION(BlueprintPure, Category="Dread Meridian|Art")
    static TArray<FTransform> GetSkeletonReferenceTransforms(USkeleton* Skeleton);

    UFUNCTION(BlueprintPure, Category="Dread Meridian|Art")
    static TArray<FName> GetSkeletonBoneNames(USkeleton* Skeleton);
};
