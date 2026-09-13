#include "DMCombatHUD.h"
#include "DMCombatant.h"
#include "DMCombatPlayerController.h"
#include "DMEncounterLayout.h"
#include "DMGameState.h"
#include "DMHudStyle.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace DMHud
{
    // Palette lives in DMHudStyle.h so the shell screens draw from the same one.
    static const float CapsuleHalfHeight = 90;

    /** World point projected for a ping marker; keep in sync with PingAnchor in DMCombatPlayerController.cpp. */
    static FVector PingAnchor(EDMPingKind Kind, const FVector& Location)
    { return Location + FVector(0, 0, FDMPingBoard::NeedsTarget(Kind) ? 260.f : 30.f); }

    static FString Phase(EDMRunPhase Value)
    { return StaticEnum<EDMRunPhase>()->GetNameStringByValue(static_cast<int64>(Value)); }
    static FString Stage(EDMRitualStage Value)
    { return StaticEnum<EDMRitualStage>()->GetNameStringByValue(static_cast<int64>(Value)); }
}

ADMCombatHUD::ADMCombatHUD()
{
    auto LoadPortrait = [](const TCHAR* Name) -> UTexture2D*
    {
        const FString Path = FString::Printf(TEXT("/Game/DreadMeridian/UI/Portraits/T_Portrait_%s"), Name);
        ConstructorHelpers::FObjectFinder<UTexture2D> Asset(*Path);
        return Asset.Object;
    };
    PlayerPortraits.SetNum(static_cast<int32>(EDMInvestigator::Smuggler) + 1);
    PlayerPortraits[static_cast<int32>(EDMInvestigator::Sapper)] = LoadPortrait(TEXT("Sapper"));
    PlayerPortraits[static_cast<int32>(EDMInvestigator::Photographer)] = LoadPortrait(TEXT("Photographer"));
    PlayerPortraits[static_cast<int32>(EDMInvestigator::Medium)] = LoadPortrait(TEXT("Medium"));
    PlayerPortraits[static_cast<int32>(EDMInvestigator::Smuggler)] = LoadPortrait(TEXT("Smuggler"));
    EnemyPortraits.SetNum(static_cast<int32>(EDMSmuggler::GangBoss) + 1);
    EnemyPortraits[static_cast<int32>(EDMSmuggler::Gunman)] = LoadPortrait(TEXT("Gunman"));
    EnemyPortraits[static_cast<int32>(EDMSmuggler::Bruiser)] = LoadPortrait(TEXT("Bruiser"));
    EnemyPortraits[static_cast<int32>(EDMSmuggler::Lookout)] = LoadPortrait(TEXT("Lookout"));
    EnemyPortraits[static_cast<int32>(EDMSmuggler::Bomber)] = LoadPortrait(TEXT("Bomber"));
    EnemyPortraits[static_cast<int32>(EDMSmuggler::GangBoss)] = LoadPortrait(TEXT("GangBoss"));
    PortraitBackground = LoadPortrait(TEXT("Background"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> Background(TEXT("/Game/DreadMeridian/UI/Portraits/M_Portrait_EnemyBackground"));
    EnemyPortraitBackground = Background.Object;
}

void ADMCombatHUD::Portrait(const FDMHudUnit& Unit, float X, float Y, float Size)
{
    Panel(X - S, Y - S, Size + 2*S, Size + 2*S, Unit.bEnemy ? DMHud::Danger : DMHud::Ally);
    if (Unit.bEnemy && EnemyPortraitBackground)
    { DrawMaterialSimple(EnemyPortraitBackground, X, Y, Size, Size); }
    else if (PortraitBackground)
    { DrawTexture(PortraitBackground, X, Y, Size, Size, 0, 0, 1, 1); }

    const ADMCombatant* Actor = Unit.Actor.Get();
    const int32 Index = Unit.bEnemy
        ? (Actor && Actor->Smuggler ? static_cast<int32>(Actor->Smuggler->Role) : 0)
        : static_cast<int32>(Unit.Kind);
    const auto& Portraits = Unit.bEnemy ? EnemyPortraits : PlayerPortraits;
    if (Portraits.IsValidIndex(Index) && Portraits[Index])
    {
        UTexture2D* Texture = Portraits[Index];
        const float Aspect = float(Texture->GetSizeX()) / FMath::Max(1, Texture->GetSizeY());
        const float U = Aspect > 1 ? 1.f / Aspect : 1.f;
        const float V = Aspect < 1 ? Aspect : 1.f;
        DrawTexture(Texture, X, Y, Size, Size, (1-U)*.5f, (1-V)*.5f, U, V);
    }
    if (Unit.bDown) { DrawRect(FLinearColor(0, 0, 0, .55f), X, Y, Size, Size); }
}

void ADMCombatHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) { return; }
    S = FMath::Clamp(Canvas->ClipY / 900.f, .7f, 2.f);

    const ADMCombatPlayerController* Player = Cast<ADMCombatPlayerController>(GetOwningPlayerController());
    const ADMCombatant* Pawn = Player ? Cast<ADMCombatant>(Player->GetPawn() ? Player->GetPawn() : Player->GetViewTarget()) : nullptr;
    const FDMHudModel Model = FDMHudModel::Build(GetWorld(), Pawn, Player ? Player->GetSelectedTarget() : nullptr);

    DrawWorldLayer(Model);
    DrawParty(Model);
    DrawRitual(Model);
    DrawTarget(Model);
    DrawEncounter(Model);
    DrawCondition(Model);
    DrawInvestigator(Model);
    DrawMinimap(Model);
    DrawControls();
    DrawPrimaryFeedback();
    if (Player) { DrawPingRadial(*Player); }
}

