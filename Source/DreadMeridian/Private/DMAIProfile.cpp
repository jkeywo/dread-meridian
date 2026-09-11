#include "DMAIProfile.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"

namespace
{
    // AllProfileNames order: roles in EDMSmuggler order, kinds in EDMInvestigator order, then the generic raider.
    const TCHAR* ProfileNames[] = { TEXT("Gunman"), TEXT("Bruiser"), TEXT("Lookout"), TEXT("Bomber"), TEXT("GangBoss"),
        TEXT("Sapper"), TEXT("Photographer"), TEXT("Medium"), TEXT("Smuggler"), TEXT("Raider") };
    constexpr int32 RoleCount = 5, KindCount = 4;

    UEnum* EnumOf(FProperty* Prop, FNumericProperty*& OutUnderlying)
    {
        OutUnderlying = nullptr;
        if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop)) { OutUnderlying = EnumProp->GetUnderlyingProperty(); return EnumProp->GetEnum(); }
        if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop)) { if (ByteProp->Enum) { OutUnderlying = ByteProp; return ByteProp->Enum; } }
        return nullptr;
    }

    /** Patches the Struct instance at Data from Json by UPROPERTY name: numeric, bool, enum (name or value), string, nested struct, array of structs (replaced wholesale). */
    bool PatchStruct(const UScriptStruct* Struct, void* Data, const TSharedPtr<FJsonObject>& Json, const FString& Where, const TSet<FString>& Skip)
    {
        bool bApplied = false;
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Json->Values)
        {
            const FString& Key = Pair.Key;
            const TSharedPtr<FJsonValue>& Value = Pair.Value;
            if (Skip.Contains(Key) || !Value.IsValid()) { continue; }
            FProperty* Prop = Struct->FindPropertyByName(FName(*Key));
            if (!Prop) { UE_LOG(LogTemp, Warning, TEXT("DMAIProfile: unknown key %s.%s ignored"), *Where, *Key); continue; }
            void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Data);
            FNumericProperty* Underlying = nullptr;
            bool bOk = false;
            if (UEnum* Enum = EnumOf(Prop, Underlying))
            {
                int64 Enumerator = INDEX_NONE;
                if (Value->Type == EJson::String) { Enumerator = Enum->GetValueByNameString(Value->AsString()); }
                else if (Value->Type == EJson::Number) { Enumerator = static_cast<int64>(Value->AsNumber()); }
                if (Enumerator != INDEX_NONE && Enum->IsValidEnumValue(Enumerator)) { Underlying->SetIntPropertyValue(ValuePtr, Enumerator); bOk = true; }
            }
            else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
            {
                if (Value->Type == EJson::Boolean) { BoolProp->SetPropertyValue(ValuePtr, Value->AsBool()); bOk = true; }
                else if (Value->Type == EJson::Number) { BoolProp->SetPropertyValue(ValuePtr, Value->AsNumber() != 0); bOk = true; }
            }
            else if (FNumericProperty* NumProp = CastField<FNumericProperty>(Prop))
            {
                if (Value->Type == EJson::Number)
                {
                    if (NumProp->IsFloatingPoint()) { NumProp->SetFloatingPointPropertyValue(ValuePtr, Value->AsNumber()); }
                    else { NumProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(FMath::RoundToDouble(Value->AsNumber()))); }
                    bOk = true;
                }
            }
            else if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
            {
                if (Value->Type == EJson::String) { StrProp->SetPropertyValue(ValuePtr, Value->AsString()); bOk = true; }
            }
            else if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
            {
                if (Value->Type == EJson::Object) { bOk = PatchStruct(StructProp->Struct, ValuePtr, Value->AsObject(), Where + TEXT(".") + Key, TSet<FString>()); }
            }
            else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
            {
                FStructProperty* Inner = CastField<FStructProperty>(ArrayProp->Inner);
                if (Inner && Value->Type == EJson::Array)
                {
                    // Validate first: every entry must be an object that patches at least one field, otherwise the
                    // existing array is left untouched (an emptied Considerations list would silently score 1 every tick).
                    const TArray<TSharedPtr<FJsonValue>>& Entries = Value->AsArray();
                    TArray<TSharedPtr<FStructOnScope>> Scratch;
                    bool bValid = true;
                    for (int32 Index = 0; Index < Entries.Num() && bValid; ++Index)
                    {
                        const TSharedPtr<FJsonValue>& Entry = Entries[Index];
                        const FString EntryWhere = FString::Printf(TEXT("%s.%s[%d]"), *Where, *Key, Index);
                        if (!Entry.IsValid() || Entry->Type != EJson::Object)
                        { UE_LOG(LogTemp, Warning, TEXT("DMAIProfile: %s is not an object; %s.%s left unchanged"), *EntryWhere, *Where, *Key); bValid = false; break; }
                        TSharedPtr<FStructOnScope> Item = MakeShared<FStructOnScope>(Inner->Struct);
                        if (!PatchStruct(Inner->Struct, Item->GetStructMemory(), Entry->AsObject(), EntryWhere, TSet<FString>()))
                        { UE_LOG(LogTemp, Warning, TEXT("DMAIProfile: %s applied nothing; %s.%s left unchanged"), *EntryWhere, *Where, *Key); bValid = false; break; }
                        Scratch.Add(Item);
                    }
                    if (bValid)
                    {
                        FScriptArrayHelper Helper(ArrayProp, ValuePtr);
                        Helper.EmptyValues();
                        for (const TSharedPtr<FStructOnScope>& Item : Scratch)
                        { Inner->Struct->CopyScriptStruct(Helper.GetRawPtr(Helper.AddValue()), Item->GetStructMemory()); }
                        bOk = true;
                    }
                }
            }
            if (!bOk) { UE_LOG(LogTemp, Warning, TEXT("DMAIProfile: could not apply %s.%s (wrong JSON type or unsupported property)"), *Where, *Key); }
            bApplied |= bOk;
        }
        return bApplied;
    }

    TSharedPtr<FJsonObject> StructToJson(const UScriptStruct* Struct, const void* Data, const TSet<FString>& Skip)
    {
        TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
        for (TFieldIterator<FProperty> It(Struct); It; ++It)
        {
            FProperty* Prop = *It;
            const FString Key = Prop->GetName();
            if (Skip.Contains(Key)) { continue; }
            const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Data);
            FNumericProperty* Underlying = nullptr;
            if (UEnum* Enum = EnumOf(Prop, Underlying)) { Out->SetStringField(Key, Enum->GetNameStringByValue(Underlying->GetSignedIntPropertyValue(ValuePtr))); }
            else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop)) { Out->SetBoolField(Key, BoolProp->GetPropertyValue(ValuePtr)); }
            else if (FNumericProperty* NumProp = CastField<FNumericProperty>(Prop))
            { Out->SetNumberField(Key, NumProp->IsFloatingPoint() ? NumProp->GetFloatingPointPropertyValue(ValuePtr) : static_cast<double>(NumProp->GetSignedIntPropertyValue(ValuePtr))); }
            else if (FStrProperty* StrProp = CastField<FStrProperty>(Prop)) { Out->SetStringField(Key, StrProp->GetPropertyValue(ValuePtr)); }
            else if (FStructProperty* StructProp = CastField<FStructProperty>(Prop)) { Out->SetObjectField(Key, StructToJson(StructProp->Struct, ValuePtr, TSet<FString>())); }
            else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
            {
                FStructProperty* Inner = CastField<FStructProperty>(ArrayProp->Inner);
                if (!Inner) { continue; }
                FScriptArrayHelper Helper(ArrayProp, ValuePtr);
                TArray<TSharedPtr<FJsonValue>> Entries;
                for (int32 Index = 0; Index < Helper.Num(); ++Index)
                { Entries.Add(MakeShared<FJsonValueObject>(StructToJson(Inner->Struct, Helper.GetRawPtr(Index), TSet<FString>()))); }
                Out->SetArrayField(Key, Entries);
            }
            // Maps (Abilities) and object references are handled by the callers by name.
        }
        return Out;
    }

    const TSet<FString>& TopLevelSkip() { static const TSet<FString> Skip = { FString(TEXT("Actions")), FString(TEXT("Abilities")) }; return Skip; }

    /**
     * DMUtilityAI::DefaultWeights bakes a few consideration bookends from top-level scalars (FleeExit, KeepDistanceExit,
     * PickupRadius, FollowDistance). A JSON override of one of those scalars re-derives the matching curve so the
     * latch and its curve keep agreeing; an explicit Actions.<Action>.Considerations override still wins (applied after).
     */
    void RefreshDerivedBookends(FDMAIWeights& W, const FDMAIWeights& Before, const FString& Where)
    {
        auto Fix = [&](EDMAIAction Action, EDMAIInput Input, float Old, float New, TFunctionRef<void(FDMAICurveSpec&)> Apply)
        {
            if (FMath::IsNearlyEqual(Old, New)) { return; }
            FDMAIActionSpec* Spec = W.FindAction(Action);
            if (!Spec) { return; }
            for (FDMAIConsideration& Con : Spec->Considerations)
            {
                if (Con.Input != Input) { continue; }
                Apply(Con.Curve);
                UE_LOG(LogTemp, Display, TEXT("DMAIProfile: %s.Actions.%s %s bookends re-derived (%g..%g)"), *Where, DMUtilityAI::ActionName(Action), DMUtilityAI::InputName(Input), Con.Curve.Min, Con.Curve.Max);
            }
        };
        Fix(EDMAIAction::Flee, EDMAIInput::SelfHealthFrac, Before.FleeExit, W.FleeExit, [&](FDMAICurveSpec& C) { C.Max = W.FleeExit * 1.5f; });
        Fix(EDMAIAction::KeepDistance, EDMAIInput::Distance, Before.KeepDistanceExit, W.KeepDistanceExit,
            [&](FDMAICurveSpec& C) { if (Before.KeepDistanceExit > UE_KINDA_SMALL_NUMBER) { C.Max *= W.KeepDistanceExit / Before.KeepDistanceExit; } });
        Fix(EDMAIAction::SeekPickup, EDMAIInput::Distance, Before.PickupRadius, W.PickupRadius, [&](FDMAICurveSpec& C) { C.Max = W.PickupRadius; });
        Fix(EDMAIAction::FollowLeader, EDMAIInput::LeaderDistance, Before.FollowDistance, W.FollowDistance,
            [&](FDMAICurveSpec& C) { C.Min = W.FollowDistance; C.Max = W.FollowDistance + 600; });
    }
    const TSet<FString>& ActionSkip() { static const TSet<FString> Skip = { FString(TEXT("Action")) }; return Skip; }
}

