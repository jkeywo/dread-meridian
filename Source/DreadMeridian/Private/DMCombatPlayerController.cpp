#include "DMCombatPlayerController.h"
#include "InputActionValue.h"
#include "DMCombatant.h"
#include "DMCombatPresentation.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DMCombatGameMode.h"
#include "DMEncounterLayout.h"
#include "DMGameState.h"
#include "DMScroungePickup.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

ADMCombatPlayerController::ADMCombatPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}
void ADMCombatPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
    Mapping = NewObject<UInputMappingContext>(this);
    MoveAction = NewObject<UInputAction>(this);
    MoveAction->ValueType = EInputActionValueType::Axis2D;
    ClickAction = NewObject<UInputAction>(this);
    AttackAction = NewObject<UInputAction>(this);
    CycleAction = NewObject<UInputAction>(this);
    ReviveAction = NewObject<UInputAction>(this);
    QAction = NewObject<UInputAction>(this); QPadAction = NewObject<UInputAction>(this);
    ConfirmQAction = NewObject<UInputAction>(this); CancelQAction = NewObject<UInputAction>(this);
    DetonateAction = NewObject<UInputAction>(this);
    PingAction = NewObject<UInputAction>(this);
    Mapping->MapKey(QAction, EKeys::Q); Mapping->MapKey(QPadAction, EKeys::Gamepad_LeftShoulder);
    Mapping->MapKey(ConfirmQAction, EKeys::LeftMouseButton);
    Mapping->MapKey(CancelQAction, EKeys::Escape); Mapping->MapKey(CancelQAction, EKeys::Gamepad_FaceButton_Right);
    Mapping->MapKey(DetonateAction, EKeys::F); Mapping->MapKey(DetonateAction, EKeys::Gamepad_FaceButton_Top);
    Input->BindAction(QAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::BeginQ);
    Input->BindAction(QPadAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::BeginQPad);
    Input->BindAction(ConfirmQAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::ConfirmQ);
    Input->BindAction(CancelQAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::CancelQ);
    Input->BindAction(DetonateAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::Detonate);
    Mapping->MapKey(PingAction, EKeys::G); Mapping->MapKey(PingAction, EKeys::Gamepad_DPad_Up);
    Input->BindAction(PingAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::PingPressed);
    Input->BindAction(PingAction, ETriggerEvent::Completed, this, &ADMCombatPlayerController::PingReleased);
    Mapping->MapKey(MoveAction, EKeys::Gamepad_Left2D);
    Mapping->MapKey(MoveAction, EKeys::D);
    Mapping->MapKey(MoveAction, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
    Mapping->MapKey(MoveAction, EKeys::W).Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Mapping));
    auto& South = Mapping->MapKey(MoveAction, EKeys::S);
    South.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Mapping));
    South.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
    Mapping->MapKey(ClickAction, EKeys::RightMouseButton);
    Mapping->MapKey(AttackAction, EKeys::SpaceBar);
    Mapping->MapKey(AttackAction, EKeys::Gamepad_FaceButton_Bottom);
    Mapping->MapKey(CycleAction, EKeys::Tab);
    Mapping->MapKey(CycleAction, EKeys::Gamepad_RightShoulder);
    Mapping->MapKey(ReviveAction, EKeys::E);
    Mapping->MapKey(ReviveAction, EKeys::Gamepad_FaceButton_Left);
    Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADMCombatPlayerController::Move);
    Input->BindAction(ClickAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::Click);
    Input->BindAction(AttackAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::Attack);
    Input->BindAction(CycleAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::Cycle);
    Input->BindAction(ReviveAction, ETriggerEvent::Started, this, &ADMCombatPlayerController::StartRevive);
    if (ULocalPlayer* Local = GetLocalPlayer())
    { Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->AddMappingContext(Mapping, 0); }
}