// ---------------------------------------------------------------- world layer

bool ADMCombatHUD::ProjectPoint(const FVector& Location, FVector2D& Out) const
{
    const FVector Screen = Project(Location);
    if (Screen.Z <= 0) { return false; }
    Out = FVector2D(Screen.X, Screen.Y);
    return true;
}

void ADMCombatHUD::DrawRing(const FVector& Feet, float Radius, FLinearColor Color, bool bDashed)
{
    constexpr int32 Segments = 28;
    FVector2D Previous = FVector2D::ZeroVector;
    bool bHavePrevious = false;
    for (int32 Index = 0; Index <= Segments; ++Index)
    {
        const float Angle = Index * UE_TWO_PI / Segments;
        FVector2D Point = FVector2D::ZeroVector;
        const bool bOk = ProjectPoint(Feet + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0), Point);
        if (bOk && bHavePrevious && (!bDashed || Index % 2 == 0))
        { DrawLine(Previous.X, Previous.Y, Point.X, Point.Y, Color, 2.f * S); }
        Previous = Point;
        bHavePrevious = bOk;
    }
}

void ADMCombatHUD::DrawSegment(const FVector& From, const FVector& To, FLinearColor Color)
{
    FVector2D A, B;
    if (ProjectPoint(From, A) && ProjectPoint(To, B)) { DrawLine(A.X, A.Y, B.X, B.Y, Color, 2.f * S); }
}

void ADMCombatHUD::DrawCone(const FVector& Origin, const FVector& Dir, float HalfAngleDeg, float Length, FLinearColor Color)
{
    if (Dir.IsNearlyZero()) { return; }
    const float Base = FMath::Atan2(Dir.Y, Dir.X);
    const float Half = FMath::DegreesToRadians(HalfAngleDeg);
    constexpr int32 Segments = 16;
    FVector2D Previous = FVector2D::ZeroVector;
    bool bHavePrevious = false;
    for (int32 Index = 0; Index <= Segments; ++Index)
    {
        const float Angle = Base - Half + 2 * Half * Index / Segments;
        FVector2D Point;
        const bool bOk = ProjectPoint(Origin + FVector(FMath::Cos(Angle) * Length, FMath::Sin(Angle) * Length, 0), Point);
        if (bOk && bHavePrevious && Index % 2 == 0) { DrawLine(Previous.X, Previous.Y, Point.X, Point.Y, Color, 2.f * S); }
        Previous = Point; bHavePrevious = bOk;
    }
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const float Angle = Base + (Side ? Half : -Half);
        DrawSegment(Origin, Origin + FVector(FMath::Cos(Angle) * Length, FMath::Sin(Angle) * Length, 0), Color);
    }
}

void ADMCombatHUD::DrawUnitWorld(const FDMHudUnit& Unit, bool bSelected)
{
    const FVector Feet = Unit.Location - FVector(0, 0, DMHud::CapsuleHalfHeight);
    // A dead enemy is inert scenery, not a unit to track - drop its floor ring too.
    if (!(Unit.bEnemy && Unit.bDown))
    {
        const FLinearColor Ring = Unit.bLocal ? DMHud::Brass : (Unit.bEnemy ? DMHud::Danger : (Unit.bDown ? DMHud::Bone : DMHud::Ally));
        DrawRing(Feet, Unit.bLocal ? 62.f : 55.f, Ring, Unit.bEnemy || Unit.bDown);
    }

    FVector2D Head;
    if (!ProjectPoint(Unit.Location + FVector(0, 0, 205), Head)) { return; }

    const float Width = (Unit.bLocal ? 88.f : 76.f) * S;
    const float Height = 7.f * S;
    const float X = Head.X - Width * .5f;
    const float Y = Head.Y;

    if (Unit.bDown && !Unit.bEnemy)
    {
        Label(TEXT("DOWN"), DMHud::Bone, X, Y - 15 * S, .95f);
        if (Unit.IsReviving())
        {
            Bar(X, Y, Width, Height, Unit.ReviveProgress, DMHud::Bone);
            Label(FString::Printf(TEXT("revive %s"), *Unit.ReviverId), DMHud::Bone, X, Y + Height + 2 * S, .8f);
        }
        else { Bar(X, Y, Width, Height, 0, DMHud::Bone); }
    }
    else
    {
        Bar(X, Y, Width, Height, Unit.HealthFraction(), Unit.bEnemy ? DMHud::Danger : DMHud::Health, Unit.ShieldFraction());
    }

    // Immediate aggro cue: this enemy is currently attacking the local investigator.
    if (Unit.bEnemy && Unit.bTargetingLocal && !Unit.bDown)
    {
        const float Mid = Head.X;
        const float Tip = Y - 6 * S;
        DrawLine(Mid - 5 * S, Tip, Mid, Tip - 6 * S, DMHud::Brass, 2.f * S);
        DrawLine(Mid + 5 * S, Tip, Mid, Tip - 6 * S, DMHud::Brass, 2.f * S);
    }

    if (bSelected)
    {
        const float Bracket = 8 * S;
        const float Left = X - 5 * S, Right = X + Width + 5 * S;
        const float Top = Y - 5 * S, Bottom = Y + Height + 5 * S;
        DrawLine(Left, Top + Bracket, Left, Top, DMHud::Bone, 2.f * S);
        DrawLine(Left, Top, Left + Bracket, Top, DMHud::Bone, 2.f * S);
        DrawLine(Right, Top + Bracket, Right, Top, DMHud::Bone, 2.f * S);
        DrawLine(Right, Top, Right - Bracket, Top, DMHud::Bone, 2.f * S);
        DrawLine(Left, Bottom - Bracket, Left, Bottom, DMHud::Bone, 2.f * S);
        DrawLine(Left, Bottom, Left + Bracket, Bottom, DMHud::Bone, 2.f * S);
        DrawLine(Right, Bottom - Bracket, Right, Bottom, DMHud::Bone, 2.f * S);
        DrawLine(Right, Bottom, Right - Bracket, Bottom, DMHud::Bone, 2.f * S);
    }
}

