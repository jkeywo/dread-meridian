#include "DMShellHUD.h"
#include "DMShellGameMode.h"
#include "DMShellPlayerController.h"
#include "DMCombatGameMode.h"
#include "DMGameState.h"
#include "DMHudStyle.h"
#include "DMInvestigatorComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/Font.h"
#include "UObject/ConstructorHelpers.h"
#include "CanvasItem.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
const TCHAR* InvestigatorTitle(EDMInvestigator Kind)
{
    switch (Kind)
    {
    case EDMInvestigator::Sapper: return TEXT("The Sapper");
    case EDMInvestigator::Photographer: return TEXT("The Photographer");
    case EDMInvestigator::Medium: return TEXT("The Medium");
    case EDMInvestigator::Smuggler: return TEXT("The Smuggler");
    default: return TEXT("Unassigned");
    }
}

FLinearColor InvestigatorColor(EDMInvestigator Kind)
{
    return UDMInvestigatorComponent::ColorFor(Kind);
}
}

ADMShellHUD::ADMShellHUD()
{
    // Roboto is a runtime (Slate) font, so it stays sharp at headline sizes. If it is ever
    // missing, DrawText falls back to the engine's default bitmap font.
    static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Engine/EngineFonts/Roboto"));
    ScreenFont = Font.Object;
}

FSlateFontInfo ADMShellHUD::FontAt(float DesignPixels) const
{
    // Roboto's em box is taller than its caps, so ask for a point size a little under the
    // design height and the drawn line lands where the mockups put it.
    return FSlateFontInfo(ScreenFont, FMath::Max(1, FMath::RoundToInt(DesignPixels * UiScale * .78f)));
}

EDMShellPhase ADMShellHUD::CurrentPhase() const
{
    const ADMGameState* State = GetWorld() ? GetWorld()->GetGameState<ADMGameState>() : nullptr;
    return State ? State->ShellPhase : EDMShellPhase::MainMenu;
}

void ADMShellHUD::DrawHUD()
{
    const EDMShellPhase Phase = CurrentPhase();
    if (Phase == EDMShellPhase::Mission)
    {
        Super::DrawHUD();
        return;
    }
    if (!Canvas) { return; }
    LayOutDesignSpace();
    // Opaque ground: there is no world worth showing behind a front-end screen.
    DrawRect(DMShell::Page, 0, 0, Canvas->ClipX, Canvas->ClipY);
    switch (Phase)
    {
    case EDMShellPhase::MainMenu: DrawMainMenu(); break;
    case EDMShellPhase::Lobby: DrawLobby(); break;
    case EDMShellPhase::Loading: DrawLoading(); break;
    case EDMShellPhase::CaseReport: DrawCaseReport(); break;
    default: break;
    }
}

void ADMShellHUD::LayOutDesignSpace()
{
    UiScale = FMath::Min(Canvas->ClipX / DesignWidth, Canvas->ClipY / DesignHeight);
    UiOriginX = (Canvas->ClipX - DesignWidth * UiScale) * .5f;
    UiOriginY = (Canvas->ClipY - DesignHeight * UiScale) * .5f;
}

// ---------------------------------------------------------------- drawing helpers

void ADMShellHUD::Fill(float DX, float DY, float DW, float DH, FLinearColor Color)
{
    DrawRect(Color, X(DX), Y(DY), DW * UiScale, DH * UiScale);
}

void ADMShellHUD::Frame(float DX, float DY, float DW, float DH, FLinearColor Border)
{
    const float L = X(DX), T = Y(DY), R = X(DX + DW), B = Y(DY + DH);
    DrawLine(L, T, R, T, Border, 1.f);
    DrawLine(L, B, R, B, Border, 1.f);
    DrawLine(L, T, L, B, Border, 1.f);
    DrawLine(R, T, R, B, Border, 1.f);
}

void ADMShellHUD::Brackets(float DX, float DY, float DW, float DH, FLinearColor Color)
{
    const float Arm = 14.f * UiScale;
    const float L = X(DX), T = Y(DY), R = X(DX + DW), B = Y(DY + DH);
    DrawLine(L, T, L + Arm, T, Color, 2.f);
    DrawLine(L, T, L, T + Arm, Color, 2.f);
    DrawLine(R, B, R - Arm, B, Color, 2.f);
    DrawLine(R, B, R, B - Arm, Color, 2.f);
}

