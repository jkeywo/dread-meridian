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
            Until = Now + (bMoving ? StartDuration : StopDuration); TurnOffset = 0;
            Heading = Yaw; HeadingSince = Now;
        }
        else if (FMath::Abs(Delta) >= 45 && Now - LastTurn >= TurnDuration + .05f)
        {
            Phase = Delta < 0 ? EDMLocomotion::TurnLeft : EDMLocomotion::TurnRight;
            Until = Now + TurnDuration; LastTurn = Now; TurnStarted = Now;
            TurnOffset = -FMath::Clamp(Delta, -120.f, 120.f);
            Heading = Yaw; HeadingSince = Now;
        }
        else if (Now >= Until) { Phase = bMoving ? EDMLocomotion::Run : EDMLocomotion::Idle; TurnOffset = 0; }
        if (Now - HeadingSince > .2f) { Heading = Yaw; HeadingSince = Now; }
        return Phase;
    }
    float VisualYaw(float Now) const
    {
        return TurnOffset * (1.f - FMath::SmoothStep(0.f, TurnDuration, Now - TurnStarted));
    }
};
