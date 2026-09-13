#include "DMObjective.h"
#include "DMObjectiveCatalogue.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMRecoverySupply.h"
#include "DMRelicDrop.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ADMObjective::ADMObjective()
{
    bReplicates = true; PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Objective")); SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder"));
    Mesh->SetStaticMesh(Shape.Object); Mesh->SetRelativeScale3D(FVector(.65f,.65f,.3f));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Instructions")); Label->SetupAttachment(Mesh);
    Label->SetAbsolute(false,true,true); Label->SetRelativeLocation(FVector(0,0,160)); Label->SetWorldSize(18);
}
bool ADMObjective::Configure(const FString& Id, const FString& Title, const TArray<FDMObjectiveStep>& Plan, EDMObjectiveReward RewardKind, int32 Deadline)
{
    if (!HasAuthority() || Id.IsEmpty() || Plan.IsEmpty() || !PublicState.Id.IsEmpty()) { return false; }
    for (const auto& S : Plan) { if (S.WorkTicks < 1 || S.Location.ContainsNaN() || S.Destination.ContainsNaN() || (S.Verb == EDMObjectiveVerb::Sequence && S.Sequence.IsEmpty())) { return false; } }
    PublicState.Id = Id; PublicState.Deadline = FMath::Max(0,Deadline); PublicState.bRevealed = true;
    Steps = Plan; Reward = RewardKind; DisplayTitle = Title; PublicState.PayloadLocation = Plan[0].Location;
    SetActorLocation(Plan[0].Location); Publish(TEXT("available")); return true;
}
bool ADMObjective::ConfigureAuthored(const FString& TemplateId, FVector Origin, int32 InDifficulty, const FString& InstanceId)
{
    FDMObjectiveDefinition D;
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!HasAuthority() || !M || !DMObjectiveCatalogue::Build(TemplateId,Origin,InDifficulty,D) || !Configure(InstanceId,D.Title,D.Steps,D.Reward)) { return false; }
    bDisruption = D.bDisruption; Difficulty = InDifficulty;
    Targets.SetNum(Steps.Num());
    for (int32 I=0; I<Steps.Num(); ++I)
    { if (Steps[I].Verb == EDMObjectiveVerb::Destroy) { Targets[I] = M->SpawnEncounterActor(InstanceId + FString::Printf(TEXT(".idol.%d"),I),Steps[I].Location + FVector(0,0,80),150,0,true); } }
    if (Targets.IsValidIndex(0)) { Destructible = Targets[0]; }
    return true;
}
const FDMObjectiveStep* ADMObjective::Current() const { return Steps.IsValidIndex(PublicState.Step) ? &Steps[PublicState.Step] : nullptr; }
bool ADMObjective::IsTerminal() const { return PublicState.State == EDMObjectiveState::Completed || PublicState.State == EDMObjectiveState::Failed; }
bool ADMObjective::Eligible(ADMCombatant* A, FVector At) const
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    return M && M->IsCombatActive() && IsValid(A) && A->GetWorld() == GetWorld() && !A->bIsEnemy && !A->IsDown() && !A->IsStunned() && !A->IsRestrained()
        && FVector::DistSquared2D(A->GetActorLocation(),At) <= FMath::Square(180.f)
        && A->Smuggler->Sight(At + FVector(0,0,40), A->GetActorLocation());
}
bool ADMObjective::Contested(FVector At) const
{
    auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
    if (!M) { return true; }
    for (ADMCombatant* A : M->GetCombatants()) { if (IsValid(A) && A->bIsEnemy && !A->IsDown() && FVector::DistSquared2D(A->GetActorLocation(),At) < FMath::Square(350.f)) { return true; } }
    return false;
}
bool ADMObjective::Interact(ADMCombatant* A, int32 Symbol)
{
    const auto* S = Current();
    if (!HasAuthority() || !S || IsTerminal() || !Eligible(A,PublicState.PayloadLocation) || (Participant.IsValid() && Participant.Get() != A)) { return false; }
    if (S->Verb == EDMObjectiveVerb::Destroy) { return false; }
    if (S->Verb == EDMObjectiveVerb::Sequence)
    {
        if (!bSequenceReady) { return false; }
        if (Symbol < 0) { return false; }
        if (!S->Sequence.IsValidIndex(PublicState.Progress) || Symbol != S->Sequence[PublicState.Progress])
        { PublicState.Progress = 0; bSequenceReady = false; SequenceStarted = -1; Publish(TEXT("sequence_mistake")); return false; }
        ++PublicState.Progress; PublicState.State = EDMObjectiveState::Active;
        if (PublicState.Progress >= S->Sequence.Num()) { Advance(); } else { Publish(TEXT("sequence_input")); }
        return true;
    }
    Participant = A; DamageAtStart = A->LastDamageTick; PublicState.State = EDMObjectiveState::Active;
    CarrierId = S->Verb == EDMObjectiveVerb::Carry ? A->EntityId : FString();
    Publish(TEXT("interaction_started")); return true;
}
bool ADMObjective::IsInteracting(const ADMCombatant* A) const { return Participant.Get() == A && !IsTerminal(); }
void ADMObjective::Release(ADMCombatant* A, bool bExplicitInterrupt)
{
    if (!HasAuthority() || !IsInteracting(A)) { return; }
    if (!bExplicitInterrupt) { A->Relics->ObjectiveFinished.Broadcast(); }
    Participant.Reset(); CarrierId.Reset(); Publish(bExplicitInterrupt ? TEXT("interrupted") : TEXT("released"));
}
void ADMObjective::Step(int32 Tick)
{
    if (!HasAuthority() || Tick <= LastTick || IsTerminal() || !Current()) { return; }
    LastTick = Tick;
    if (PublicState.Deadline > 0)
    {
        const int32 Left = PublicState.Deadline - Tick;
        if (Left <= 0) { Fail(); return; }
        const auto Urgency = Left <= 100 ? EDMObjectiveState::Imminent : Left <= 300 ? EDMObjectiveState::Critical : Left <= 600 ? EDMObjectiveState::Deteriorating : PublicState.State;
        if (Urgency != PublicState.State) { PublicState.State = Urgency; Publish(TEXT("urgency")); }
    }
    const FDMObjectiveStep S = *Current();
    if (S.Verb == EDMObjectiveVerb::Sequence && !bSequenceReady)
    {
        if (SequenceStarted < 0) { SequenceStarted = Tick; }
        const int32 Index = (Tick - SequenceStarted) / 10;
        ObservedSymbol = S.Sequence.IsValidIndex(Index) ? S.Sequence[Index] : 0;
        bSequenceReady = Index >= S.Sequence.Num();
    }
    if (Difficulty >= 2 && bDisruption)
    {
        auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>();
        for (ADMCombatant* Idol : Targets)
        { if (IsValid(Idol) && !Idol->IsDown() && M)
          { for (ADMCombatant* Enemy : M->GetCombatants())
            { if (Enemy != Idol && Enemy->bIsEnemy && !Enemy->IsDown() && FVector::DistSquared2D(Enemy->GetActorLocation(),Idol->GetActorLocation()) < FMath::Square(400.f)) { Enemy->SpiritProtection = FMath::Max(Enemy->SpiritProtection,.2f); } } } }
    }
    if (S.Verb == EDMObjectiveVerb::Destroy)
    { if (IsValid(Destructible) && Destructible->IsDown()) { Advance(); } return; }
    auto* A = Participant.Get();
    if (!A) { return; }
    const bool bCarry = S.Verb == EDMObjectiveVerb::Carry;
    if (!Eligible(A,bCarry ? A->GetActorLocation() : PublicState.PayloadLocation) || (A->LastDamageTick != DamageAtStart && !A->Relics->ProtectsObjective()))
    { Release(A,true); return; }
    if (bCarry)
    {
        PublicState.PayloadLocation = A->GetActorLocation();
        if (FVector::DistSquared2D(PublicState.PayloadLocation,S.Destination) <= FMath::Square(150.f)) { Advance(); }
    }
    else if (S.Verb == EDMObjectiveVerb::Escort)
    {
        if (Contested(PublicState.PayloadLocation)) { return; }
        PublicState.PayloadLocation = FMath::VInterpConstantTo(PublicState.PayloadLocation,S.Destination,.1f,100.f);
        if (FVector::DistSquared2D(PublicState.PayloadLocation,S.Destination) <= FMath::Square(15.f)) { Advance(); }
    }
    else if (S.Verb != EDMObjectiveVerb::Sequence && (S.Verb != EDMObjectiveVerb::Defend || !Contested(S.Location)))
    { if (++PublicState.Progress >= S.WorkTicks) { Advance(); } }
}
void ADMObjective::Advance()
{
    if (auto* A=Participant.Get()) { A->Relics->ObjectiveFinished.Broadcast(); }
    Participant.Reset(); CarrierId.Reset(); Destructible = nullptr; PublicState.Progress = 0; SequenceStarted = -1; bSequenceReady = false; ObservedSymbol = 0;
    ++PublicState.Step;
    if (Targets.IsValidIndex(PublicState.Step)) { Destructible = Targets[PublicState.Step]; }
    if (const auto* S = Current()) { PublicState.PayloadLocation = S->Location; SetActorLocation(S->Location); Publish(TEXT("step_completed")); }
    else { PublicState.State = EDMObjectiveState::Completed; GrantReward(); Publish(TEXT("completed")); }
}
void ADMObjective::GrantReward()
{
    if (PublicState.bRewarded) { return; }
    PublicState.bRewarded = true;
    if (auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    { for (ADMCombatant* A : M->GetCombatants()) { if (!A->bIsEnemy) { A->Progression->Award(TEXT("objective.") + PublicState.Id, XPReward); } } }
    if (Reward == EDMObjectiveReward::Treatment)
    { auto* Supply = GetWorld()->SpawnActor<ADMRecoverySupply>(PublicState.PayloadLocation,FRotator::ZeroRotator); if (Supply) { Supply->Charges = 3; } }
    if (Reward == EDMObjectiveReward::Relic)
    { auto* Drop=GetWorld()->SpawnActor<ADMRelicDrop>(); if (Drop && !Drop->Initialize(TEXT("objective.")+PublicState.Id)) { Drop->Destroy(); } }
    bVisionOnline = Reward == EDMObjectiveReward::Vision;
    bBasinDrained = Reward == EDMObjectiveReward::DrainBasin;
}
bool ADMObjective::Fail()
{ if (!HasAuthority() || IsTerminal() || !Current()) { return false; } Participant.Reset(); CarrierId.Reset(); PublicState.State = EDMObjectiveState::Failed; Publish(TEXT("failed")); return true; }
bool ADMObjective::Repair(const TArray<FDMObjectiveStep>& Replacement, const FString& Explanation)
{
    if (!HasAuthority() || PublicState.State != EDMObjectiveState::Failed || Replacement.IsEmpty() || (PublicState.bRevealed && Explanation.IsEmpty())) { return false; }
    for (const auto& S : Replacement) { if (S.WorkTicks < 1 || S.Location.ContainsNaN() || S.Destination.ContainsNaN() || (S.Verb == EDMObjectiveVerb::Sequence && S.Sequence.IsEmpty())) { return false; } }
    // Keep completed steps as established history; replace only the remaining future.
    Steps.SetNum(PublicState.Step); Steps.Append(Replacement); PublicState.Progress = 0; PublicState.Deadline = 0;
    PublicState.State = EDMObjectiveState::Repaired; RepairExplanation = Explanation; PublicState.PayloadLocation = Current()->Location;
    Publish(TEXT("repaired")); return true;
}
bool ADMObjective::ConvertApocalypse()
{
    if (!HasAuthority() || IsTerminal() || PublicState.bApocalypse || !Current()) { return false; }
    PublicState.bApocalypse = true; PublicState.State = EDMObjectiveState::ApocalypseConverted;
    Publish(TEXT("apocalypse_converted")); return true;
}
bool ADMObjective::Restore(const FDMObjectiveSnapshot& S)
{
    if (!HasAuthority() || S.Version != 1 || S.Id != PublicState.Id || S.Step < 0 || S.Step > Steps.Num() || S.Progress < 0 || S.PayloadLocation.ContainsNaN()
        || static_cast<uint8>(S.State) > static_cast<uint8>(EDMObjectiveState::ApocalypseConverted)
        || (S.State == EDMObjectiveState::Completed && (S.Step != Steps.Num() || !S.bRewarded))
        || (S.State != EDMObjectiveState::Completed && (S.Step == Steps.Num() || S.bRewarded))) { return false; }
    PublicState = S; Participant.Reset(); CarrierId.Reset(); LastTick = -1; ForceNetUpdate(); return true;
}
void ADMObjective::Publish(const FString& Reason)
{
    ForceNetUpdate();
    if (auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    { auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("objective_id"),PublicState.Id); D->SetStringField(TEXT("reason"),Reason); D->SetNumberField(TEXT("step"),PublicState.Step); M->Emit(TEXT("objective.changed"),D); }
}
void ADMObjective::Tick(float Delta)
{
    Super::Tick(Delta);
    const auto* S = Current();
    SetActorLocation(PublicState.PayloadLocation);
    Label->SetText(FText::FromString(DisplayTitle + TEXT("\n") + (S ? S->Instruction : TEXT("Completed")) + FString::Printf(TEXT(" | %d"),PublicState.Progress)));
    if (S && S->Verb == EDMObjectiveVerb::Sequence)
    { Label->SetText(FText::FromString(DisplayTitle + (bSequenceReady ? TEXT("\nRepeat: keys 1 / 2 / 3") : FString::Printf(TEXT("\nObserve bell: %d"),ObservedSymbol)))); }
}
void ADMObjective::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADMObjective,PublicState); DOREPLIFETIME(ADMObjective,DisplayTitle); DOREPLIFETIME(ADMObjective,RepairExplanation);
    DOREPLIFETIME(ADMObjective,Steps); DOREPLIFETIME(ADMObjective,Reward); DOREPLIFETIME(ADMObjective,Destructible); DOREPLIFETIME(ADMObjective,CarrierId);
    DOREPLIFETIME(ADMObjective,bVisionOnline); DOREPLIFETIME(ADMObjective,bBasinDrained);
    DOREPLIFETIME(ADMObjective,Targets); DOREPLIFETIME(ADMObjective,Difficulty); DOREPLIFETIME(ADMObjective,ObservedSymbol); DOREPLIFETIME(ADMObjective,bSequenceReady);
}