void ADMCombatPlayerController::Move(const FInputActionValue& Value)
{
    ADMCombatant* Actor = Cast<ADMCombatant>(GetPawn());
    if (!Actor)
    {
        if (auto* Observed = Cast<ADMCombatant>(GetViewTarget())) { SelectedTarget = Observed->GetAttackTarget(); }
        return;
    }
    const ADMGameState* State = GetWorld()->GetGameState<ADMGameState>();
    if (!Actor || Actor->IsDown() || !State || State->GetRunState().Phase != EDMRunPhase::Apocalypse) { return; }
    if (Actor->IsRestrained()) { return; }
    if (bQAiming && bQGamepad)
    {
        const FVector2D Axis = Value.Get<FVector2D>();
        PadAimOffset = FVector(Axis.Y, Axis.X, 0) * Actor->Primary->Range();
        if (!PadAimOffset.IsNearlyZero()) { SelectedTarget = nullptr; }
        return;
    }
    if (Actor->Primary->FrameTarget) { ServerCancelFrame(); }
    if (bAutoAttack) { bAutoAttack = false; ServerSelectTarget(nullptr); }
    Actor->StopGoal();
    const FVector2D Axis = Value.Get<FVector2D>().GetClampedToMaxSize(1);
    Actor->AddMovementInput(FVector(Axis.Y, Axis.X, 0), 1);
}
void ADMCombatPlayerController::Click()
{
    if (bQAiming) { CancelQ(); return; }
    ServerCancelFrame();
    ADMCombatant* Actor = Cast<ADMCombatant>(GetPawn());
    if (!Actor || Actor->IsDown()) { return; }
    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit)) { return; }
    if (ADMCombatant* Target = Cast<ADMCombatant>(Hit.GetActor()); Target && Target->bIsEnemy && !Target->IsDown())
    { SelectedTarget = Target; Attack(); }
    else
    {
        bAutoAttack = false;
        ServerSelectTarget(nullptr);
        Actor->MoveToward(FVector(FMath::Clamp(Hit.Location.X, -DMEncounterLayout::PlayableX, DMEncounterLayout::PlayableX), FMath::Clamp(Hit.Location.Y, -DMEncounterLayout::PlayableY, DMEncounterLayout::PlayableY), Actor->GetActorLocation().Z));
    }
}
void ADMCombatPlayerController::Cycle()
{
    TArray<ADMCombatant*> Targets;
    for (TActorIterator<ADMCombatant> It(GetWorld()); It; ++It)
    {
        const auto* Actor = Cast<ADMCombatant>(GetPawn());
        const bool bBinding = bQAiming && Actor && Actor->Investigator->Kind == EDMInvestigator::Medium;
        if ((It->bIsEnemy && !It->IsDown()) || bBinding) { Targets.Add(*It); }
    }
    Targets.Sort([](const ADMCombatant& A, const ADMCombatant& B) { return A.EntityId < B.EntityId; });
    if (Targets.IsEmpty()) { SelectedTarget = nullptr; return; }
    const int32 Index = Targets.IndexOfByKey(SelectedTarget.Get());
    SelectedTarget = Targets[(Index + 1) % Targets.Num()];
    if (bAutoAttack && !bQAiming) { ServerSelectTarget(SelectedTarget.Get()); }
}
void ADMCombatPlayerController::Attack()
{
    if (bQAiming) { ConfirmQ(); return; }
    StartAutoAttack(SelectedTarget.Get());
}
void ADMCombatPlayerController::StartAutoAttack(ADMCombatant* Target)
{
    auto* Actor = Cast<ADMCombatant>(GetPawn());
    if (!Actor || Actor->IsDown()) { return; }
    if (!IsValid(Target) || Target->IsDown() || !Target->bIsEnemy)
    {
        Target = nullptr; float Best = MAX_flt;
        for (TActorIterator<ADMCombatant> It(GetWorld()); It; ++It)
        {
            const float Distance = FVector::DistSquared(Actor->GetActorLocation(), It->GetActorLocation());
            if (It->bIsEnemy && !It->IsDown() && Distance < Best) { Best = Distance; Target = *It; }
        }
    }
    ServerCancelFrame(); SelectedTarget = Target; bAutoAttack = Target != nullptr;
    ServerSelectTarget(Target);
}
void ADMCombatPlayerController::StartRevive()
{
    CancelQ(); ServerCancelFrame();
    ADMCombatant* Actor = Cast<ADMCombatant>(GetPawn());
    if (!Actor || Actor->IsDown()) { return; }
    ADMCombatant* Nearest = nullptr;
    float Distance = FMath::Square(160.f);
    for (TActorIterator<ADMCombatant> It(GetWorld()); It; ++It)
    {
        const float Candidate = FVector::DistSquared(Actor->GetActorLocation(), It->GetActorLocation());
        if (!It->bIsEnemy && It->IsDown() && Candidate <= Distance) { Nearest = *It; Distance = Candidate; }
    }
    if (Nearest) { bAutoAttack = false; Actor->StopGoal(); ServerSelectTarget(nullptr); ServerRevive(Nearest); }
}
void ADMCombatPlayerController::ServerSelectTarget_Implementation(ADMCombatant* Target)
{
    ADMCombatant* Actor = Cast<ADMCombatant>(GetPawn());
    if (Actor && (!Target || (Target->GetWorld() == GetWorld() && Target->bIsEnemy && !Target->IsDown())))
    { Actor->SetAttackTarget(Target); }
}
void ADMCombatPlayerController::ServerRevive_Implementation(ADMCombatant* Ally)
{
    if (ADMCombatGameMode* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    { Mode->RequestRevive(Cast<ADMCombatant>(GetPawn()), Ally); }
}
void ADMCombatPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    ADMCombatant* Actor = Cast<ADMCombatant>(GetPawn());
    if (!Actor)
    {
        if (auto* Observed = Cast<ADMCombatant>(GetViewTarget())) { SelectedTarget = Observed->GetAttackTarget(); }
        return;
    }
    const ADMGameState* State = GetWorld()->GetGameState<ADMGameState>();
    if (bPingHeld && !bPingRadialOpen && GetWorld()->GetTimeSeconds() - PingPressedAt >= PingHoldSeconds) { bPingRadialOpen = true; }
#if !UE_BUILD_SHIPPING
    if (IsLocalController() && Actor && !Actor->IsDown())
    {
        if (FParse::Param(FCommandLine::Get(),TEXT("DMSmugglerProbe")) && !bVisualScheduled)
        {
            auto* Mode=GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
            if (Mode)
            {
                bVisualScheduled=true;TArray<ADMCombatant*> Subjects;Subjects.SetNumZeroed(5);
                for (ADMCombatant* A:Mode->GetCombatants())
                {
                    A->GetCharacterMovement()->DisableMovement();A->SetActorLocation(FVector(2500,2000,95));
                    const int32 EnemyRole=static_cast<int32>(A->Smuggler->Role);
                    if (A->bIsEnemy && EnemyRole>0 && EnemyRole<5 && !Subjects[EnemyRole-1]) { Subjects[EnemyRole-1]=A; }
                }
                auto* Boss=GetWorld()->SpawnActor<ADMCombatant>();Boss->Smuggler->Initialize(EDMSmuggler::GangBoss);
                Boss->InitializeCombatant(TEXT("enemy.preview.boss"),true,750,9);Subjects[4]=Boss;
                for (int32 I=0;I<Subjects.Num();++I)
                {
                    auto* A=Subjects[I];A->SetActorLocation(FVector(0,-360+I*180,95));
                    A->SetActorRotation(FRotator::ZeroRotator);A->Presentation->UpdatePresentation();
                }
                Actor->SetActorLocation(FVector(-220,0,95));
                auto* Camera=GetWorld()->SpawnActor<ACameraActor>(FVector(900,0,300),FRotator(-13,180,0));
                Camera->GetCameraComponent()->SetFieldOfView(62);SetViewTarget(Camera);
                auto Shot=[this](const TCHAR* Name,float Delay)
                {
                    FTimerHandle Timer;const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),FString::Printf(TEXT("Screenshots/Smugglers-%s.png"),Name));
                    GetWorldTimerManager().SetTimer(Timer,[Path]{FScreenshotRequest::RequestScreenshot(Path,false,false);},Delay,false);
                };
                Shot(TEXT("idle"),15);Shot(TEXT("attack"),16.15f);Shot(TEXT("signatures"),18.25f);
                FTimerHandle AttackTimer,SignatureTimer,ExitTimer;
                GetWorldTimerManager().SetTimer(AttackTimer,[Subjects]{for(auto* A:Subjects){A->Presentation->Cue(0,A->GetActorLocation()+FVector(200,0,0));}},16,false);
                GetWorldTimerManager().SetTimer(SignatureTimer,[Subjects,Mode,Actor]
                {
                    for(auto* A:Subjects){A->Smuggler->NextSignatureTick=0;A->Smuggler->TrySignature(*Mode,Actor);}
                },18,false);
                GetWorldTimerManager().SetTimer(ExitTimer,[]{FPlatformMisc::RequestExit(false);},21,false);
                ConsoleCommand(TEXT("showhud"),false);return;
            }
        }
        if ((FParse::Param(FCommandLine::Get(), TEXT("DMPresentationProbe")) || FParse::Param(FCommandLine::Get(), TEXT("DMLocomotionProbe"))) && !bVisualScheduled)
        {
            bVisualScheduled = true;
            auto* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
            if (Mode)
            {
                int32 I = 0;
                for (ADMCombatant* Subject : Mode->GetCombatants())
                {
                    Subject->GetCharacterMovement()->DisableMovement(); Subject->NextAttackTick = 100000;
                    Subject->SetActorLocation(Subject->bIsEnemy ? FVector(2500, 2000, 95) : FVector(0, -270 + I++ * 180, 95));
                    Subject->SetActorRotation(FRotator::ZeroRotator); Subject->Presentation->UpdatePresentation();
                }
                auto* Camera = GetWorld()->SpawnActor<ACameraActor>(FVector(670, 0, 270), FRotator(-14, 180, 0));
                Camera->GetCameraComponent()->SetFieldOfView(65); SetViewTarget(Camera);
                auto Shot = [this](const TCHAR* Name, float Delay)
                {
                    FTimerHandle Timer;
                    const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), FString::Printf(TEXT("Screenshots/Presentation-%s.png"), Name));
                    GetWorldTimerManager().SetTimer(Timer, [Path] { FScreenshotRequest::RequestScreenshot(Path, false, false); }, Delay, false);
                };
                if (FParse::Param(FCommandLine::Get(), TEXT("DMLocomotionProbe")))
                {
                    auto Change = [this, Mode](float Delay, float Speed, float Yaw)
                    {
                        FTimerHandle Timer;
                        GetWorldTimerManager().SetTimer(Timer, [Mode, Speed, Yaw]
                        {
                            for (ADMCombatant* Subject : Mode->GetCombatants())
                            {
                                if (Subject->bIsEnemy) { continue; }
                                Subject->GetCharacterMovement()->SetComponentTickEnabled(false);
                                Subject->GetCharacterMovement()->Velocity = FVector(Speed, 0, 0);
                                Subject->SetActorRotation(FRotator(0, Yaw, 0));
                                Subject->Presentation->UpdatePresentation();
                                UE_LOG(LogTemp, Display, TEXT("DREAD_LOCOMOTION_PROBE %s %s"), *Subject->DisplayName(), *Subject->Presentation->CurrentClip());
                            }
                        }, Delay, false);
                    };
                    Shot(TEXT("motion-idle"),15);
                    Change(16,420,0); Shot(TEXT("motion-start"),16.15f);
                    Change(17,0,0); Shot(TEXT("motion-stop"),17.18f);
                    Change(18,0,-90); Shot(TEXT("motion-left"),18.18f);
                    Change(19,0,0); Shot(TEXT("motion-right"),19.18f);
                    FTimerHandle Exit; GetWorldTimerManager().SetTimer(Exit, [] { FPlatformMisc::RequestExit(false); },21,false);
                    ConsoleCommand(TEXT("showhud"),false);
                    return;
                }
                Shot(TEXT("idle"), 15); Shot(TEXT("attack"), 16.15f); Shot(TEXT("q"), 18.2f); Shot(TEXT("down"), 21.5f);
                FTimerHandle AttackTimer, QTimer, DownTimer, ExitTimer;
                GetWorldTimerManager().SetTimer(AttackTimer, [Mode] { for (ADMCombatant* Subject : Mode->GetCombatants()) { if (!Subject->bIsEnemy) { Subject->Presentation->Cue(0, Subject->GetActorLocation() + FVector(200, 0, 0)); } } }, 16, false);
                GetWorldTimerManager().SetTimer(QTimer, [Mode] { for (ADMCombatant* Subject : Mode->GetCombatants()) { if (!Subject->bIsEnemy) { if (Subject->Investigator->Kind == EDMInvestigator::Photographer) { Subject->Primary->FrameTarget = Mode->GetCombatants().Last(); } Subject->Presentation->Cue(1, Subject->GetActorLocation() + FVector(150, 0, 0)); } } }, 18, false);
                GetWorldTimerManager().SetTimer(DownTimer, [Mode] { for (ADMCombatant* Subject : Mode->GetCombatants()) { if (!Subject->bIsEnemy) { Subject->InitializeCombatant(Subject->EntityId, false, 0, 18); } } }, 20, false);
                GetWorldTimerManager().SetTimer(ExitTimer, [] { FPlatformMisc::RequestExit(false); }, 23, false);
                ConsoleCommand(TEXT("showhud"), false);
            }
        }
        if (FParse::Param(FCommandLine::Get(), TEXT("DMVisualProbe")) && !bVisualScheduled)
        {
            bVisualScheduled = true;
            FTimerHandle ShotTimer, ExitTimer;
            GetWorldTimerManager().SetTimer(ShotTimer, [] {
                FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots/CombatSandbox.png")), true, false);
            }, 5.f, false);
            GetWorldTimerManager().SetTimer(ExitTimer, [] { FPlatformMisc::RequestExit(false); }, 7.f, false);
        }
        if ((FParse::Param(FCommandLine::Get(), TEXT("DMNetworkProbe")) || FParse::Param(FCommandLine::Get(), TEXT("DMVisualProbe"))) && (!SelectedTarget.IsValid() || SelectedTarget->IsDown() || !bAutoAttack)) { Attack(); }
        // Authority pawns think through ADMSquadController; the probe only drives the client's Q by RPC.
        if ((FParse::Param(FCommandLine::Get(), TEXT("DMNetworkProbe")) || FParse::Param(FCommandLine::Get(), TEXT("DMVisualProbe")))
            && !HasAuthority() && SelectedTarget.IsValid() && GetWorld()->GetTimeSeconds() > NextProbeQTime)
        {
            NextProbeQTime = GetWorld()->GetTimeSeconds() + .6f;
            const bool bSapper = Actor->Investigator->Kind == EDMInvestigator::Sapper;
            ServerCastQ(bSapper ? nullptr : SelectedTarget.Get(), SelectedTarget->GetActorLocation(), bSapper && !Actor->Primary->Satchels.IsEmpty());
        }
        if (FParse::Param(FCommandLine::Get(), TEXT("DMDisconnectProbe")) && !bDisconnectScheduled)
        {
            bDisconnectScheduled = true;
            FTimerHandle ExitTimer;
            GetWorldTimerManager().SetTimer(ExitTimer, [] {
                UE_LOG(LogTemp, Display, TEXT("DREAD_DISCONNECT_PROBE_COMPLETE"));
                FPlatformMisc::RequestExit(false);
            }, 1.f, false);
        }
    }
