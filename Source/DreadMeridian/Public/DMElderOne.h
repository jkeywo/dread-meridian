#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMMadnessRules.h"
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
    bool AssignResonance(const TArray<class ADMCombatant*>& Roster, uint32 SeatDraw, const TArray<uint32>& FamilyDraws);
    bool IsResonant(const ADMCombatant* Actor) const;
    static TArray<EDMMadnessFamily> Families(EDMElderOne Boss, uint32 SeatDraw, const TArray<uint32>& Draws);
    EDMElderOne Identity() const { return State.Identity; }
    EDMBossPhase Phase() const { return State.Phase; }
private:
    FDMElderSnapshot State;
    bool Authority() const { return GetOwner() && GetOwner()->HasAuthority(); }
};