void ADMCombatHUD::DrawWorldLayer(const FDMHudModel& Model)
{
    const FString TargetId = Model.bHasTarget ? Model.Target.EntityId : FString();
    for (const FDMHudUnit& Unit : Model.Allies) { DrawUnitWorld(Unit, Unit.EntityId == TargetId); }
    for (const FDMHudUnit& Unit : Model.Enemies) { DrawUnitWorld(Unit, Unit.EntityId == TargetId); }
    if (Model.bHasSelf) { DrawUnitWorld(Model.Self, false); }
    for (const FDMHudPing& Ping : Model.Pings) { DrawPing(Ping); }
}

void ADMCombatHUD::Diamond(float X, float Y, float R, FLinearColor Color, bool bFilled)
{
    if (bFilled)
    {
        const float Step = FMath::Max(1.f, S);
        for (float Dy = -R; Dy < R; Dy += Step)
        {
            const float Half = R - FMath::Abs(Dy);
            if (Half > 0) { DrawRect(Color, X - Half, Y + Dy, Half * 2, Step); }
        }
        return;
    }
    DrawLine(X, Y - R, X + R, Y, Color, 1.5f * S);
    DrawLine(X + R, Y, X, Y + R, Color, 1.5f * S);
    DrawLine(X, Y + R, X - R, Y, Color, 1.5f * S);
    DrawLine(X - R, Y, X, Y - R, Color, 1.5f * S);
}

void ADMCombatHUD::Arc(float X, float Y, float Radius, float From, float To, FLinearColor Color, float Thickness)
{
    const int32 Segments = FMath::Max(2, FMath::CeilToInt(FMath::Abs(To - From) / (UE_PI / 24)));
    FVector2D Previous(X + FMath::Sin(From) * Radius, Y - FMath::Cos(From) * Radius);
    for (int32 Index = 1; Index <= Segments; ++Index)
    {
        const float Angle = From + (To - From) * Index / Segments;
        const FVector2D Point(X + FMath::Sin(Angle) * Radius, Y - FMath::Cos(Angle) * Radius);
        DrawLine(Previous.X, Previous.Y, Point.X, Point.Y, Color, Thickness);
        Previous = Point;
    }
}

void ADMCombatHUD::DrawPing(const FDMHudPing& Ping)
{
    // Subjective pings are "I perceive something here": a dashed ring on the ground, never a target.
    if (Ping.bSubjective) { DrawRing(Ping.Location + FVector(0, 0, 5), 70.f, Ping.Tint, true); }
    FVector2D P;
    if (!ProjectPoint(DMHud::PingAnchor(Ping.Kind, Ping.Location), P)) { return; }

    const float R = (Ping.bMine ? 8.f : 6.f) * S;
    Diamond(P.X, P.Y, R, Ping.Tint, true);
    if (Ping.bMine) { Diamond(P.X, P.Y, R + 3 * S, DMHud::Brass, false); }
    Label(Ping.Label, Ping.Tint, P.X - TextWidth(Ping.Label, .85f) * .5f, P.Y - R - 17 * S, .85f);

    // Age bar shrinks toward expiry.
    const float W = 36 * S, H = 3 * S, BarY = P.Y + R + 4 * S;
    DrawRect(DMHud::Sunk, P.X - W * .5f, BarY, W, H);
    const float Left = W * (1 - FMath::Clamp(Ping.Age, 0.f, 1.f));
    if (Left > 0) { DrawRect(Ping.Tint, P.X - Left * .5f, BarY, Left, H); }

    // Responder pips: filled square = on it, hollow square = busy, tick = acknowledged by a human.
    const float Pip = 6 * S, Gap = 3 * S, PipY = BarY + H + 3 * S;
    const int32 Count = Ping.OnIt + Ping.Busy + Ping.Acknowledged;
    float PipX = P.X - (Count * Pip + FMath::Max(0, Count - 1) * Gap) * .5f;
    for (int32 Index = 0; Index < Ping.OnIt; ++Index, PipX += Pip + Gap) { DrawRect(DMHud::Ally, PipX, PipY, Pip, Pip); }
    for (int32 Index = 0; Index < Ping.Busy; ++Index, PipX += Pip + Gap)
    {
        DrawLine(PipX, PipY, PipX + Pip, PipY, DMHud::Muted, 1.f);
        DrawLine(PipX + Pip, PipY, PipX + Pip, PipY + Pip, DMHud::Muted, 1.f);
        DrawLine(PipX + Pip, PipY + Pip, PipX, PipY + Pip, DMHud::Muted, 1.f);
        DrawLine(PipX, PipY + Pip, PipX, PipY, DMHud::Muted, 1.f);
    }
    for (int32 Index = 0; Index < Ping.Acknowledged; ++Index, PipX += Pip + Gap)
    {
        DrawLine(PipX, PipY + Pip * .55f, PipX + Pip * .4f, PipY + Pip, DMHud::Bone, 1.5f * S);
        DrawLine(PipX + Pip * .4f, PipY + Pip, PipX + Pip, PipY, DMHud::Bone, 1.5f * S);
    }
    if (!Ping.bMine && !Ping.AuthorName.IsEmpty())
    { Label(Ping.AuthorName, DMHud::Muted, P.X - TextWidth(Ping.AuthorName, .7f) * .5f, PipY + (Count > 0 ? Pip + 2 * S : 0), .7f); }
}

