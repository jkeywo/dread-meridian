#include "DMArtRigLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

bool UDMArtRigLibrary::SetMeshSocket(USkeletalMesh* Mesh, FName SocketName, FName BoneName, FTransform LocalTransform)
{
    if (!Mesh || SocketName.IsNone() || Mesh->GetRefSkeleton().FindBoneIndex(BoneName) == INDEX_NONE)
        return false;
    Mesh->Modify();
    auto& Sockets = Mesh->GetMeshOnlySocketList();
    USkeletalMeshSocket* Socket = nullptr;
    for (USkeletalMeshSocket* Existing : Sockets)
        if (Existing && Existing->SocketName == SocketName) { Socket = Existing; break; }
    if (!Socket) { Socket = NewObject<USkeletalMeshSocket>(Mesh); Sockets.Add(Socket); }
    Socket->SocketName = SocketName;
    Socket->BoneName = BoneName;
    Socket->SetSocketLocalTransform(LocalTransform);
    Mesh->PostEditChange();
    Mesh->MarkPackageDirty();
    return true;
}

TArray<FTransform> UDMArtRigLibrary::GetSkeletonReferenceTransforms(USkeleton* Skeleton)
{
    return Skeleton ? Skeleton->GetReferenceSkeleton().GetRefBonePose() : TArray<FTransform>();
}

TArray<FName> UDMArtRigLibrary::GetSkeletonBoneNames(USkeleton* Skeleton)
{
    TArray<FName> Names;
    if (Skeleton)
        for (const FMeshBoneInfo& Bone : Skeleton->GetReferenceSkeleton().GetRefBoneInfo()) Names.Add(Bone.Name);
    return Names;
}

bool UDMArtRigLibrary::SetStaticMeshSocket(UStaticMesh* Mesh, FName SocketName, FVector Location)
{
    if (!Mesh || SocketName.IsNone()) { return false; }
    Mesh->Modify(); auto* Socket = Mesh->FindSocket(SocketName);
    if (!Socket) { Socket = NewObject<UStaticMeshSocket>(Mesh); Socket->SocketName = SocketName; Mesh->AddSocket(Socket); }
    Socket->RelativeLocation = Location; Mesh->PostEditChange(); Mesh->MarkPackageDirty(); return true;
}

bool UDMArtRigLibrary::BakeConstantBonePose(UAnimSequence* Animation, const TArray<FName>& Bones, const TArray<FTransform>& Transforms)
{
    if (!Animation || Bones.Num() != Transforms.Num()) { return false; }
    auto& Controller = Animation->GetController();
    const int32 Count = Animation->GetDataModel()->GetNumberOfKeys();
    if (Count < 1) { return false; }
    Controller.OpenBracket(FText::FromString(TEXT("Bake supported rifle pose")), false);
    bool bSuccess = true;
    for (int32 I = 0; I < Bones.Num(); ++I)
    {
        if (!Animation->GetDataModel()->IsValidBoneTrackName(Bones[I])) { Controller.AddBoneCurve(Bones[I], false); }
        TArray<FVector> Positions, Scales; TArray<FQuat> Rotations;
        Positions.Init(Transforms[I].GetTranslation(), Count); Scales.Init(Transforms[I].GetScale3D(), Count); Rotations.Init(Transforms[I].GetRotation(), Count);
        bSuccess &= Controller.SetBoneTrackKeys(Bones[I], Positions, Rotations, Scales, false);
    }
    Controller.CloseBracket(false); Animation->MarkPackageDirty(); return bSuccess;
}

bool UDMArtRigLibrary::CropAnimation(UAnimSequence* Animation, float StartSeconds, float EndSeconds)
{
    return CopyAnimationWindow(Animation, Animation, StartSeconds, EndSeconds);
}

bool UDMArtRigLibrary::CopyAnimationWindow(UAnimSequence* Source, UAnimSequence* Destination, float StartSeconds, float EndSeconds)
{
    if (!Source || !Destination || Source->GetSkeleton() != Destination->GetSkeleton() || StartSeconds < 0 || EndSeconds <= StartSeconds) { return false; }
    const auto* Model = Source->GetDataModel();
    const FFrameRate Rate = Model->GetFrameRate();
    const int32 Start = Rate.AsFrameNumber(StartSeconds).Value;
    const int32 End = FMath::Min(Model->GetNumberOfFrames(), Rate.AsFrameNumber(EndSeconds).Value);
    if (End <= Start) { return false; }
    TArray<FFrameNumber> Frames;
    for (int32 I = Start; I <= End; ++I) { Frames.Add(FFrameNumber(I)); }
    TArray<FName> Names; Model->GetBoneTrackNames(Names);
    TMap<FName, TArray<FTransform>> Tracks;
    for (FName Name : Names) { Model->GetBoneTrackTransforms(Name, Frames, Tracks.Add(Name)); }
    auto& Controller = Destination->GetController();
    Controller.OpenBracket(FText::FromString(TEXT("Copy movement transition window")), false);
    Controller.SetFrameRate(Rate, false);
    Controller.SetNumberOfFrames(FFrameNumber(End - Start), false);
    bool bSuccess = true;
    // Resizing sequence length does not shift bone tracks. Copy the selected keys explicitly.
    for (const auto& Track : Tracks)
    {
        if (!Destination->GetDataModel()->IsValidBoneTrackName(Track.Key)) { Controller.AddBoneCurve(Track.Key, false); }
        TArray<FVector> Positions, Scales; TArray<FQuat> Rotations;
        for (const FTransform& Key : Track.Value)
        { Positions.Add(Key.GetTranslation()); Rotations.Add(Key.GetRotation()); Scales.Add(Key.GetScale3D()); }
        bSuccess &= Controller.SetBoneTrackKeys(Track.Key, Positions, Rotations, Scales, false);
    }
    Controller.CloseBracket(false); Destination->MarkPackageDirty(); return bSuccess;
}
