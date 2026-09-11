#include "DMHarnessHUD.h"
#include "DMGameState.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"

void ADMHarnessHUD::DrawHUD()
{
    Super::DrawHUD();
    DrawRect(FLinearColor(0.025f, 0.035f, 0.045f, 1.0f), 0, 0, Canvas->ClipX, Canvas->ClipY);
    DrawText(TEXT("DREAD MERIDIAN"), FLinearColor(0.85f, 0.68f, 0.35f), 40, 40, nullptr, 2.5f);
    DrawText(TEXT("Foundation harness | no combat or production bots yet"), FLinearColor::White, 40, 100);
    if (const ADMGameState* State = GetWorld()->GetGameState<ADMGameState>())
    {
        const FDMRunState Run = State->GetRunState();
        DrawText(FString::Printf(TEXT("Phase: %s | Ritual: %s | Progress: %d"),
            *StaticEnum<EDMRunPhase>()->GetNameStringByValue(static_cast<int64>(Run.Phase)),
            *StaticEnum<EDMRitualStage>()->GetNameStringByValue(static_cast<int64>(Run.RitualStage)),
            Run.RitualProgress), FLinearColor::White, 40, 145);
    }
    DrawText(TEXT("Open console (~): DMRitualAdvance 100 | DMSummon | DMFinish true / false"),
        FLinearColor::White, 40, 195);
    DrawText(TEXT("Capture is opt-in via -PlaytraceCapture. Evidence is written to Saved/Playtrace."),
        FLinearColor(0.55f, 0.65f, 0.68f), 40, 235);
}