FString UDMAIProfile::ProfileName(EDMSmuggler Role, EDMInvestigator Kind)
{
    if (Role != EDMSmuggler::None) { return ProfileNames[FMath::Clamp(static_cast<int32>(Role) - 1, 0, RoleCount - 1)]; }
    if (Kind != EDMInvestigator::None) { return ProfileNames[RoleCount + FMath::Clamp(static_cast<int32>(Kind) - 1, 0, KindCount - 1)]; }
    return ProfileNames[RoleCount + KindCount];
}

FString UDMAIProfile::AssetPath(const FString& ProfileName) { return TEXT("/Game/DreadMeridian/AI/AIP_") + ProfileName; }

bool UDMAIProfile::ProfileFromName(const FString& Name, EDMSmuggler& OutRole, EDMInvestigator& OutKind)
{
    OutRole = EDMSmuggler::None; OutKind = EDMInvestigator::None;
    for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(ProfileNames)); ++Index)
    {
        if (!Name.Equals(ProfileNames[Index], ESearchCase::IgnoreCase)) { continue; }
        if (Index < RoleCount) { OutRole = static_cast<EDMSmuggler>(Index + 1); }
        else if (Index < RoleCount + KindCount) { OutKind = static_cast<EDMInvestigator>(Index - RoleCount + 1); }
        return true;
    }
    return false;
}