void ADMCombatHUD::DrawPingRadial(const ADMCombatPlayerController& Player)
{
    if (!Player.IsPingRadialOpen()) { return; }
    const TArray<EDMPingKind>& Kinds = ADMCombatPlayerController::RadialKinds();
    const int32 Count = Kinds.Num();
    if (Count == 0) { return; }
    const FVector2D O = Player.GetPingRadialOrigin();
    const float Width = UE_TWO_PI / Count;
    const float Inner = ADMCombatPlayerController::PingRadialDeadZone, Outer = 92 * S;
    const int32 Hover = Player.GetPingRadialHover();

    Arc(O.X, O.Y, Inner, 0, UE_TWO_PI, DMHud::BrassDim, 1.5f * S);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const bool bHot = Index == Hover;
        const float Centre = Index * Width, From = Centre - Width * .5f, To = Centre + Width * .5f;
        Arc(O.X, O.Y, Outer, From + .02f, To - .02f, bHot ? DMHud::Brass : DMHud::BrassDim, (bHot ? 3.f : 1.5f) * S);
        DrawLine(O.X + FMath::Sin(From) * Inner, O.Y - FMath::Cos(From) * Inner,
            O.X + FMath::Sin(From) * Outer, O.Y - FMath::Cos(From) * Outer, DMHud::BrassDim, 1.f);
        const FString Text = FDMHudModel::PingLabel(Kinds[Index]);
        const float Scale = bHot ? 1.f : .85f;
        const float TW = TextWidth(Text, Scale);
        const FVector2D At(O.X + FMath::Sin(Centre) * Outer * .68f - TW * .5f, O.Y - FMath::Cos(Centre) * Outer * .68f - 7 * S);
        if (bHot) { DrawRect(DMHud::Ground, At.X - 4 * S, At.Y - 2 * S, TW + 8 * S, 18 * S); }
        Label(Text, bHot ? DMHud::Brass : DMHud::Bone, At.X, At.Y, Scale);
    }
    const FString Hint = Hover == INDEX_NONE ? TEXT("release: cancel") : TEXT("release: ping");
    Label(Hint, DMHud::Muted, O.X - TextWidth(Hint, .75f) * .5f, O.Y + Outer + 8 * S, .75f);
}

// ------------------------------------------------------------------- widgets

void ADMCombatHUD::Panel(float X, float Y, float W, float H, FLinearColor Border)
{
    DrawRect(DMHud::Ground, X, Y, W, H);
    DrawLine(X, Y, X + W, Y, Border, 1.f);
    DrawLine(X, Y + H, X + W, Y + H, Border, 1.f);
    DrawLine(X, Y, X, Y + H, Border, 1.f);
    DrawLine(X + W, Y, X + W, Y + H, Border, 1.f);
}

void ADMCombatHUD::Bar(float X, float Y, float W, float H, float Fraction, FLinearColor Fill, float ShieldFraction)
{
    DrawRect(DMHud::Sunk, X, Y, W, H);
    const float Filled = W * FMath::Clamp(Fraction, 0.f, 1.f);
    if (Filled > 0) { DrawRect(Fill, X, Y, Filled, H); }
    if (ShieldFraction > 0)
    {
        // Shield reads as a distinct hatched segment, never as colour alone.
        const float ShieldWidth = FMath::Min(W - Filled, W * ShieldFraction);
        for (float Offset = 0; Offset < ShieldWidth; Offset += 4 * S)
        { DrawRect(DMHud::Shield, X + Filled + Offset, Y, FMath::Min(2.f * S, ShieldWidth - Offset), H); }
    }
    for (float Tick = W * .25f; Tick < W - 1; Tick += W * .25f)
    { DrawLine(X + Tick, Y, X + Tick, Y + H, FLinearColor(0, 0, 0, .75f), 1.f); }
}

void ADMCombatHUD::Label(const FString& Text, FLinearColor Color, float X, float Y, float Scale)
{
    DrawText(Text, Color, X, Y, nullptr, Scale * S);
}

float ADMCombatHUD::TextWidth(const FString& Text, float Scale) const
{
    float W = 0, H = 0;
    const_cast<ADMCombatHUD*>(this)->GetTextSize(Text, W, H, nullptr, Scale * S);
    return W;
}

void ADMCombatHUD::Slot(float X, float Y, float Size, const FString& Key, const FString& Caption, FLinearColor Border, float Cooldown)
{
    Panel(X, Y, Size, Size, Border);
    Label(Key, DMHud::Bone, X + Size - 11 * S, Y + Size - 15 * S, 1.f);
    if (!Caption.IsEmpty()) { Label(Caption, DMHud::Gap, X + 4 * S, Y + Size * .42f, .85f); }
    if (Cooldown > 0)
    {
        DrawRect(FLinearColor(0, 0, 0, .7f), X, Y, Size, Size * Cooldown);
    }
}

// -------------------------------------------------------------- screen layer