#endif
    if (!Actor || !State || State->GetRunState().Phase != EDMRunPhase::Apocalypse || Actor->IsDown())
    { if (Actor) { Actor->StopGoal(); } return; }
    if (Actor->Primary->FrameTarget || Actor->IsRestrained()) { Actor->StopGoal(); return; }
    if (bAutoAttack && SelectedTarget.IsValid() && !SelectedTarget->IsDown())
    {
        if (FVector::DistSquared2D(Actor->GetActorLocation(), SelectedTarget->GetActorLocation()) > FMath::Square(Actor->GetAttackRange() - 35))
        { Actor->MoveToward(SelectedTarget->GetActorLocation()); }
        else { Actor->StopGoal(); }
    }
}

void ADMCombatPlayerController::PawnLeavingGame()
{
    if (ADMCombatGameMode* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>()) { Mode->ReleaseInvestigator(this); }
    else { Super::PawnLeavingGame(); }
}

void ADMCombatPlayerController::ClientVerifyCombatState_Implementation(const FString& ExpectedJson)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("DMNetworkProbe"))) { return; }
    // Attributes replicate on different actor channels; wait for their final updates.
    FTimerHandle VerifyTimer;
    GetWorldTimerManager().SetTimer(VerifyTimer, FTimerDelegate::CreateWeakLambda(this, [this, ExpectedJson] {
        TSharedPtr<FJsonObject> Expected;
        bool bPassed = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ExpectedJson), Expected);
        int32 Count = 0;
        if (bPassed)
        {
            for (TActorIterator<ADMCombatant> It(GetWorld()); It; ++It)
            {
                ++Count;
                double HP = -1, Shield = -1;
                const bool bHasNumbers = Expected->TryGetNumberField(It->EntityId + TEXT(".health"), HP) && Expected->TryGetNumberField(It->EntityId + TEXT(".shield"), Shield);
                // Each field is checked on its own so a mismatch names the actor and field in the client log.
                auto Check = [&](const TCHAR* Field, bool bOk, const FString& Got, const FString& Want)
                {
                    if (!bOk) { UE_LOG(LogTemp, Display, TEXT("DREAD_NETWORK_PROBE_MISMATCH %s.%s client=%s server=%s"), *It->EntityId, Field, *Got, *Want); }
                    bPassed &= bOk;
                };
                Check(TEXT("health"), bHasNumbers && FMath::IsNearlyEqual(It->Health(), static_cast<float>(HP)), FString::SanitizeFloat(It->Health()), FString::SanitizeFloat(HP));
                Check(TEXT("shield"), bHasNumbers && FMath::IsNearlyEqual(It->Shield(), static_cast<float>(Shield)), FString::SanitizeFloat(It->Shield()), FString::SanitizeFloat(Shield));
                Check(TEXT("name"), Expected->GetStringField(It->EntityId + TEXT(".name")) == It->DisplayName(), It->DisplayName(), Expected->GetStringField(It->EntityId + TEXT(".name")));
                Check(TEXT("resources"), Expected->GetStringField(It->EntityId + TEXT(".resources")) == It->Investigator->ResourceSummary(), It->Investigator->ResourceSummary(), Expected->GetStringField(It->EntityId + TEXT(".resources")));
                Check(TEXT("primary"), Expected->GetStringField(It->EntityId + TEXT(".primary")) == It->Primary->ReplicationSummary(), It->Primary->ReplicationSummary(), Expected->GetStringField(It->EntityId + TEXT(".primary")));
            }
            const ADMGameState* State = GetWorld()->GetGameState<ADMGameState>();
            bPassed &= State && Expected->GetStringField(TEXT("phase")) == StaticEnum<EDMRunPhase>()->GetNameStringByValue(static_cast<int64>(State->GetRunState().Phase));
        }
        bPassed &= Count == 7;
        UE_LOG(LogTemp, Display, TEXT("DREAD_NETWORK_PROBE_%s actors=%d"), bPassed ? TEXT("PASSED") : TEXT("FAILED"), Count);
        FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
    }), 3.f, false);
