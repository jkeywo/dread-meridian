#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DMHudModel.h"
#include "DMCombatHUD.generated.h"

class ADMCombatPlayerController;

/**
 * Canvas combat HUD for the sandbox slice: world-space rings and overhead bars plus the
 * always-visible screen layer. It draws only simulated state; unimplemented systems
 * (kits, Madness, objectives, Break/Resolve) are shown as explicit gaps, never as data.
 */
UCLASS()
class DREADMERIDIAN_API ADMCombatHUD : public AHUD
{
    GENERATED_BODY()
public:
    ADMCombatHUD();
    virtual void DrawHUD() override;

protected:
    // Protected rather than private so ADMShellHUD can draw its screens with the same
    // panels, labels and portraits as the combat HUD.
    // Hard references retain portrait assets in cooked builds as well as PIE.
    UPROPERTY() TArray<TObjectPtr<class UTexture2D>> PlayerPortraits;
    UPROPERTY() TArray<TObjectPtr<class UTexture2D>> EnemyPortraits;
    UPROPERTY() TObjectPtr<class UTexture2D> PortraitBackground;
    UPROPERTY() TObjectPtr<class UMaterialInterface> EnemyPortraitBackground;
    void Portrait(const FDMHudUnit& Unit, float X, float Y, float Size);
    float S = 1;

    void DrawWorldLayer(const FDMHudModel& Model);
    void DrawUnitWorld(const FDMHudUnit& Unit, bool bSelected);
    void DrawRing(const FVector& Feet, float Radius, FLinearColor Color, bool bDashed);
    /** Projected world-space line, for the pending Tripwire span. */
    void DrawSegment(const FVector& From, const FVector& To, FLinearColor Color);
    /** Dashed far arc plus the two straight edges: the targeting preview for cone abilities. */
    void DrawCone(const FVector& Origin, const FVector& Dir, float HalfAngleDeg, float Length, FLinearColor Color);
    bool ProjectPoint(const FVector& Location, FVector2D& Out) const;
    /** Projected ping marker: diamond, label, age bar, responder pips; dashed ground ring for subjective pings. */
    void DrawPing(const FDMHudPing& Ping);
    /** Hold-to-ping radial around the press origin while the controller reports it open. */
    void DrawPingRadial(const ADMCombatPlayerController& Player);
    void Diamond(float X, float Y, float R, FLinearColor Color, bool bFilled);
    /** Screen-space arc; angles are radians clockwise from 12 o'clock, matching the controller's radial sectors. */
    void Arc(float X, float Y, float Radius, float From, float To, FLinearColor Color, float Thickness);

    void DrawParty(const FDMHudModel& Model);
    void DrawRitual(const FDMHudModel& Model);
    void DrawTarget(const FDMHudModel& Model);
    void DrawEncounter(const FDMHudModel& Model);
    void DrawCondition(const FDMHudModel& Model);
    void DrawInvestigator(const FDMHudModel& Model);
    void DrawMinimap(const FDMHudModel& Model);
    void DrawControls();
    void DrawPrimaryFeedback();

    void Panel(float X, float Y, float W, float H, FLinearColor Border);
    void Bar(float X, float Y, float W, float H, float Fraction, FLinearColor Fill, float ShieldFraction = 0);
    void Label(const FString& Text, FLinearColor Color, float X, float Y, float Scale = 1.f);
    void Slot(float X, float Y, float Size, const FString& Key, const FString& Caption, FLinearColor Border, float Cooldown);
    float TextWidth(const FString& Text, float Scale) const;
};
