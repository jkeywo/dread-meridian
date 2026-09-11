#pragma once

#include "CoreMinimal.h"

/**
 * Shared canvas palette for every Dread Meridian screen: the combat HUD and the shell
 * (main menu, lobby, case report). Cold near-black grounds, brass chrome, one occult accent.
 * Source of truth for the mockups in design/ui-mockups.
 */
namespace DMHud
{
    static const FLinearColor Ground(.035f, .055f, .055f, .86f);
    static const FLinearColor Sunk(0, 0, 0, .6f);
    static const FLinearColor Brass(.78f, .63f, .29f);
    static const FLinearColor BrassDim(.78f, .63f, .29f, .3f);
    static const FLinearColor Bone(.87f, .84f, .77f);
    static const FLinearColor Muted(.55f, .58f, .56f);
    static const FLinearColor Health(.49f, .66f, .43f);
    static const FLinearColor Shield(.62f, .73f, .79f);
    static const FLinearColor Danger(.82f, .29f, .22f);
    static const FLinearColor Ally(.31f, .54f, .48f);
    static const FLinearColor Gap(.42f, .45f, .44f);
}

/**
 * Full-screen shell pages, converted from the sRGB hexes in design/ui-mockups. The combat HUD's
 * palette above is authored directly in linear space and reads lighter on screen; a page that
 * fills the frame has to be the colour the mockups specify, so these go through FromSRGBColor.
 */
namespace DMShell
{
    /** #0B1212 */
    static const FLinearColor Page = FLinearColor::FromSRGBColor(FColor(0x0B, 0x12, 0x12));
    /** #111A1A */
    static const FLinearColor Panel = FLinearColor::FromSRGBColor(FColor(0x11, 0x1A, 0x1A));
    /** #C7A14A */
    static const FLinearColor Brass = FLinearColor::FromSRGBColor(FColor(0xC7, 0xA1, 0x4A));
    /** #DED6C4 */
    static const FLinearColor Bone = FLinearColor::FromSRGBColor(FColor(0xDE, 0xD6, 0xC4));
    /** #B9B3A4 */
    static const FLinearColor Body = FLinearColor::FromSRGBColor(FColor(0xB9, 0xB3, 0xA4));
    /** #8C948F */
    static const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(0x8C, 0x94, 0x8F));
    /** #D14A38 */
    static const FLinearColor Danger = FLinearColor::FromSRGBColor(FColor(0xD1, 0x4A, 0x38));
    /** #7DA86E */
    static const FLinearColor Good = FLinearColor::FromSRGBColor(FColor(0x7D, 0xA8, 0x6E));
    /** #2A3535, the hairline rule and inert chrome. */
    static const FLinearColor Rule = FLinearColor::FromSRGBColor(FColor(0x2A, 0x35, 0x35));
    /** #4A5352, drawn but plainly unavailable. */
    static const FLinearColor Disabled = FLinearColor::FromSRGBColor(FColor(0x4A, 0x53, 0x52));
}
