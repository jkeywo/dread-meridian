#pragma once

#include "CoreMinimal.h"
#include "DMCombatHUD.h"
#include "DMShellState.h"
#include "Fonts/SlateFontInfo.h"
#include "DMShellHUD.generated.h"

/**
 * Canvas front end for the shell level: main menu, expedition lobby, loading and case report.
 * During the mission it defers entirely to the combat HUD.
 *
 * Screens follow design/ui-mockups. Everything the shell cannot yet compute is drawn with a
 * visible PLACEHOLDER tag rather than invented numbers; see docs/shell-flow.md for the list.
 */
UCLASS()
class DREADMERIDIAN_API ADMShellHUD : public ADMCombatHUD
{
    GENERATED_BODY()
public:
    ADMShellHUD();
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;
    virtual void NotifyHitBoxEndCursorOver(FName BoxName) override;

private:
    /** Design space is 1440x900; screens letterbox into the real canvas. */
    static constexpr float DesignWidth = 1440.f;
    static constexpr float DesignHeight = 900.f;
    /** Runtime font, so headline type scales cleanly instead of smearing a bitmap glyph. */
    UPROPERTY() TObjectPtr<class UFont> ScreenFont;
    float UiScale = 1;
    float UiOriginX = 0;
    float UiOriginY = 0;
    FName Hovered;

    EDMShellPhase CurrentPhase() const;
    /** Slate font sized for DesignPixels of cap height, rasterised at that size rather than scaled up. */
    FSlateFontInfo FontAt(float DesignPixels) const;
    void LayOutDesignSpace();
    float X(float DesignX) const { return UiOriginX + DesignX * UiScale; }
    float Y(float DesignY) const { return UiOriginY + DesignY * UiScale; }

    void DrawMainMenu();
    void DrawLobby();
    void DrawLoading();
    void DrawCaseReport();

    // ---- design-space drawing helpers
    void Fill(float DX, float DY, float DW, float DH, FLinearColor Color);
    void Frame(float DX, float DY, float DW, float DH, FLinearColor Border);
    /** Corner brackets, the panel treatment shared with the mockups. */
    void Brackets(float DX, float DY, float DW, float DH, FLinearColor Color);
    void Text(const FString& Value, FLinearColor Color, float DX, float DY, float DesignPixels);
    void TextRight(const FString& Value, FLinearColor Color, float DRight, float DY, float DesignPixels);
    float Width(const FString& Value, float DesignPixels) const;
    /**
     * Clickable row. Id names the hit box and is routed in NotifyHitBoxClick; a disabled row is
     * drawn but registers no hit box, so it cannot be clicked or hovered.
     */
    void MenuRow(FName Id, const FString& Label, float DY, bool bEnabled, bool bPrimary, const FString& Note);
    void Button(FName Id, float DX, float DY, float DW, float DH, const FString& Label, bool bEnabled, bool bPrimary);
    /** Amber tag marking chrome with no system behind it yet. */
    void PlaceholderTag(float DX, float DY);
    void DrawTitleMark(float DX, float DY, float DSize, FLinearColor Color);
};
