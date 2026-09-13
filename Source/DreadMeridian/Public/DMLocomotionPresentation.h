#pragma once
#include "CoreMinimal.h"

// Local cosmetic state; never writes to movement, input, or replicated combat state.
enum class EDMLocomotion : uint8 { Idle, Run, Start, Stop, TurnLeft, TurnRight };
struct FDMLocomotionPresentation
{
    static constexpr float StartDuration = .35f, StopDuration = .55f, TurnDuration = .45f;
    EDMLocomotion Phase = EDMLocomotion::Idle;
    bool bInitialized = false, bMoving = false;
    float Until = 0, Heading = 0, HeadingSince = 0, LastTurn = -1;
    float TurnOffset = 0, TurnStarted = 0;
    void Reset(float Speed, float Yaw, float Now)
    {
        bInitialized = true; bMoving = Speed > 35; Heading = Yaw; HeadingSince = Now;
        Phase = bMoving ? EDMLocomotion::Run : EDMLocomotion::Idle;
        Until = Now; TurnOffset = 0;
    }
    EDMLocomotion Update(float Speed, float Yaw, float Now)
    {
        if (!bInitialized) { Reset(Speed, Yaw, Now); return Phase; }
        const bool bNextMoving = Speed > (bMoving ? 10.f : 35.f);
        const float Delta = FMath::FindDeltaAngleDegrees(Heading, Yaw);
        if (bNextMoving != bMoving)
        {
            bMoving = bNextMoving; Phase = bMoving ? EDMLocomotion::Start : EDMLocomotion::Stop;
            Until = Now + (bMoving ? StartDuration : StopDuration);
            Heading = Yaw; HeadingSince = Now;
        }
        else if (FMath::Abs(Delta) >= 45 && Now - LastTurn >= TurnDuration + .05f)
        {
            Phase = Delta < 0 ? EDMLocomotion::TurnLeft : EDMLocomotion::TurnRight;
            Until = Now + TurnDuration; LastTurn = Now; TurnStarted = Now;
            TurnOffset = -FMath::Clamp(Delta, -120.f, 120.f);
            Heading = Yaw; HeadingSince = Now;
        }
        else if (Now >= Until) { Phase = bMoving ? EDMLocomotion::Run : EDMLocomotion::Idle; }
        if (Now - HeadingSince > .2f) { Heading = Yaw; HeadingSince = Now; }
        return Phase;
    }
    /** How long VisualYaw takes to rise to (near) its full held offset once a turn is detected; see below. */
    static constexpr float TurnBlendIn = .08f;
    /**
     * The turn lean rises quickly to TurnOffset (holding the pre-turn visual heading while the capsule turns
     * underneath it) then decays back to 0 over TurnDuration, matching the original design - but the rise is
     * itself eased over TurnBlendIn instead of snapping to full offset in the same frame the turn is detected,
     * which is what actually produced the visible "blink" when turns retrigger often (e.g. fast mouse-directed
     * click-to-move): every retrigger jumped the mesh's yaw by up to 120 degrees in a single frame.
     */
    float VisualYaw(float Now) const
    {
        const float T = Now - TurnStarted;
        const float RiseFactor = FMath::SmoothStep(0.f, TurnBlendIn, T);
        const float DecayFactor = 1.f - FMath::SmoothStep(0.f, TurnDuration, T);
        return TurnOffset * RiseFactor * DecayFactor;
    }
    /** Buckets MoveYaw relative to FacingYaw into 8 45-degree wedges: 0=Fwd,1=FwdR,2=Right,3=BwdR,4=Bwd,5=BwdL,6=Left,7=FwdL. */
    static int32 StrafeOctant(float FacingYaw, float MoveYaw)
    {
        const float Delta = FMath::FindDeltaAngleDegrees(FacingYaw, MoveYaw);
        return (FMath::RoundToInt(Delta / 45.f) + 8) % 8;
    }
};