void ADMShellHUD::Text(const FString& Value, FLinearColor Color, float DX, float DY, float DesignPixels)
{
    FCanvasTextItem Item(FVector2D(X(DX), Y(DY)), FText::FromString(Value), FontAt(DesignPixels), Color);
    Item.EnableShadow(FLinearColor::Transparent);
    Canvas->DrawItem(Item);
}

float ADMShellHUD::Width(const FString& Value, float DesignPixels) const
{
    const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    return Measure->Measure(Value, FontAt(DesignPixels)).X;
}

void ADMShellHUD::TextRight(const FString& Value, FLinearColor Color, float DRight, float DY, float DesignPixels)
{
    FCanvasTextItem Item(FVector2D(X(DRight) - Width(Value, DesignPixels), Y(DY)), FText::FromString(Value), FontAt(DesignPixels), Color);
    Item.EnableShadow(FLinearColor::Transparent);
    Canvas->DrawItem(Item);
}

void ADMShellHUD::PlaceholderTag(float DX, float DY)
{
    const FString Tag = TEXT("PLACEHOLDER");
    const float W = Width(Tag, 11.f) / UiScale + 12.f;
    Frame(DX, DY, W, 18, DMShell::Danger);
    Text(Tag, DMShell::Danger, DX + 6, DY + 4, 11);
}

void ADMShellHUD::Button(FName Id, float DX, float DY, float DW, float DH, const FString& Label, bool bEnabled, bool bPrimary)
{
    const bool bHot = bEnabled && Hovered == Id;
    if (bPrimary) { Fill(DX, DY, DW, DH, bEnabled ? (bHot ? DMShell::Bone : DMShell::Brass) : DMShell::Disabled); }
    else { Frame(DX, DY, DW, DH, bEnabled ? (bHot ? DMShell::Bone : DMShell::Rule) : DMShell::Disabled); }
    const FLinearColor Ink = bPrimary
        ? DMShell::Page
        : (bEnabled ? (bHot ? DMShell::Bone : DMShell::Brass) : DMShell::Disabled);
    const float Size = 20.f;
    Text(Label, Ink, DX + (DW - Width(Label, Size) / UiScale) * .5f, DY + (DH - Size) * .5f, Size);
    if (bEnabled)
    { AddHitBox(FVector2D(X(DX), Y(DY)), FVector2D(DW * UiScale, DH * UiScale), Id, true, 1); }
}

void ADMShellHUD::MenuRow(FName Id, const FString& Label, float DY, bool bEnabled, bool bPrimary, const FString& Note)
{
    constexpr float RowX = 120.f, RowW = 420.f, RowH = 64.f;
    const bool bHot = bEnabled && Hovered == Id;
    if (bPrimary) { Fill(RowX, DY, RowW, RowH, DMShell::Panel); }
    // Left rule: brass on the primary entry, bone on hover, dead grey when unavailable.
    const FLinearColor Rule = !bEnabled ? DMShell::Disabled : (bHot ? DMShell::Bone : (bPrimary ? DMShell::Brass : DMShell::Rule));
    Fill(RowX, DY, 3, RowH, Rule);
    const FLinearColor Ink = !bEnabled ? DMShell::Disabled : (bHot ? DMShell::Bone : (bPrimary ? DMShell::Brass : DMShell::Muted));
    Text(Label.ToUpper(), Ink, RowX + 26, DY + 20, 30);
    if (!Note.IsEmpty())
    { TextRight(Note, DMShell::Disabled, RowX + RowW - 20, DY + 26, 13); }
    if (bEnabled)
    { AddHitBox(FVector2D(X(RowX), Y(DY)), FVector2D(RowW * UiScale, RowH * UiScale), Id, true, 1); }
}