void ADMCombatHUD::DrawParty(const FDMHudModel& Model)
{
    const float X = 20 * S;
    float Y = 16 * S;
    const float W = 272 * S;
    Label(TEXT("PARTY"), DMHud::Muted, X, Y, .9f);
    Y += 18 * S;
    for (const FDMHudUnit& Unit : Model.Allies)
    {
        const float H = 58 * S;
        Panel(X, Y, W, H, Unit.bDown ? FLinearColor(DMHud::Bone.R, DMHud::Bone.G, DMHud::Bone.B, .5f) : DMHud::BrassDim);
        Portrait(Unit, X + 7 * S, Y + 7 * S, 44 * S);
        Label(Unit.Name, Unit.bDown ? DMHud::Muted : DMHud::Bone, X + 60 * S, Y + 6 * S, FMath::Min(1.f, (W - 68*S) / FMath::Max(1.f, TextWidth(Unit.Name, 1.f))));
        const FString Control = FString::Printf(TEXT("[%s]"), *Unit.Control);
        if (!Unit.bDown) { Label(Control, DMHud::Muted, X + W - TextWidth(Control, .75f) - 8 * S, Y + 38 * S, .75f); }
        if (Unit.bDown)
        {
            Bar(X + 60 * S, Y + 24 * S, W - 68 * S, 10 * S, Unit.ReviveProgress, DMHud::Bone);
            Label(Unit.IsReviving()
                    ? FString::Printf(TEXT("DOWN - revive %.0f%% by %s"), Unit.ReviveProgress * 100, *Unit.ReviverId)
                    : FString(TEXT("DOWN - no one channelling")),
                DMHud::Bone, X + 60 * S, Y + 38 * S, .85f);
        }
        else
        {
            Bar(X + 60 * S, Y + 24 * S, W - 68 * S, 10 * S, Unit.HealthFraction(), DMHud::Health, Unit.ShieldFraction());
            Label(FString::Printf(TEXT("%.0f/%.0f  INJ %d  GRV %d"), Unit.Health, Unit.MaxHealth, Unit.Injuries, Unit.Grievous),
                DMHud::Muted, X + 60 * S, Y + 38 * S, .75f);
        }
        Y += H + 6 * S;
    }
}

void ADMCombatHUD::DrawRitual(const FDMHudModel& Model)
{
    const float W = 476 * S, H = 62 * S;
    const float X = (Canvas->ClipX - W) * .5f, Y = 14 * S;
    Panel(X, Y, W, H, DMHud::BrassDim);
    Label(TEXT("RITUAL"), DMHud::Muted, X + 14 * S, Y + 8 * S, .9f);
    const FString Stage = DMHud::Stage(Model.RitualStage).ToUpper();
    Label(Stage, DMHud::Brass, X + (W - TextWidth(Stage, 1.2f)) * .5f, Y + 6 * S, 1.2f);
    const FString Phase = FString::Printf(TEXT("%s  %d"), *DMHud::Phase(Model.Phase), Model.RitualProgress);
    Label(Phase, DMHud::Muted, X + W - TextWidth(Phase, .9f) - 14 * S, Y + 8 * S, .9f);

    const int32 Current = static_cast<int32>(Model.RitualStage);
    const float Rail = W - 40 * S;
    const float Step = Rail / 4;
    const float NodeY = Y + 38 * S;
    DrawLine(X + 20 * S, NodeY, X + 20 * S + Rail, NodeY, DMHud::BrassDim, 2.f);
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const float NodeX = X + 20 * S + Step * Index;
        const float Size = (Index == Current ? 12.f : 8.f) * S;
        const FLinearColor Colour = Index <= Current ? DMHud::Brass : DMHud::BrassDim;
        DrawRect(Colour, NodeX - Size * .5f, NodeY - Size * .5f, Size, Size);
    }
}

void ADMCombatHUD::DrawTarget(const FDMHudModel& Model)
{
    if (!Model.bHasTarget) { return; }
    const FDMHudUnit& Unit = Model.Target;
    const float W = 390 * S, H = 82 * S;
    const float X = (Canvas->ClipX - W) * .5f, Y = 88 * S;
    Panel(X, Y, W, H, DMHud::BrassDim);
    Portrait(Unit, X + 8*S, Y + 8*S, 64*S);
    Label(Unit.Name, DMHud::Bone, X + 84 * S, Y + 7 * S, FMath::Min(1.1f, (W - 145*S) / FMath::Max(1.f, TextWidth(Unit.Name, 1.f))));
    const FString Tier = Unit.bEnemy ? TEXT("hostile") : TEXT("ally");
    Label(Tier, DMHud::Muted, X + W - TextWidth(Tier, .85f) - 12 * S, Y + 9 * S, .85f);
    Bar(X + 84 * S, Y + 28 * S, W - 96 * S, 12 * S, Unit.HealthFraction(), Unit.bEnemy ? DMHud::Danger : DMHud::Health, Unit.ShieldFraction());
    Label(FString::Printf(TEXT("%.0f/%.0f"), Unit.Health, Unit.MaxHealth), DMHud::Muted, X + 84 * S, Y + 43 * S, .85f);
    if (Unit.bTargetingLocal)
    { Label(TEXT("FIXED ON YOU"), DMHud::Brass, X + W - TextWidth(TEXT("FIXED ON YOU"), .85f) - 12 * S, Y + 43 * S, .85f); }
    if (Unit.bEnemy) { Label(Unit.Resources, DMHud::Brass, X + 84*S,Y + 61*S,FMath::Min(.8f, (W - 96*S) / FMath::Max(1.f, TextWidth(Unit.Resources, 1.f)))); }
    // Elites carry a visible Resolve layer; common enemies never do (GDD 4.4).
    if (Unit.bElite)
    {
        Label(Unit.bBroken ? TEXT("BROKEN") : Unit.bInterruptible ? TEXT("INTERRUPT") : Unit.bResisting ? TEXT("RESISTING") : TEXT("RESOLVE"), Unit.bBroken ? DMHud::Brass : DMHud::Muted, X + W - 96 * S, Y + 26 * S, .75f);
        Bar(X + W - 96 * S, Y + 38 * S, 84 * S, 8 * S, Unit.BreakFraction, Unit.bBroken ? DMHud::Brass : DMHud::Shield);
    }
    if (Unit.bSuppressed) { Label(TEXT("SUPPRESSED"), DMHud::Shield, X + 84 * S, Y + 43 * S, .8f); }
}

