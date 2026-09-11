#pragma once
#include "CoreMinimal.h"
#if WITH_EDITOR
#include "Misc/ConfigCacheIni.h"

// Shared by the editor toolbar and PIE authority; no UnrealEd dependency in runtime.
namespace DMEditorPlaySelection
{
    inline const TArray<FString>& Choices()
    {
        static const TArray<FString> Values = { TEXT("Sapper"), TEXT("Photographer"), TEXT("Medium"), TEXT("Smuggler") };
        return Values;
    }
    inline FString Load()
    {
        FString Value;
        GConfig->GetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), Value, GEditorPerProjectIni);
        return Choices().Contains(Value) ? Value : TEXT("Sapper");
    }
    inline bool LoadBotControl()
    {
        bool Value = false;
        GConfig->GetBool(TEXT("DreadMeridian.EditorPlay"), TEXT("BotControlled"), Value, GEditorPerProjectIni);
        return Value;
    }
    inline void SaveBotControl(bool Value)
    {
        GConfig->SetBool(TEXT("DreadMeridian.EditorPlay"), TEXT("BotControlled"), Value, GEditorPerProjectIni);
        GConfig->Flush(false, GEditorPerProjectIni);
    }
    inline void Save(const FString& Value)
    {
        if (!Choices().Contains(Value)) { return; }
        GConfig->SetString(TEXT("DreadMeridian.EditorPlay"), TEXT("Investigator"), *Value, GEditorPerProjectIni);
        GConfig->Flush(false, GEditorPerProjectIni);
    }
}
#endif