void ADMShellHUD::DrawTitleMark(float DX, float DY, float DSize, FLinearColor Color)
{
    const float CX = X(DX + DSize * .5f), CY = Y(DY + DSize * .5f);
    const float R = DSize * .5f * UiScale;
    constexpr int32 Segments = 48;
    for (int32 Ring = 0; Ring < 3; ++Ring)
    {
        const float Radius = R * (1.f - Ring * .27f);
        for (int32 Step = 0; Step < Segments; ++Step)
        {
            // The middle ring is dashed, echoing the ritual mark in the mockups.
            if (Ring == 1 && (Step % 3) != 0) { continue; }
            const float A0 = Step * 2.f * PI / Segments;
            const float A1 = (Step + 1) * 2.f * PI / Segments;
            DrawLine(CX + FMath::Cos(A0) * Radius, CY + FMath::Sin(A0) * Radius,
                     CX + FMath::Cos(A1) * Radius, CY + FMath::Sin(A1) * Radius, Color, 1.f);
        }
    }
    DrawLine(CX, CY - R, CX, CY + R, Color, 1.f);
    DrawLine(CX - R, CY, CX + R, CY, Color, 1.f);
}

// ---------------------------------------------------------------- screens

void ADMShellHUD::DrawMainMenu()
{
    DrawTitleMark(760, 120, 620, FLinearColor(DMShell::Brass.R, DMShell::Brass.G, DMShell::Brass.B, .16f));

    Text(TEXT("FOUR INVESTIGATORS - ONE RITUAL"), DMShell::Brass, 120, 150, 14);
    Text(TEXT("DREAD"), DMShell::Bone, 120, 178, 96);
    Text(TEXT("MERIDIAN"), DMShell::Brass, 120, 268, 96);

    MenuRow(TEXT("shell.start"), TEXT("Start"), 470, true, true, FString());
    MenuRow(TEXT("shell.settings"), TEXT("Settings"), 540, false, false, TEXT("NOT YET AVAILABLE"));
    MenuRow(TEXT("shell.quit"), TEXT("Exit to desktop"), 610, true, false, FString());

    // Roster strip: the four investigator portraits the encounter will actually spawn.
    constexpr float StripX = 700.f, StripY = 430.f, Size = 150.f, Gap = 16.f;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const EDMInvestigator Kind = static_cast<EDMInvestigator>(Index + 1);
        const float Column = StripX + Index * (Size + Gap);
        Fill(Column, StripY, Size, Size, DMShell::Panel);
        if (PlayerPortraits.IsValidIndex(static_cast<int32>(Kind)) && PlayerPortraits[static_cast<int32>(Kind)])
        {
            UTexture2D* Portrait = PlayerPortraits[static_cast<int32>(Kind)];
            DrawTexture(Portrait, X(Column), Y(StripY), Size * UiScale, Size * UiScale, 0, 0, 1, 1);
        }
        Fill(Column, StripY + Size - 3, Size, 3, InvestigatorColor(Kind));
        Text(FString(InvestigatorTitle(Kind)).ToUpper(), DMShell::Muted, Column, StripY + Size + 10, 13);
    }

    DrawLine(X(0), Y(812), X(DesignWidth), Y(812), DMShell::Rule, 1.f);
    Text(TEXT("CLICK TO SELECT"), DMShell::Muted, 120, 840, 13);
    TextRight(TEXT("BUILD [PLACEHOLDER] - UNREAL 5.8.2"), DMShell::Muted, 1320, 840, 13);
}