TArray<FString> UDMAIProfile::AllProfileNames()
{
    TArray<FString> Out;
    for (const TCHAR* Name : ProfileNames) { Out.Add(Name); }
    return Out;
}

void UDMAIProfile::ApplyDefaults()
{
    if (ProfileId.IsEmpty()) { ProfileId = GetName(); ProfileId.RemoveFromStart(TEXT("AIP_")); }
    EDMSmuggler Role = EDMSmuggler::None;
    EDMInvestigator Kind = EDMInvestigator::None;
    if (!ProfileFromName(ProfileId, Role, Kind))
    { UE_LOG(LogTemp, Warning, TEXT("DMAIProfile %s: unknown ProfileId '%s', applying Raider defaults"), *GetName(), *ProfileId); }
    Weights = DMUtilityAI::DefaultWeights(Role, Kind);
#if WITH_EDITOR
    if (IsAsset()) { MarkPackageDirty(); }
#endif
}

UDMAIProfile* UDMAIProfile::Resolve(UObject* Outer, EDMSmuggler Role, EDMInvestigator Kind, const FString& OverrideJsonPath)
{
    static TSet<FString> Logged;
    static TMap<FString, TSharedPtr<FJsonObject>> OverrideCache;
    const FString Name = ProfileName(Role, Kind);
    const FString Package = AssetPath(Name);
    UObject* Owner = Outer ? Outer : GetTransientPackage();
    UDMAIProfile* Profile = nullptr;
    const TCHAR* Source = TEXT("default");
    if (FPackageName::DoesPackageExist(Package))
    {
        Profile = LoadObject<UDMAIProfile>(nullptr, *(Package + TEXT(".") + FPackageName::GetShortName(Package)));
        if (Profile) { Source = TEXT("asset"); }
    }
    TSharedPtr<FJsonObject> Root;
    if (!OverrideJsonPath.IsEmpty())
    {
        if (const TSharedPtr<FJsonObject>* Cached = OverrideCache.Find(OverrideJsonPath)) { Root = *Cached; }
        else
        {
            FString Text;
            if (!FFileHelper::LoadFileToString(Text, *OverrideJsonPath)) { UE_LOG(LogTemp, Error, TEXT("DMAIProfile: could not read -DMAIWeights file %s"), *OverrideJsonPath); }
            else if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
            { UE_LOG(LogTemp, Error, TEXT("DMAIProfile: could not parse -DMAIWeights file %s"), *OverrideJsonPath); Root.Reset(); }
            OverrideCache.Add(OverrideJsonPath, Root);
        }
    }
    const TSharedPtr<FJsonObject>* Star = nullptr;
    const TSharedPtr<FJsonObject>* Section = nullptr;
    if (Root.IsValid()) { Root->TryGetObjectField(TEXT("*"), Star); Root->TryGetObjectField(Name, Section); }
    // Never patch a loaded asset in place: overrides go onto a transient copy.
    if (Profile && (Star || Section)) { Profile = DuplicateObject<UDMAIProfile>(Profile, Owner); }
    if (!Profile)
    {
        Profile = NewObject<UDMAIProfile>(Owner);
        Profile->ProfileId = Name;
        Profile->ApplyDefaults();
    }
    if (Profile->ProfileId.IsEmpty()) { Profile->ProfileId = Name; }
    int32 Applied = 0;
    if (Star && Profile->ApplyJsonOverrides(*Star)) { ++Applied; }
    if (Section && Profile->ApplyJsonOverrides(*Section)) { ++Applied; }
    if (!Logged.Contains(Name))
    {
        Logged.Add(Name);
        UE_LOG(LogTemp, Display, TEXT("DREAD_AI_PROFILE name=%s source=%s overrides=%d"), *Name, Source, Applied);
    }
    return Profile;
}

