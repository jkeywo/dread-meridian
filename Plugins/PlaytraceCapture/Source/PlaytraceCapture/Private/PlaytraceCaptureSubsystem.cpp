#include "PlaytraceCaptureSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogPlaytraceCapture, Log, All);

bool UPlaytraceCaptureSubsystem::BeginCapture(const TSharedRef<FJsonObject>& Metadata)
{
    if (bCapturing || !GetWorld() || GetWorld()->GetNetMode() == NM_Client) { return false; }
    if (!FParse::Param(FCommandLine::Get(), TEXT("PlaytraceCapture"))) { return false; }
    RunId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Playtrace"), RunId);
    if (!IFileManager::Get().MakeDirectory(*Directory, true))
    {
        UE_LOG(LogPlaytraceCapture, Error, TEXT("Cannot create capture directory: %s"), *Directory);
        return false;
    }
    OutputPath = FPaths::Combine(Directory, TEXT("events.jsonl"));
    Sequence = 0;
    StartedAt = FPlatformTime::Seconds();
    bCapturing = true;
    UE_LOG(LogPlaytraceCapture, Display, TEXT("Capture: %s"), *OutputPath);
    return RecordEvent(TEXT("run.started"), Metadata);
}

bool UPlaytraceCaptureSubsystem::RecordEvent(const FString& EventType, const TSharedRef<FJsonObject>& Data)
{
    if (!bCapturing) { return false; }
    TSharedRef<FJsonObject> Event = MakeShared<FJsonObject>();
    Event->SetNumberField(TEXT("schema_version"), 1);
    Event->SetStringField(TEXT("run_id"), RunId);
    Event->SetNumberField(TEXT("sequence"), Sequence);
    Event->SetStringField(TEXT("event_type"), EventType);
    Event->SetNumberField(TEXT("elapsed_seconds"), FPlatformTime::Seconds() - StartedAt);
    Event->SetStringField(TEXT("authority"), TEXT("server"));
    Event->SetStringField(TEXT("visibility"), TEXT("developer"));
    Event->SetObjectField(TEXT("data"), Data);
    FString Line;
    const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Line);
    if (!FJsonSerializer::Serialize(Event, Writer) ||
        !FFileHelper::SaveStringToFile(Line + TEXT("\n"), *OutputPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
            &IFileManager::Get(), FILEWRITE_Append))
    {
        UE_LOG(LogPlaytraceCapture, Error, TEXT("Capture write failed; capture stopped: %s"), *OutputPath);
        bCapturing = false;
        return false;
    }
    ++Sequence;
    return true;
}

void UPlaytraceCaptureSubsystem::EndCapture(const FString& Outcome)
{
    if (!bCapturing) { return; }
    TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("outcome"), Outcome);
    RecordEvent(TEXT("run.ended"), Data);
    bCapturing = false;
}

void UPlaytraceCaptureSubsystem::Deinitialize()
{
    EndCapture(TEXT("aborted"));
    Super::Deinitialize();
}