void ADMShellHUD::DrawLobby()
{
    // Header
    Text(TEXT("DREAD MERIDIAN"), DMShell::Muted, 40, 24, 16);
    Text(TEXT("EXPEDITION LOBBY"), DMShell::Bone, 620, 20, 24);
    TextRight(TEXT("INVITE CODE  [PLACEHOLDER]"), DMShell::Muted, 1400, 26, 13);
    DrawLine(X(0), Y(64), X(DesignWidth), Y(64), DMShell::Rule, 1.f);

    // Scenario
    Fill(40, 92, 860, 158, DMShell::Panel);
    Brackets(40, 92, 860, 158, DMShell::Bone);
    Text(TEXT("SCENARIO"), DMShell::Brass, 62, 110, 12);
    Text(TEXT("SWAMP FISHING VILLAGE"), DMShell::Bone, 62, 132, 34);
    Text(TEXT("FIXED SCENARIO"), DMShell::Brass, 62, 178, 12);
    Text(TEXT("Complete the core chain and three Disruptions to summon Shub."), DMShell::Body, 62, 202, 15);
    Text(TEXT("Lighthouse, treatment and relic sites are optional."), DMShell::Body, 62, 221, 15);
    Frame(700, 110, 180, 114, DMShell::Rule);
    Text(TEXT("ELDER ONE"), DMShell::Brass, 718, 126, 12);
    Text(TEXT("Hidden until"), DMShell::Muted, 718, 150, 15);
    Text(TEXT("it manifests"), DMShell::Muted, 718, 170, 15);
    PlaceholderTag(718, 194);

    // Party. The encounter always fields the four investigators; seats are not yet assignable.
    Text(TEXT("EXPEDITION - FOUR SEATS"), DMShell::Brass, 40, 262, 12);
    TextRight(TEXT("PREFERENCES, INVITES AND READINESS ARE NOT IMPLEMENTED"), DMShell::Muted, 900, 264, 12);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const EDMInvestigator Kind = static_cast<EDMInvestigator>(Index + 1);
        const float RowY = 286.f + Index * 92.f;
        Fill(40, RowY, 860, 84, DMShell::Panel);
        Fill(40, RowY, 3, 84, InvestigatorColor(Kind));
        if (PlayerPortraits.IsValidIndex(static_cast<int32>(Kind)) && PlayerPortraits[static_cast<int32>(Kind)])
        {
            DrawTexture(PlayerPortraits[static_cast<int32>(Kind)], X(56), Y(RowY + 6), 72 * UiScale, 72 * UiScale, 0, 0, 1, 1);
        }
        Text(FString(InvestigatorTitle(Kind)).ToUpper(), DMShell::Bone, 144, RowY + 20, 24);
        Text(Index == 0 ? TEXT("Local player") : TEXT("Companion"), DMShell::Muted, 144, RowY + 50, 15);
        TextRight(TEXT("SEAT LOCKED"), DMShell::Disabled, 880, RowY + 34, 13);
    }

    // Leads
    Fill(920, 92, 480, 562, DMShell::Panel);
    Brackets(920, 92, 480, 562, DMShell::Bone);
    Text(TEXT("YOUR LEADS"), DMShell::Brass, 942, 110, 12);
    Text(TEXT("WORTH PURSUING HERE"), DMShell::Bone, 942, 130, 26);
    PlaceholderTag(942, 168);
    Text(TEXT("The investigation archive, Lead tracking and the"), DMShell::Muted, 942, 196, 15);
    Text(TEXT("briefing shortlist are not implemented. Nothing"), DMShell::Muted, 942, 216, 15);
    Text(TEXT("here is recorded when the run ends."), DMShell::Muted, 942, 236, 15);

    // Bottom bar
    DrawLine(X(0), Y(812), X(DesignWidth), Y(812), DMShell::Rule, 1.f);
    Button(TEXT("shell.back"), 40, 830, 180, 52, TEXT("BACK"), true, false);
    Button(TEXT("shell.launch"), 1080, 830, 320, 52, TEXT("LAUNCH EXPEDITION"), true, true);
}

void ADMShellHUD::DrawLoading()
{
    Text(TEXT("STREAMING THE MISSION"), DMShell::Brass, 120, 420, 34);
    Text(TEXT("Loading the combat sandbox into the shell level."), DMShell::Muted, 120, 470, 17);
}

