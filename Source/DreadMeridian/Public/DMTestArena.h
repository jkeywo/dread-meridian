#pragma once
#include "CoreMinimal.h"
#include "DMCombatGameMode.h"
#include "DMCombatPlayerController.h"
#include "DMCombatHUD.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DMTestArena.generated.h"

UENUM()
enum class EDMArenaEnemy : uint8 { Gunman, Bruiser, Lookout, Bomber, GangBoss, Crawler, Lurker, Spitter, Grasper, OldThing, Count };
UENUM()
enum class EDMArenaPhase : uint8 { Setup, Fighting, Paused, Victory, Defeat };

USTRUCT()
struct FDMArenaPlacement
{
    GENERATED_BODY()
    UPROPERTY() EDMArenaEnemy Type = EDMArenaEnemy::Gunman;
    UPROPERTY() FVector Position = FVector::ZeroVector;
    UPROPERTY() int32 Batch = 0;
};
USTRUCT()
struct DREADMERIDIAN_API FDMArenaConfig
{
    GENERATED_BODY()
    UPROPERTY() EDMInvestigator Player = EDMInvestigator::Sapper;
    UPROPERTY() TArray<EDMInvestigator> Companions;
    UPROPERTY() int32 Seed = 1927;
    UPROPERTY() TArray<FDMArenaPlacement> Placements;
    bool IsValid() const;
};

namespace DMArena
{
    constexpr int32 MaxEnemies = 32;
    DREADMERIDIAN_API FString EnemyName(EDMArenaEnemy Type);
    DREADMERIDIAN_API FString HeroName(EDMInvestigator Kind);
    DREADMERIDIAN_API TArray<EDMArenaEnemy> Preset(int32 Index);
    DREADMERIDIAN_API FString PresetName(int32 Index);
    DREADMERIDIAN_API FVector FormationOffset(int32 Index, int32 Count);
}

/** Session-local setup only; no gameplay state survives a reload. */
UCLASS()
class DREADMERIDIAN_API UDMArenaSession : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY() FDMArenaConfig Config;
    bool bRestore = false;
    int32 ProbeStage = 0;
};

UCLASS()
class DREADMERIDIAN_API ADMTestArenaGeometry : public AActor
{
    GENERATED_BODY()
public:
    ADMTestArenaGeometry();
    virtual void BeginPlay() override;
};

UCLASS()
class DREADMERIDIAN_API ADMTestArenaGameMode : public ADMCombatGameMode
{
    GENERATED_BODY()
public:
    ADMTestArenaGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool UsesEncounterLayout() const override { return false; }
    UPROPERTY() FDMArenaConfig Config;
    EDMArenaPhase Phase = EDMArenaPhase::Setup;
    FString Feedback;
    bool CanPlace() const { return Phase == EDMArenaPhase::Setup || Phase == EDMArenaPhase::Paused; }
    bool ValidatePlacement(const TArray<EDMArenaEnemy>& Types, FVector Center, TArray<FDMArenaPlacement>& Out, FString& Error) const;
    bool Place(const TArray<EDMArenaEnemy>& Types, FVector Center);
    void Undo(bool bAll = false);
    void ChangeParty(EDMInvestigator Player, TArray<EDMInvestigator> Companions);
    void FightOrPause();
    void ResetTest();
protected:
    virtual bool ShouldBeginEncounterOnStartPlay() const override { return false; }
    virtual FString PreferredInvestigator() const override { return DMArena::HeroName(Config.Player); }
    virtual bool AllowEditorBotControl() const override { return false; }
    virtual void ConfigureCaptureMetadata(const TSharedRef<FJsonObject>& Metadata) override;
    virtual bool HandleCustomOutcome(bool bInvestigatorsUp, bool bEnemiesUp) override;
private:
    UPROPERTY() TArray<TObjectPtr<ADMCombatant>> PlacedActors;
    bool bReloading = false;
    bool bProbe = false;
    double ProbeAt = 0;
    int32 ProbeTick = 0;
    int32 NextEntity = 0;
    bool SpawnPlacement(const FDMArenaPlacement& Placement);
    void RecordSetup(const FString& Action);
    void Reload(const FDMArenaConfig& NextConfig);
    void Probe();
};

UCLASS()
class DREADMERIDIAN_API ADMTestArenaController : public ADMCombatPlayerController
{
    GENERATED_BODY()
public:
    ADMTestArenaController();
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;
    virtual void PlayerTick(float DeltaSeconds) override;
    bool GroundPoint(FVector& Point) const;
    bool bPlacing = false;
    TArray<EDMArenaEnemy> Selection = { EDMArenaEnemy::Gunman };
private:
    UPROPERTY() TObjectPtr<class ACameraActor> SetupCamera;
};

UCLASS()
class DREADMERIDIAN_API ADMTestArenaHUD : public ADMCombatHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    bool Click(float X, float Y);
    bool OverPanel(float X, float Y) const;
    FVector2D PanelPoint(float X, float Y) const { return FVector2D(Left+X*Scale,Y*Scale); }
    bool HasDrawnPanel() const { return !Buttons.IsEmpty(); }
private:
    struct FButton { FBox2D Bounds; TFunction<void()> Action; };
    TArray<FButton> Buttons;
    TArray<int32> Counts = {1,0,0,0,0,0,0,0,0,0};
    float Scale = 1;
    float Left = 0;
    int32 PresetIndex = 0;
    int32 LastShotStage = -1;
    int32 ShotFrames = 0;
    void Button(const FString& Label, float X, float Y, float W, TFunction<void()> Action, bool bEnabled = true);
};