void ADMCombatHUD::DrawEncounter(const FDMHudModel& Model)
{
    const float W = 300 * S, H = 78 * S;
    const float X = Canvas->ClipX - W - 20 * S, Y = 16 * S;
    Panel(X, Y, W, H, DMHud::BrassDim);
    Label(Model.EncounterObjective.IsEmpty() ? TEXT("SANDBOX ENCOUNTER") : TEXT("SMUGGLER TERRITORY"), DMHud::Brass, X + 12 * S, Y + 8 * S, .9f);
    Label(FString::Printf(TEXT("Enemies standing  %d / %d"), Model.EnemiesStanding, Model.EnemyCount),
        DMHud::Bone, X + 12 * S, Y + 26 * S, 1.f);
    Label(FString::Printf(TEXT("Investigators up  %d / %d"), Model.InvestigatorsStanding, Model.InvestigatorCount),
        DMHud::Bone, X + 12 * S, Y + 42 * S, 1.f);
    Label(Model.EncounterObjective.IsEmpty() ? TEXT("No scenario objectives implemented") : Model.EncounterObjective, DMHud::Brass, X + 12 * S, Y + 60 * S, .85f);
}

void ADMCombatHUD::DrawCondition(const FDMHudModel& Model)
{
    if (!Model.bHasSelf) { return; }
    const float W = 244 * S, H = 180 * S;
    const float X = 118 * S, Y = Canvas->ClipY - H - 18 * S;
    Panel(X, Y, W, H, DMHud::BrassDim);
    Label(TEXT("CONDITION"), DMHud::Muted, X + 10 * S, Y + 7 * S, .9f);
    Label(FString::Printf(TEXT("Injuries %d"), Model.Self.Injuries), DMHud::Bone, X + 10 * S, Y + 26 * S, 1.f);
    Label(FString::Printf(TEXT("Grievous %d"), Model.Self.Grievous),
        Model.Self.Grievous > 0 ? DMHud::Danger : DMHud::Bone, X + 120 * S, Y + 26 * S, 1.f);
    if (!Model.Self.Symptom.IsEmpty()) { Label(Model.Self.Symptom, DMHud::Bone, X + 10 * S, Y + 150 * S, .55f); }
    if (const auto* P = Cast<ADMCombatPlayerController>(PlayerOwner))
    {
        const auto V = P->PrivateMadness();
        Label(StaticEnum<EDMMadnessFamily>()->GetNameStringByValue(static_cast<int64>(V.Family)), DMHud::Bone, X + 130*S, Y + 7*S, .7f);
        for (const auto& C : V.Cues)
        {
            FVector2D Screen;
            if (P->ProjectWorldLocationToScreen(C.Location + FVector(0,0,120), Screen))
            {
                DrawRect(FLinearColor(.65f,.35f,1,.8f), Screen.X-5*S, Screen.Y-5*S, 10*S, 10*S);
                Label(FString::Printf(TEXT("%s %d/%d"), *C.Label, C.Progress, C.Goal), DMHud::Bone, Screen.X+9*S, Screen.Y-8*S, .7f);
            }
        }
    }
    // Only the local owner receives this private state.
    Bar(X + 10 * S, Y + 48 * S, 110 * S, 8 * S, Model.Self.Madness / 100.f, DMHud::Gap);
    Label(FString::Printf(TEXT("Madness %.0f / floor %.0f | %s"), Model.Self.Madness, Model.Self.MadnessFloor, Model.Self.bCrisis ? TEXT("CRISIS") : Model.Self.bGrounding ? TEXT("Grounding") : TEXT("H: ground")), DMHud::Bone, X + 10 * S, Y + 58 * S, .65f);
    for (int32 I = 0; I < Model.Self.SpecificInjuries.Num(); ++I)
    {
        const auto K = Model.Self.SpecificInjuries[I];
        Label(DMInjuryRules::Name(K), DMHud::Danger, X + 10 * S, Y + (76 + I * 32) * S, .8f);
        Label(DMInjuryRules::Effect(K), DMHud::Bone, X + 10 * S, Y + (90 + I * 32) * S, .55f);
    }
}

