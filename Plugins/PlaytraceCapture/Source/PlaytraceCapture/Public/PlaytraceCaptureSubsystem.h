#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlaytraceCaptureSubsystem.generated.h"

/** One authority-owned run per GameInstance. Captures only explicitly supplied public data. */
UCLASS()
class PLAYTRACECAPTURE_API UPlaytraceCaptureSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    bool BeginCapture(const TSharedRef<FJsonObject>& Metadata);
    bool RecordEvent(const FString& EventType, const TSharedRef<FJsonObject>& Data);
    void EndCapture(const FString& Outcome);
    virtual void Deinitialize() override;

private:
    FString OutputPath;
    FString RunId;
    int32 Sequence = 0;
    double StartedAt = 0;
    bool bCapturing = false;
};