#endif
}

void ADMCombatPlayerController::BeginQ()
{
    auto* Actor = Cast<ADMCombatant>(GetPawn());
    if (!Actor || Actor->IsDown() || Actor->IsRestrained()) { return; }
    bQAiming = !bQAiming; bQGamepad = false; LastQFeedback.Reset();
}
void ADMCombatPlayerController::BeginQPad()
{
    BeginQ(); bQGamepad = true;
    auto* Actor = Cast<ADMCombatant>(GetPawn());
    if (Actor) { PadAimOffset = Actor->GetActorForwardVector() * FMath::Min(400.f, Actor->Primary->Range()); }
}
void ADMCombatPlayerController::GetQAim(ADMCombatant*& Target, FVector& Point) const
{
    Target = nullptr; Point = FVector::ZeroVector;
    auto* Actor = Cast<ADMCombatant>(GetPawn()); if (!Actor) { return; }
    Point = Actor->GetActorLocation() + Actor->GetActorForwardVector() * 300;
    if (bQGamepad)
    {
        if (SelectedTarget.IsValid() && !Actor->Primary->HeldTarget) { Target = SelectedTarget.Get(); Point = Target->GetActorLocation(); }
        else { Point = Actor->GetActorLocation() + PadAimOffset; }
    }
    else
    {
        FHitResult Hit;
        if (GetHitResultUnderCursor(ECC_Visibility, false, Hit)) { Target = Cast<ADMCombatant>(Hit.GetActor()); Point = Hit.Location; }
    }
    if (Actor->Investigator->Kind == EDMInvestigator::Sapper || Actor->Primary->HeldTarget) { Target = nullptr; }
}
void ADMCombatPlayerController::ConfirmQ()
{
    if (!bQAiming)
    {
        FHitResult Hit;
        if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
        { if (auto* Enemy = Cast<ADMCombatant>(Hit.GetActor()); Enemy && Enemy->bIsEnemy && !Enemy->IsDown()) { StartAutoAttack(Enemy); } }
        return;
    }
    ADMCombatant* Target; FVector Point; GetQAim(Target, Point);
    bQAiming = false; ServerCastQ(Target, Point, false);
}
void ADMCombatPlayerController::CancelQ() { bQAiming = false; }
void ADMCombatPlayerController::Detonate()
{ if (GetPawn()) { ServerCastQ(nullptr, GetPawn()->GetActorLocation(), true); } }
void ADMCombatPlayerController::ServerCastQ_Implementation(ADMCombatant* Target, FVector Point, bool bDetonate)
{
    auto* Actor = Cast<ADMCombatant>(GetPawn());
    if (!Actor) { return; }
    const bool bCast = Actor->Primary->Request(Target, Point, bDetonate);
    ClientQFeedback(bCast ? (bDetonate ? TEXT("Satchels detonated") : Actor->Primary->Name()) : Actor->Primary->LastFailure);
}
void ADMCombatPlayerController::ServerCancelFrame_Implementation()
{ if (auto* Actor = Cast<ADMCombatant>(GetPawn())) { Actor->Primary->CancelChannel(); } }
void ADMCombatPlayerController::ClientQFeedback_Implementation(const FString& Message)
{ LastQFeedback = Message; FeedbackUntil = GetWorld()->GetTimeSeconds() + 2.5f; }
FString ADMCombatPlayerController::QFeedback() const
{ return GetWorld()->GetTimeSeconds() <= FeedbackUntil ? LastQFeedback : TEXT(""); }

