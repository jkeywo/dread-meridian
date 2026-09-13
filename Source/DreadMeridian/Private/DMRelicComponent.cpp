#include "DMRelicComponent.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "Net/UnrealNetwork.h"
UDMRelicComponent::UDMRelicComponent() { SetIsReplicatedByDefault(true); }
ADMCombatant* UDMRelicComponent::Self() const { return Cast<ADMCombatant>(GetOwner()); }
bool UDMRelicComponent::CanAcquire(EDMRelic R) const
{ return Self() && !Self()->bIsEnemy && static_cast<uint8>(R)<static_cast<uint8>(EDMRelic::Count) && !Has(R) && Inventory.Items.Num()<FMath::Clamp(Capacity,1,8); }
bool UDMRelicComponent::Acquire(EDMRelic R,const FString& AwardId)
{
    if (!Self() || !Self()->HasAuthority() || !CanAcquire(R) || AwardId.IsEmpty() || Inventory.AwardIds.Contains(AwardId)) { return false; }
    Inventory.Items.Add(R); Inventory.AwardIds.Add(AwardId); Self()->ForceNetUpdate();
    if (auto* M = GetWorld()->GetAuthGameMode<ADMCombatGameMode>())
    { auto D = MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"),Self()->EntityId); D->SetStringField(TEXT("award_id"),AwardId); D->SetStringField(TEXT("relic"),Name(R)); M->Emit(TEXT("relic.acquired"),D); }
    return true;
}
bool UDMRelicComponent::Restore(const FDMRelicInventory& S)
{
    if (!Self() || !Self()->HasAuthority() || S.Version!=1 || S.Items.Num()>FMath::Clamp(Capacity,1,8) || S.AwardIds.Num()!=S.Items.Num()) { return false; }
    TSet<EDMRelic> Unique; TSet<FString> Awards;
    for (EDMRelic R : S.Items) { if (static_cast<uint8>(R)>=static_cast<uint8>(EDMRelic::Count) || Unique.Contains(R)) { return false; } Unique.Add(R); }
    for (const FString& Id : S.AwardIds) { if (Id.IsEmpty() || Awards.Contains(Id)) { return false; } Awards.Add(Id); }
    Inventory = S; Self()->ForceNetUpdate(); return true;
}
FString UDMRelicComponent::Name(EDMRelic R)
{
    static const TCHAR* Names[] = {TEXT("Officer's Swagger"),TEXT("Cracked Saint's Medal"),TEXT("Hangman's Knot"),TEXT("Overheal Shield (name pending)"),TEXT("Soldier's Morphine Tin"),TEXT("Black Glass Rosary"),TEXT("Ferryman's Coin"),TEXT("Saint's Work Gloves")};
    return static_cast<uint8>(R)<8 ? Names[static_cast<uint8>(R)] : TEXT("Unknown relic");
}
FString UDMRelicComponent::Description(EDMRelic R)
{
    static const TCHAR* Text[] = {TEXT("Control draws threat; allies Break your focused enemy more effectively."),TEXT("Contribute to a Break, then spend your primed ability during its vulnerability."),TEXT("Hard control spreads weaker control to nearby enemies."),TEXT("Excess healing becomes temporary, decaying Shield."),TEXT("Remove a specific Injury; future wounds become Grievous. Faster revives both ways."),TEXT("Entering a higher Madness tier briefly strengthens your resource."),TEXT("Rapid movement leaves wakes that help allies and hinder enemies."),TEXT("Protect objective interactions; finishing or releasing grants nearby defense.")};
    return static_cast<uint8>(R)<8 ? Text[static_cast<uint8>(R)] : FString();
}
FString UDMRelicComponent::Summary() const
{ FString S; for (EDMRelic R : Inventory.Items) { if (!S.IsEmpty()) { S+=TEXT(" | "); } S+=Name(R); } return S.IsEmpty() ? TEXT("No relics") : S; }
float UDMRelicComponent::Value(EDMRelic R) const
{
    if (!CanAcquire(R)) { return 0; }
    const auto* A = Self(); const auto K = A->Investigator->Kind;
    float Score = 1;
    if (R==EDMRelic::Morphine) { Score+=A->InjuryCount+ .5f*A->GrievousCount; }
    if (R==EDMRelic::Overheal && (K==EDMInvestigator::Medium || K==EDMInvestigator::Photographer)) { Score+=2; }
    if ((R==EDMRelic::Swagger || R==EDMRelic::Knot || R==EDMRelic::Coin) && K==EDMInvestigator::Smuggler) { Score+=2; }
    if (R==EDMRelic::Medal && K==EDMInvestigator::Sapper) { Score+=1; }
    if (R==EDMRelic::Rosary) { Score+=A->MadnessCore->View().Floor/20.f; }
    if (R==EDMRelic::Gloves && A->Health()/FMath::Max(1.f,A->MaxHealth())>.6f) { Score+=1; }
    return Score;
}
void UDMRelicComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UDMRelicComponent,Inventory); }
