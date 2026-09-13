#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMElderOne.generated.h"

enum class EDMElderOne : uint8 { Shub, Nyarlathotep };
enum class EDMBossPhase : uint8 { Dormant, Rooted, Mobile, Frenzy, Victory, Defeat };
struct DREADMERIDIAN_API FDMElderSnapshot
{
    int32 Version = 1;
    bool bSelected = false;
    EDMElderOne Identity = EDMElderOne::Shub;
    EDMBossPhase Phase = EDMBossPhase::Dormant;
    FString ResonantId;
};
/** Lives on authority GameMode. Hidden identity never enters shared GameState. */
UCLASS()
class DREADMERIDIAN_API UDMElderOne : public UActorComponent
{
    GENERATED_BODY()
public:
    bool Select(uint32 Draw);
    bool Begin();
    bool Advance(EDMBossPhase Phase);
    bool Finish(bool bVictory);
    FDMElderSnapshot Capture() const { return State; }
    bool Restore(const FDMElderSnapshot& Snapshot);
    EDMElderOne Identity() const { return State.Identity; }
    EDMBossPhase Phase() const { return State.Phase; }
private:
    FDMElderSnapshot State;
    bool Authority() const { return GetOwner() && GetOwner()->HasAuthority(); }
};
