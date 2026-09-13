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
{ return FString::Printf(TEXT("Lv %d | XP %d/1200 | Evolutions %d"), State.Level(), State.XP, State.Opportunities()); }
void UDMProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UDMProgressionComponent, State); }

