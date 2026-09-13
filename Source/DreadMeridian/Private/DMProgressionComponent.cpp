#include "DMProgressionComponent.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "Net/UnrealNetwork.h"
UDMProgressionComponent::UDMProgressionComponent() { SetIsReplicatedByDefault(true); }
bool UDMProgressionComponent::Award(const FString& EventId, int32 Amount)
{
    auto* Actor = Cast<ADMCombatant>(GetOwner());
    auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ADMCombatGameMode>() : nullptr;
    if (!Actor || !Actor->HasAuthority() || Actor->bIsEnemy || !Mode || !Mode->IsCombatActive()
        || EventId.IsEmpty() || RewardedEvents.Contains(EventId) || Amount <= 0) { return false; }
    RewardedEvents.Add(EventId);
    const int32 Before = State.XP;
    if (!State.Add(Amount)) { return false; }
    auto Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("entity_id"), Actor->EntityId);
    Data->SetStringField(TEXT("source"), EventId);
    Data->SetNumberField(TEXT("xp_before"), Before);
    Data->SetNumberField(TEXT("xp_after"), State.XP);
    Data->SetNumberField(TEXT("level"), State.Level());
    Data->SetNumberField(TEXT("opportunities"), State.Opportunities());
    Mode->Emit(TEXT("progression.awarded"), Data);
    Actor->ForceNetUpdate(); return true;
}
FString UDMProgressionComponent::Summary() const
{ return FString::Printf(TEXT("Lv %d | XP %d/1200 | Evolutions %d"), State.Level(), State.XP, State.Opportunities()) + FString::Printf(TEXT(" | Q:%c W:%c E:%c"), TCHAR(65 + State.Q), TCHAR(65 + State.W), TCHAR(65 + State.E)); }
void UDMProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UDMProgressionComponent, State); }



FString UDMProgressionComponent::Name(uint8 Slot) const
{
    const auto* Actor = Cast<ADMCombatant>(GetOwner());
    const auto* Entry = Actor ? DMEvolution::Find(Actor->Investigator->Kind, Slot, Node(Slot)) : nullptr;
    return Entry ? Entry->Name : FString();
}
bool UDMProgressionComponent::Choose(uint8 Slot, uint8 To)
{
    auto* Actor = Cast<ADMCombatant>(GetOwner());
    auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ADMCombatGameMode>() : nullptr;
    if (!Actor || !Actor->HasAuthority() || Actor->bIsEnemy || Actor->IsDown() || !Mode || !Mode->IsCombatActive()
        || !DMEvolution::Find(Actor->Investigator->Kind, Slot, To)) { return false; }
    const uint8 From = Node(Slot);
    if (!State.Choose(Slot, To)) { return false; }
    // Commit the graph transition before applying its cost; rejected/replayed requests cannot add floor.
    Actor->MadnessCore->RaiseFloor(Actor->MadnessCore->View().Floor + FDMProgressionState::FloorCost(To), TEXT("ability_evolution"));
    auto Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("entity_id"), Actor->EntityId);
    Data->SetStringField(TEXT("node_id"), DMEvolution::Find(Actor->Investigator->Kind, Slot, To)->Id);
    Data->SetNumberField(TEXT("predecessor"), From);
    Data->SetNumberField(TEXT("opportunities"), State.Opportunities());
    Mode->Emit(TEXT("progression.evolved"), Data);
    Actor->ForceNetUpdate(); return true;
}
void UDMProgressionComponent::ChooseForBot()
{
    auto* Actor = Cast<ADMCombatant>(GetOwner());
    auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ADMCombatGameMode>() : nullptr;
    if (!Actor || !Actor->HasAuthority() || Actor->bIsEnemy || Actor->IsDown() || Actor->ControlKind != TEXT("bot") || !Mode) { return; }
    float Crowd = 0, Danger = 0, Elite = 0;
    for (const ADMCombatant* Unit : Mode->GetCombatants())
    {
        if (!Unit) { continue; }
        if (!Unit->bIsEnemy) { Danger += Unit->IsDown() ? 2.f : 1.f - Unit->Health() / Unit->MaxHealth(); }
        else if (!Unit->IsDown() && FVector::Dist2D(Actor->GetActorLocation(), Unit->GetActorLocation()) < 1000)
        { Crowd += .5f; Elite += Unit->bCommonEnemy ? 0.f : 1.f; }
    }
    while (State.Opportunities() > 0)
    {
        float Best = -1; uint8 BestSlot = 255, BestNode = 255;
        for (uint8 Slot = 0; Slot < 3; ++Slot)
        { for (uint8 To = 1; To < 6; ++To)
          {
              const auto* Entry = DMEvolution::Find(Actor->Investigator->Kind, Slot, To);
              if (!Entry || !FDMProgressionState::Edge(Node(Slot), To)) { continue; }
              // Ability roles: Q values priority targets, W crowds, E protection/control.
              float Score = DMEvolution::Value(*Entry, Crowd * (Slot == 1 ? 1.3f : 1.f),
                  Danger * (Slot == 2 ? 1.3f : 1.f), Elite * (Slot == 0 ? 1.3f : 1.f));
              if (Score > Best) { Best = Score; BestSlot = Slot; BestNode = To; }
          } }
        if (!Choose(BestSlot, BestNode)) { break; }
    }
}