void ADMCombatHUD::DrawInvestigator(const FDMHudModel& Model)
{
    if (!Model.bHasSelf) { return; }
    const FDMHudUnit& Self = Model.Self;
    const ADMCombatant* Actor = Self.Actor.Get();
    const float W = 560 * S, H = 132 * S;
    const float X = (Canvas->ClipX - W) * .5f, Y = Canvas->ClipY - H - 18 * S;
    Panel(X, Y, W, H, DMHud::Brass);
    Portrait(Self, X + 8 * S, Y + 8 * S, 64 * S);

    Label(Self.Name, DMHud::Bone, X + 84 * S, Y + 8 * S, 1.3f);
    Label(Self.bDown ? TEXT("DOWNED - an ally must channel the revive") : TEXT(""), DMHud::Danger, X + 84 * S, Y - 18 * S, .9f);

    Bar(X + 84 * S, Y + 30 * S, 260 * S, 18 * S, Self.HealthFraction(), DMHud::Health, Self.ShieldFraction());
    Label(FString::Printf(TEXT("%.0f / %.0f"), Self.Health, Self.MaxHealth), DMHud::Bone, X + 352 * S, Y + 31 * S, 1.1f);
    if (Self.Shield > 0)
    { Label(FString::Printf(TEXT("+%.0f shield"), Self.Shield), DMHud::Shield, X + 448 * S, Y + 32 * S, .9f); }

    // Bespoke resource, drawn in the shape the resource actually has.
    const float ResourceY = Y + 56 * S;
    if (Actor)
    {
        const UDMInvestigatorComponent* Kit = Actor->Investigator;
        switch (Self.Kind)
        {
        case EDMInvestigator::Sapper:
            Label(TEXT("CHARGES"), DMHud::Muted, X + 84 * S, ResourceY, .85f);
            for (int32 Index = 0; Index < UDMInvestigatorComponent::ChargeCapacity; ++Index)
            {
                const float PipX = X + 160 * S + Index * 20 * S;
                if (Index < Kit->Charges) { DrawRect(DMHud::Brass, PipX, ResourceY, 14 * S, 14 * S); }
                else { Panel(PipX, ResourceY, 14 * S, 14 * S, DMHud::BrassDim); }
            }
            Label(FString::Printf(TEXT("components %d/2"), Kit->Components), DMHud::Muted, X + 236 * S, ResourceY, .85f);
            break;
        case EDMInvestigator::Smuggler:
            Label(TEXT("MOMENTUM"), DMHud::Muted, X + 84 * S, ResourceY, .85f);
            Bar(X + 160 * S, ResourceY, 160 * S, 12 * S, Kit->Momentum / 100.f, DMHud::Brass);
            Label(FString::Printf(TEXT("%.0f  combo %d/3"), Kit->Momentum, Kit->Combo), DMHud::Muted, X + 328 * S, ResourceY, .85f);
            break;
        default:
            Label(Kit->ResourceSummary(Model.bHasTarget ? Model.Target.EntityId : FString()),
                DMHud::Muted, X + 84 * S, ResourceY, .9f);
            break;
        }
    }

    // Basic plus the four named abilities. Evolution nodes are not implemented, so no slot advertises one.
    const float SlotY = Y + 78 * S;
    const float Size = 44 * S;
    Slot(X + 24 * S, SlotY, Size, TEXT("LMB"), TEXT("basic"), DMHud::Brass, Model.BasicCooldown);
    if (Model.BasicCooldownSeconds > 0)
    { Label(FString::Printf(TEXT("%.1f"), Model.BasicCooldownSeconds), DMHud::Bone, X + 30 * S, SlotY + Size * .3f, 1.1f); }
    const auto* Player = Cast<ADMCombatPlayerController>(GetOwningPlayerController());
    const int32 AimingSlot = Player && Player->IsAiming() ? Player->GetAimSlot() : INDEX_NONE;
    for (int32 Index = 0; Index < Model.Abilities.Num(); ++Index)
    {
        const FDMHudAbility& Ability = Model.Abilities[Index];
        const float SlotX = X + (84 + Index * 52) * S;
        const FLinearColor Border = !Ability.bImplemented ? FLinearColor(.35f, .37f, .36f, .6f)
            : Index == AimingSlot ? DMHud::Bone : Ability.bActive ? DMHud::Ally : DMHud::Brass;
        // The caption is the ability's own short name, so the player reads the kit rather than a slot letter.
        FString Caption = Ability.bImplemented ? Ability.Name : TEXT("--");
        if (Caption.Len() > 10) { Caption = Caption.Left(9) + TEXT("."); }
        Slot(SlotX, SlotY, Size, Ability.Key, Caption, Border, Ability.Cooldown);
        if (Ability.CooldownSeconds > 0)
        { Label(FString::Printf(TEXT("%.1f"), Ability.CooldownSeconds), DMHud::Bone, SlotX + 6 * S, SlotY + Size * .3f, 1.f); }
        if (Ability.bActive) { Label(TEXT("ON"), DMHud::Ally, SlotX + 4 * S, SlotY + 2 * S, .7f); }
    }
    const int32 StatusSlot = AimingSlot != INDEX_NONE ? AimingSlot : 0;
    Label(Model.Abilities.IsValidIndex(StatusSlot) ? Model.Abilities[StatusSlot].Status : TEXT(""), DMHud::Brass, X + 300 * S, SlotY + 6 * S, .8f);
    Label(FString::Printf(TEXT("revive nearby ally  V     tick %d"), Model.CombatTick),
        DMHud::Muted, X + 300 * S, SlotY + 24 * S, .85f);
}