bool UDMAIProfile::ApplyJsonOverrides(const TSharedPtr<FJsonObject>& Section)
{
    if (!Section.IsValid()) { return false; }
    const FString Where = ProfileId.IsEmpty() ? GetName() : ProfileId;
    const FDMAIWeights Before = Weights;
    bool bApplied = PatchStruct(FDMAIWeights::StaticStruct(), &Weights, Section, Where, TopLevelSkip());
    if (bApplied) { RefreshDerivedBookends(Weights, Before, Where); }
    const TSharedPtr<FJsonObject>* Actions = nullptr;
    if (Section->TryGetObjectField(TEXT("Actions"), Actions))
    {
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Actions)->Values)
        {
            const EDMAIAction Action = DMUtilityAI::ActionFromName(Pair.Key);
            FDMAIActionSpec* Spec = Action == EDMAIAction::None || Action == EDMAIAction::Count ? nullptr : Weights.FindAction(Action);
            if (!Spec || !Pair.Value.IsValid() || Pair.Value->Type != EJson::Object)
            { UE_LOG(LogTemp, Warning, TEXT("DMAIProfile: %s.Actions.%s ignored (not an action of this profile or not an object)"), *Where, *Pair.Key); continue; }
            if (!PatchStruct(FDMAIActionSpec::StaticStruct(), Spec, Pair.Value->AsObject(), Where + TEXT(".Actions.") + Pair.Key, ActionSkip())) { continue; }
            bApplied = true;
            // bEnabled is derived from Weight at default time; a Weight override re-derives it unless the JSON set bEnabled itself.
            if (!Pair.Value->AsObject()->HasField(TEXT("bEnabled")))
            {
                const bool bWasEnabled = Spec->bEnabled;
                Spec->bEnabled = Spec->Weight > 0;
                if (bWasEnabled != Spec->bEnabled)
                { UE_LOG(LogTemp, Display, TEXT("DMAIProfile: %s.Actions.%s %s by its Weight override"), *Where, *Pair.Key, Spec->bEnabled ? TEXT("enabled") : TEXT("disabled")); }
            }
        }
    }
    const TSharedPtr<FJsonObject>* Abilities = nullptr;
    if (Section->TryGetObjectField(TEXT("Abilities"), Abilities))
    {
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Abilities)->Values)
        {
            const EDMAIAction Action = DMUtilityAI::ActionFromName(Pair.Key);
            FDMAIAbilityTemplate* Template = Action == EDMAIAction::None || Action == EDMAIAction::Count ? nullptr : Weights.Abilities.Find(Action);
            if (!Template || !Pair.Value.IsValid() || Pair.Value->Type != EJson::Object)
            { UE_LOG(LogTemp, Warning, TEXT("DMAIProfile: %s.Abilities.%s ignored (not an ability of this profile or not an object)"), *Where, *Pair.Key); continue; }
            bApplied |= PatchStruct(FDMAIAbilityTemplate::StaticStruct(), Template, Pair.Value->AsObject(), Where + TEXT(".Abilities.") + Pair.Key, TSet<FString>());
        }
    }
    return bApplied;
}

TSharedPtr<FJsonObject> UDMAIProfile::ToJson() const
{
    TSharedPtr<FJsonObject> Out = StructToJson(FDMAIWeights::StaticStruct(), &Weights, TopLevelSkip());
    TSharedPtr<FJsonObject> Actions = MakeShared<FJsonObject>();
    for (const FDMAIActionSpec& Spec : Weights.Actions)
    { Actions->SetObjectField(DMUtilityAI::ActionName(Spec.Action), StructToJson(FDMAIActionSpec::StaticStruct(), &Spec, ActionSkip())); }
    Out->SetObjectField(TEXT("Actions"), Actions);
    TSharedPtr<FJsonObject> Abilities = MakeShared<FJsonObject>();
    for (const TPair<EDMAIAction, FDMAIAbilityTemplate>& Pair : Weights.Abilities)
    { Abilities->SetObjectField(DMUtilityAI::ActionName(Pair.Key), StructToJson(FDMAIAbilityTemplate::StaticStruct(), &Pair.Value, TSet<FString>())); }
    Out->SetObjectField(TEXT("Abilities"), Abilities);
    return Out;
}