// ------------------------------------------------------------------- pings

namespace
{
    FVector ClampPlayable(const FVector& Point)
    { return FVector(FMath::Clamp(Point.X, -DMEncounterLayout::PlayableX, DMEncounterLayout::PlayableX), FMath::Clamp(Point.Y, -DMEncounterLayout::PlayableY, DMEncounterLayout::PlayableY), Point.Z); }
    ADMCombatant* CombatantById(UWorld* World, const FString& Id)
    {
        if (Id.IsEmpty()) { return nullptr; }
        for (TActorIterator<ADMCombatant> It(World); It; ++It) { if (It->EntityId == Id) { return *It; } }
        return nullptr;
    }
    /** World point the HUD projects for a ping marker; keep in sync with PingAnchor in DMCombatHUD.cpp. */
    FVector PingAnchor(EDMPingKind Kind, const FVector& Location)
    { return Location + FVector(0, 0, FDMPingBoard::NeedsTarget(Kind) ? 260.f : 30.f); }
}

const TArray<EDMPingKind>& ADMCombatPlayerController::RadialKinds()
{
    static const TArray<EDMPingKind> Kinds = { EDMPingKind::GoHere, EDMPingKind::Defend, EDMPingKind::Retreat, EDMPingKind::Help,
        EDMPingKind::Focus, EDMPingKind::Ignore, EDMPingKind::Perceive };
    return Kinds;
}
int32 ADMCombatPlayerController::GetPingRadialHover() const
{
    float MX = 0, MY = 0;
    if (!GetMousePosition(MX, MY)) { return INDEX_NONE; }
    const FVector2D Delta = FVector2D(MX, MY) - PingRadialOrigin;
    if (Delta.SizeSquared() < FMath::Square(PingRadialDeadZone)) { return INDEX_NONE; }
    const int32 Count = RadialKinds().Num();
    const float Width = UE_TWO_PI / Count;
    // Clockwise from 12 o'clock in screen space (Y down); sector 0 is centred on the top.
    float Angle = static_cast<float>(FMath::Atan2(Delta.X, -Delta.Y)) + Width * .5f;
    if (Angle < 0) { Angle += UE_TWO_PI; }
    return FMath::Clamp(static_cast<int32>(Angle / Width), 0, Count - 1);
}
void ADMCombatPlayerController::PingPressed()
{
    if (!Cast<ADMCombatant>(GetPawn())) { return; }
    bPingHeld = true; bPingRadialOpen = false;
    PingPressedAt = GetWorld()->GetTimeSeconds();
    float MX = 0, MY = 0;
    if (!GetMousePosition(MX, MY)) { int32 SX = 0, SY = 0; GetViewportSize(SX, SY); MX = SX * .5f; MY = SY * .5f; }
    PingRadialOrigin = FVector2D(MX, MY);
}
void ADMCombatPlayerController::PingReleased()
{
    if (!bPingHeld) { return; }
    const bool bRadial = bPingRadialOpen;
    bPingHeld = false; bPingRadialOpen = false;
    ADMCombatant* Actor = Cast<ADMCombatant>(GetPawn());
    if (!Actor) { return; }
    if (!bRadial)
    {
        EDMPingKind Kind; FVector Location; ADMCombatant* Target; int32 Existing;
        if (ResolveContextPing(Kind, Location, Target, Existing)) { ServerPing(static_cast<uint8>(Kind), Location, Target, Existing); }
        return;
    }
    const int32 Hover = GetPingRadialHover();
    if (Hover == INDEX_NONE) { return; }
    const EDMPingKind Kind = RadialKinds()[Hover];
    FHitResult Hit;
    const bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, Hit);
    ADMCombatant* Target = nullptr;
    if (FDMPingBoard::NeedsTarget(Kind))
    {
        // Help wants an investigator (down or not); Focus/Ignore want a living enemy, falling back to the selected target.
        Target = bHit ? Cast<ADMCombatant>(Hit.GetActor()) : nullptr;
        if (Target && (Kind == EDMPingKind::Help ? Target->bIsEnemy : (!Target->bIsEnemy || Target->IsDown()))) { Target = nullptr; }
        if (!Target && Kind != EDMPingKind::Help && SelectedTarget.IsValid() && SelectedTarget->bIsEnemy && !SelectedTarget->IsDown()) { Target = SelectedTarget.Get(); }
        if (!Target) { LastQFeedback = TEXT("Ping needs a target"); FeedbackUntil = GetWorld()->GetTimeSeconds() + 2.5f; return; }
    }
    const FVector Location = Target ? Target->GetActorLocation() : ClampPlayable(bHit ? Hit.Location : Actor->GetActorLocation());
    ServerPing(static_cast<uint8>(Kind), Location, Target, INDEX_NONE);
}
bool ADMCombatPlayerController::ResolveContextPing(EDMPingKind& OutKind, FVector& OutLocation, ADMCombatant*& OutTarget, int32& OutExistingPingId) const
{
    OutKind = EDMPingKind::GoHere; OutLocation = FVector::ZeroVector; OutTarget = nullptr; OutExistingPingId = INDEX_NONE;
    const int32 Existing = PingUnderCursor();
    if (Existing != INDEX_NONE) { OutExistingPingId = Existing; return true; }
    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit)) { return false; }
    OutLocation = ClampPlayable(Hit.Location);
    if (ADMCombatant* Unit = Cast<ADMCombatant>(Hit.GetActor()))
    {
        if (Unit->bIsEnemy && !Unit->IsDown()) { OutKind = EDMPingKind::Enemy; OutTarget = Unit; OutLocation = Unit->GetActorLocation(); }
        else if (!Unit->bIsEnemy && (Unit->IsDown() || Unit->Health() < Unit->MaxHealth() * .5f)) { OutKind = EDMPingKind::Help; OutTarget = Unit; OutLocation = Unit->GetActorLocation(); }
        return true; // a healthy ally or a dead enemy reads as ground under the cursor
    }
    if (const ADMScroungePickup* Pickup = Cast<ADMScroungePickup>(Hit.GetActor()))
    { OutKind = EDMPingKind::Pickup; OutLocation = ClampPlayable(Pickup->GetActorLocation()); }
    return true;
}
int32 ADMCombatPlayerController::PingUnderCursor() const
{
    // Returns the ping Id (ids start at 1), matching ServerPing's ExistingPingId contract.
    const ADMGameState* State = GetWorld()->GetGameState<ADMGameState>();
    float MX = 0, MY = 0;
    if (!State || !GetMousePosition(MX, MY)) { return INDEX_NONE; }
    const int32 Tick = State->GetCombatTick();
    int32 Best = INDEX_NONE;
    float BestDistance = FMath::Square(PingRadialDeadZone);
    for (const FDMPing& Ping : State->Pings)
    {
        if (!Ping.IsLive(Tick)) { continue; }
        FVector Location = Ping.Location;
        if (const ADMCombatant* Target = CombatantById(GetWorld(), Ping.TargetId)) { Location = Target->GetActorLocation(); }
        FVector2D Screen;
        if (!ProjectWorldLocationToScreen(PingAnchor(Ping.Kind, Location), Screen)) { continue; }
        const float Distance = static_cast<float>(FVector2D::DistSquared(Screen, FVector2D(MX, MY)));
        if (Distance < BestDistance) { BestDistance = Distance; Best = Ping.Id; }
    }
    return Best;
}
void ADMCombatPlayerController::ServerPing_Implementation(uint8 Kind, FVector Location, ADMCombatant* Target, int32 ExistingPingId)
{
    ADMCombatant* Actor = Cast<ADMCombatant>(GetPawn());
    ADMCombatGameMode* Mode = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!Actor || !Mode || Kind >= static_cast<uint8>(EDMPingKind::Count)) { return; }
    if (ExistingPingId >= 1)
    {
        const FDMPing* Existing = Mode->GetPingBoard().Find(ExistingPingId);
        if (!Existing) { return; }
        if (Existing->AuthorId == Actor->EntityId) { Mode->CancelPing(ExistingPingId, Actor->EntityId); }
        else { Mode->AcknowledgePing(ExistingPingId, Actor->EntityId); }
        return;
    }
    const EDMPingKind PingKind = static_cast<EDMPingKind>(Kind);
    if (Actor->IsDown() && PingKind != EDMPingKind::Help && PingKind != EDMPingKind::Perceive) { ClientQFeedback(TEXT("Ping rejected")); return; }
    if (Target && (Target->GetWorld() != GetWorld() || !FDMPingBoard::AllowsTarget(PingKind))) { Target = nullptr; }
    if (Mode->CreatePing(PingKind, Actor->EntityId, false, ClampPlayable(Location), Target ? Target->EntityId : FString()) == INDEX_NONE)
    { ClientQFeedback(TEXT("Ping rejected")); }
}