void ADMCombatHUD::DrawMinimap(const FDMHudModel& Model)
{
    // The map keeps the playable arena's aspect so distances read honestly.
    const float W = 190 * S, H = 230 * S;
    const float X = Canvas->ClipX - W - 20 * S, Y = Canvas->ClipY - H - 40 * S;
    Panel(X, Y, W, H, DMHud::BrassDim);

    auto Place = [&](const FVector& Location, float& OutX, float& OutY)
    {
        OutX = X + W * .5f + FMath::Clamp(Location.Y / DMEncounterLayout::PlayableY, -1.f, 1.f) * W * .46f;
        OutY = Y + H * .5f - FMath::Clamp(Location.X / DMEncounterLayout::PlayableX, -1.f, 1.f) * H * .46f;
    };
    float PointX = 0, PointY = 0;
    for (const FDMHudUnit& Unit : Model.Enemies)
    {
        if (Unit.bDown) { continue; }
        Place(Unit.Location, PointX, PointY);
        // Hostiles are triangles, objectives and allies are not: shape carries the distinction.
        DrawLine(PointX, PointY - 5 * S, PointX - 5 * S, PointY + 4 * S, DMHud::Danger, 2.f * S);
        DrawLine(PointX, PointY - 5 * S, PointX + 5 * S, PointY + 4 * S, DMHud::Danger, 2.f * S);
        DrawLine(PointX - 5 * S, PointY + 4 * S, PointX + 5 * S, PointY + 4 * S, DMHud::Danger, 2.f * S);
    }
    for (const FDMHudUnit& Unit : Model.Allies)
    {
        Place(Unit.Location, PointX, PointY);
        DrawRect(Unit.bDown ? DMHud::Bone : DMHud::Ally, PointX - 3 * S, PointY - 3 * S, 6 * S, 6 * S);
    }
    if (Model.bHasSelf)
    {
        Place(Model.Self.Location, PointX, PointY);
        DrawRect(DMHud::Brass, PointX - 4 * S, PointY - 4 * S, 8 * S, 8 * S);
        DrawLine(PointX, PointY - 12 * S, PointX - 4 * S, PointY - 6 * S, DMHud::Brass, 2.f * S);
        DrawLine(PointX, PointY - 12 * S, PointX + 4 * S, PointY - 6 * S, DMHud::Brass, 2.f * S);
    }
    Label(TEXT("no fog of war implemented"), DMHud::Gap, X, Y + H + 4 * S, .85f);
}

void ADMCombatHUD::DrawControls()
{
    if (const auto* Player = GetOwningPlayerController(); Player && !Player->GetPawn() && Cast<ADMCombatant>(Player->GetViewTarget()))
    {
        Label(TEXT("BOT CONTROLLED - following selected investigator"), DMHud::Brass, 20*S, Canvas->ClipY-170*S, .95f);
        return;
    }
    const float X = 20 * S;
    const float Y = Canvas->ClipY - 170 * S;
    Label(TEXT("DREAD MERIDIAN  |  combat sandbox"), DMHud::Brass, X, Y, 1.f);
    Label(TEXT("right-click move/attack   left stick move   Tab target"), DMHud::Muted, X, Y + 18 * S, .85f);
    Label(TEXT("Q / W / E / R abilities   LMB / Space attack   V revive ally"), DMHud::Muted, X, Y + 34 * S, .85f);
    Label(TEXT("G ping (hold: radial)"), DMHud::Muted, X, Y + 50 * S, .85f);
}

void ADMCombatHUD::DrawPrimaryFeedback()
{
    const auto* Player = Cast<ADMCombatPlayerController>(GetOwningPlayerController());
    const auto* Actor = Player ? Cast<ADMCombatant>(Player->GetPawn()) : nullptr;
    if (!Actor) { return; }
    const float X = Canvas->ClipX * .5f, Y = Canvas->ClipY - 190 * S;
    Label(TEXT("Q W E R aim | LMB / A confirm | RMB / B cancel | F / Y detonate"), DMHud::Muted, X - 220 * S, Y, .85f);
    if (Player->IsAiming())
    {
        ADMCombatant* Target; FVector Point; Player->GetAim(Target, Point);
        const FString Failure = Player->AimFailure(Target, Point);
        const FLinearColor Color = Failure.IsEmpty() ? DMHud::Ally : DMHud::Danger;
        const FVector Feet = Actor->GetActorLocation() - FVector(0, 0, 80);
        DrawRing(Feet, Player->AimRange(), DMHud::Brass, true);
        const int32 Slot = Player->GetAimSlot();
        const EDMInvestigator Kind = Actor->Investigator->Kind;
        const bool bCone = Slot == 1 && (Kind == EDMInvestigator::Sapper || Kind == EDMInvestigator::Photographer);
        const bool bWire = Slot == 2 && Kind == EDMInvestigator::Sapper;
        if (bCone)
        {
            const FVector Dir = (Point - Actor->GetActorLocation()).GetSafeNormal2D();
            const float Half = Kind == EDMInvestigator::Sapper ? 25.f : 35.f;
            const float Length = Kind == EDMInvestigator::Sapper ? 600.f : 350.f;
            DrawCone(Feet, Dir, Half, Length, Color);
        }
        else if (bWire && Actor->Kit->bWirePending) { DrawSegment(Actor->Kit->PendingWireStart + FVector(0, 0, 5), Point + FVector(0, 0, 5), Color); }
        else if (bWire) { DrawRing(Point + FVector(0, 0, 5), 45, Color, false); }
        else if (Slot == 0)
        {
            DrawRing((Target ? Target->GetActorLocation() - FVector(0, 0, 80) : Point) + FVector(0, 0, 5),
                Kind == EDMInvestigator::Sapper ? 220 : 45, Color, false);
        }
        else { DrawRing((Target ? Target->GetActorLocation() - FVector(0, 0, 80) : Point) + FVector(0, 0, 5), 180, Color, false); }
        Label(Failure.IsEmpty() ? TEXT("Confirm: ") + Player->AimName() : Failure, Color, X - 120 * S, Y - 22 * S, 1.f);
    }
    else { Label(Player->QFeedback(), DMHud::Brass, X - 120 * S, Y - 22 * S, 1.f); }
    for (const FDMHudUnit& Unit : FDMHudModel::Build(GetWorld(), Actor, nullptr).Enemies)
    {
        const auto* Enemy = Unit.Actor.Get();
        if (Enemy && Enemy->bTelegraphActive) { DrawRing(Unit.Location - FVector(0, 0, 80), 65, DMHud::Danger, false); }
        if (Enemy && Enemy->IsRestrained()) { DrawRing(Unit.Location - FVector(0, 0, 80), 45, DMHud::Ally, false); }
    }
}
