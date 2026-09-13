#include "DMElderOne.h"
bool UDMElderOne::Select(uint32 Draw)
{
    if (!Authority() || State.bSelected) { return false; }
    State.bSelected = true; State.Identity = static_cast<EDMElderOne>(Draw%2); return true;
}
bool UDMElderOne::Begin()
{
    if (!Authority() || !State.bSelected || State.Phase != EDMBossPhase::Dormant) { return false; }
    State.Phase = EDMBossPhase::Rooted; return true;
}
bool UDMElderOne::Advance(EDMBossPhase Phase)
{
    if (!Authority() || !((State.Phase == EDMBossPhase::Rooted && Phase == EDMBossPhase::Mobile) || (State.Phase == EDMBossPhase::Mobile && Phase == EDMBossPhase::Frenzy))) { return false; }
    State.Phase = Phase; return true;
}
bool UDMElderOne::Finish(bool bVictory)
{
    if (!Authority() || State.Phase < EDMBossPhase::Rooted || State.Phase > EDMBossPhase::Frenzy) { return false; }
    State.Phase = bVictory ? EDMBossPhase::Victory : EDMBossPhase::Defeat; return true;
}
bool UDMElderOne::Restore(const FDMElderSnapshot& S)
{
    if (!Authority() || S.Version != 1 || static_cast<uint8>(S.Identity)>1 || static_cast<uint8>(S.Phase)>static_cast<uint8>(EDMBossPhase::Defeat)
        || (!S.bSelected && S.Phase != EDMBossPhase::Dormant)) { return false; }
    State = S; return true;
}