void ADMShellHUD::DrawCaseReport()
{
    const ADMGameState* State = GetWorld() ? GetWorld()->GetGameState<ADMGameState>() : nullptr;
    const bool bVictory = State && State->bShellVictory;

    Text(TEXT("CASE REPORT - EXPEDITION"), DMShell::Brass, 40, 28, 12);
    Text(bVictory ? TEXT("VICTORY") : TEXT("DEFEAT"), bVictory ? DMShell::Brass : DMShell::Danger, 40, 52, 92);
    Text(bVictory
            ? TEXT("The squad defeated the manifested Elder One.")
            : TEXT("All four investigators went down. The expedition is lost."),
        DMShell::Muted, 40, 150, 17);
    DrawLine(X(40), Y(186), X(1400), Y(186), DMShell::Rule, 1.f);

    // What the encounter actually recorded. Metrics live on the authority only.
    Fill(40, 206, 660, 566, DMShell::Panel);
    Brackets(40, 206, 660, 566, DMShell::Bone);
    Text(TEXT("WHAT THIS RUN RECORDED"), DMShell::Brass, 62, 224, 12);
    if (const ADMCombatGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ADMCombatGameMode>() : nullptr)
    {
        const FDMCombatMetrics& Metrics = Mode->GetMetrics();
        // Label left, figure right: the screen font is proportional, so padded columns would not line up.
        const auto Line = [this](const TCHAR* Label, const FString& Value, float RowY)
        {
            Text(Label, DMShell::Body, 62, RowY, 17);
            TextRight(Value, DMShell::Bone, 660, RowY, 17);
        };
        const float Row = 256.f;
        Line(TEXT("Combat ticks"), FString::FromInt(Mode->GetCombatTick()), Row);
        Line(TEXT("Enemies defeated"), FString::FromInt(Metrics.EnemyKills), Row + 34);
        Line(TEXT("Investigators downed"), FString::FromInt(Metrics.InvestigatorDowns), Row + 68);
        Line(TEXT("Revives completed"), FString::FromInt(Metrics.Revives), Row + 102);
        Line(TEXT("Damage to enemies"), FString::Printf(TEXT("%.0f"), Metrics.DamageToEnemies), Row + 136);
        Line(TEXT("Damage to investigators"), FString::Printf(TEXT("%.0f"), Metrics.DamageToInvestigators), Row + 170);
        Line(TEXT("Pings raised"), FString::FromInt(Metrics.PingsCreated), Row + 204);
        Text(TEXT("Provisional sandbox tuning, not approved balance."), DMShell::Muted, 62, Row + 246, 14);
    }
    else
    {
        Text(TEXT("Encounter metrics are server-side and are not replicated to clients yet."), DMShell::Muted, 62, 256, 16);
        PlaceholderTag(62, 286);
    }

    // Everything a real case report owes the player, none of which exists yet.
    Fill(720, 206, 680, 566, DMShell::Panel);
    Brackets(720, 206, 680, 566, DMShell::Bone);
    Text(TEXT("CONSEQUENCES"), DMShell::Brass, 742, 224, 12);
    PlaceholderTag(742, 244);
    const TCHAR* Missing[] = {
        TEXT("Elder One faced"),
        TEXT("Objectives completed, failed or expired"),
        TEXT("Ritual stage and its consequences"),
        TEXT("Injuries, Crises and resonant events"),
        TEXT("Scenario flavour consequences"),
        TEXT("Lead completions and new evidence"),
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Missing); ++Index)
    {
        const float RowY = 282.f + Index * 34.f;
        Fill(742, RowY + 6, 8, 8, DMShell::Disabled);
        Text(Missing[Index], DMShell::Body, 760, RowY, 17);
    }
    Text(TEXT("No letter grade by design; these are consequences, and none are implemented."), DMShell::Muted, 742, 506, 14);

    DrawLine(X(0), Y(812), X(DesignWidth), Y(812), DMShell::Rule, 1.f);
    Button(TEXT("shell.dismiss"), 1060, 830, 340, 52, TEXT("RETURN TO MAIN MENU"), true, true);
    Text(TEXT("Returning reopens the shell level; the finished run is discarded."), DMShell::Muted, 40, 846, 15);
}

// ---------------------------------------------------------------- input

void ADMShellHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    ADMShellPlayerController* Player = Cast<ADMShellPlayerController>(PlayerOwner);
    if (!Player) { return; }
    if (BoxName == TEXT("shell.start")) { Player->ServerShellStart(); }
    else if (BoxName == TEXT("shell.launch")) { Player->ServerShellLaunch(); }
    else if (BoxName == TEXT("shell.back") || BoxName == TEXT("shell.dismiss")) { Player->ServerShellDismiss(); }
    else if (BoxName == TEXT("shell.quit")) { Player->ServerShellQuit(); }
}

void ADMShellHUD::NotifyHitBoxBeginCursorOver(FName BoxName)
{
    Super::NotifyHitBoxBeginCursorOver(BoxName);
    Hovered = BoxName;
}

void ADMShellHUD::NotifyHitBoxEndCursorOver(FName BoxName)
{
    Super::NotifyHitBoxEndCursorOver(BoxName);
    if (Hovered == BoxName) { Hovered = NAME_None; }
}
