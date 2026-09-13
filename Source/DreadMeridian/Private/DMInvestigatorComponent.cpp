#include "DMInvestigatorComponent.h"
#include "DMCombatant.h"
#include "DMRelicComponent.h"
#include "DMCombatGameMode.h"
#include "Dom/JsonObject.h"
#include "Net/UnrealNetwork.h"

UDMInvestigatorComponent::UDMInvestigatorComponent() { SetIsReplicatedByDefault(true); }
bool UDMInvestigatorComponent::Authority() const { return GetOwner() && GetOwner()->HasAuthority(); }
void UDMInvestigatorComponent::Initialize(EDMInvestigator Value)
{
    if (!Authority()) { return; }
    Kind = Value; Charges = Kind == EDMInvestigator::Sapper ? 2 : 0;
    Momentum = 0; Components = 0; Combo = 0; Exposure.Reset(); Spirits.Reset(); SpiritTargets.Reset();
    CastChecked<ADMCombatant>(GetOwner())->MadnessCore->Reset();
    Madness = 0; MomentumFloor = 0; bExposureFrozen = false;
    LastPressureTick = -100; LastTarget.Reset();
}
FString UDMInvestigatorComponent::DisplayName() const
{
    switch (Kind) {
    case EDMInvestigator::Sapper: return TEXT("Trench Raider / Sapper");
    case EDMInvestigator::Photographer: return TEXT("Expedition Photographer");
    case EDMInvestigator::Medium: return TEXT("Stage Medium");
    case EDMInvestigator::Smuggler: return TEXT("Bare-Knuckle Smuggler");
    default: return TEXT("Human Raider"); }
}
FString UDMInvestigatorComponent::PassiveName() const
{
    switch (Kind) {
    case EDMInvestigator::Sapper: return TEXT("Scrounger");
    case EDMInvestigator::Photographer: return TEXT("Perfect Moment");
    case EDMInvestigator::Medium: return TEXT("Thin Places");
    case EDMInvestigator::Smuggler: return TEXT("Keep Your Feet");
    default: return TEXT(""); }
}
FLinearColor UDMInvestigatorComponent::Color() const
{
    return ColorFor(Kind);
}
FLinearColor UDMInvestigatorComponent::ColorFor(EDMInvestigator Value)
{
    switch (Value) {
    case EDMInvestigator::Sapper: return FLinearColor(1, .55f, .12f);
    case EDMInvestigator::Photographer: return FLinearColor(.3f, .8f, 1);
    case EDMInvestigator::Medium: return FLinearColor(.7f, .35f, 1);
    case EDMInvestigator::Smuggler: return FLinearColor(.25f, 1, .6f);
    default: return FLinearColor(1, .2f, .15f); }
}
float UDMInvestigatorComponent::Range() const
{
    switch (Kind) {
    case EDMInvestigator::Sapper: return 550;
    case EDMInvestigator::Photographer: return 850;
    case EDMInvestigator::Medium: return 500;
    case EDMInvestigator::Smuggler: return 155 + Stickiness() * 50;
    default: return 420; }
}
float UDMInvestigatorComponent::Damage() const
{
    switch (Kind) {
    case EDMInvestigator::Sapper: return 18;
    case EDMInvestigator::Photographer: return 30;
    case EDMInvestigator::Medium: return 12;
    case EDMInvestigator::Smuggler: return 10;
    default: return 7; }
}
int32 UDMInvestigatorComponent::Interval() const
{
    switch (Kind) {
    case EDMInvestigator::Sapper: return 9;
    case EDMInvestigator::Photographer: return 18;
    case EDMInvestigator::Medium: return 11;
    case EDMInvestigator::Smuggler: return 5;
    default: return 10; }
}
float UDMInvestigatorComponent::Resistance() const
{ return Kind == EDMInvestigator::Smuggler ? (Momentum >= 75 ? .6f : Momentum >= 40 ? .35f : Momentum >= 20 ? .15f : 0) : 0; }
float UDMInvestigatorComponent::Stickiness() const
{ return Kind == EDMInvestigator::Smuggler ? Resistance() + FMath::Min(Combo, 2) * .1f : 0; }
void UDMInvestigatorComponent::Pressure(int32 Tick, float Amount)
{
    if (!Authority() || Kind != EDMInvestigator::Smuggler || Amount < 1) { return; }
    Momentum = FMath::Min(100.f, Momentum + FMath::Min(10.f, Amount)*CastChecked<ADMCombatant>(GetOwner())->Relics->ResourceMultiplier()); LastPressureTick = Tick;
}
void UDMInvestigatorComponent::Step(int32 Tick, const FString& EngagedId)
{
    if (!Authority()) { return; }
    if (Tick - LastPressureTick > 30) { Momentum = FMath::Max(0.f, Momentum - 1.f); Combo = 0; LastTarget.Reset(); }
    if (MomentumFloor > 0) { Momentum = FMath::Max(Momentum, FMath::Min(100.f, MomentumFloor)); }
    for (FDMSubjectResource& Subject : Exposure)
    {
        if (!EngagedId.IsEmpty() && Subject.Id == EngagedId) { Subject.LastEngagedTick = Tick; }
        else if (bExposureFrozen) { Subject.LastEngagedTick = Tick; }
        else if (Tick - Subject.LastEngagedTick > 20) { Subject.Value = FMath::Max(0.f, Subject.Value - .5f); }
    }
}
float UDMInvestigatorComponent::PeekExposure(const FString& TargetId) const
{
    for (const auto& Subject : Exposure) { if (Subject.Id == TargetId) { return Subject.Value; } }
    return 0.f;
}
float UDMInvestigatorComponent::ConsumeExposure(const FString& TargetId)
{
    if (!Authority()) { return 0.f; }
    for (auto& Subject : Exposure)
    {
        if (Subject.Id != TargetId) { continue; }
        const float Value = Subject.Value;
        if (!bExposureFrozen) { Subject.Value = 0; }
        return Value;
    }
    return 0.f;
}
void UDMInvestigatorComponent::UpdateSpiritLocationById(const FString& SpiritId, FVector Location)
{
    if (!Authority()) { return; }
    for (auto& Spirit : Spirits) { if (Spirit.Id == SpiritId) { Spirit.Location = Location; } }
}
void UDMInvestigatorComponent::ClearSpiritTarget(const FString& SpiritId)
{
    if (!Authority()) { return; }
    for (int32 I = 0; I < Spirits.Num() && I < SpiritTargets.Num(); ++I)
    { if (Spirits[I].Id == SpiritId) { SpiritTargets[I].Reset(); } }
}
void UDMInvestigatorComponent::SpendAttention(const FString& SpiritId, float Keep)
{
    if (!Authority()) { return; }
    for (auto& Spirit : Spirits) { if (Spirit.Id == SpiritId) { Spirit.Value = FMath::Max(0.f, Spirit.Value * FMath::Clamp(Keep, 0.f, 1.f)); } }
}
float UDMInvestigatorComponent::PeekAttention(const FString& SpiritId) const
{
    for (const auto& Spirit : Spirits) { if (Spirit.Id == SpiritId) { return Spirit.Value; } }
    return 0.f;
}
void UDMInvestigatorComponent::AddMadness(float Amount, const FString& Reason)
{
    if (!Authority() || Kind == EDMInvestigator::None || !FMath::IsFinite(Amount) || Amount <= 0) { return; }
    CastChecked<ADMCombatant>(GetOwner())->MadnessCore->Add(Amount, Reason);
}
bool UDMInvestigatorComponent::OnHit(const FString& TargetId, int32 Tick)
{
    if (!Authority()) { return false; }
    if (Kind == EDMInvestigator::Medium)
    {
        for (int32 I = 0; I < Spirits.Num(); ++I)
        { if (SpiritTargets[I] == TargetId) { Spirits[I].Value = FMath::Min(100.f, Spirits[I].Value + 12*CastChecked<ADMCombatant>(GetOwner())->Relics->ResourceMultiplier()); } }
    }
    if (Kind != EDMInvestigator::Smuggler) { return false; }
    if (LastTarget != TargetId) { Combo = 0; }
    LastTarget = TargetId; ++Combo; Pressure(Tick, 10);
    const bool bFinisher = Combo >= 3;
    if (bFinisher) { Combo = 0; }
    return bFinisher;
}
float UDMInvestigatorComponent::DamageMultiplier(const FString& TargetId, bool bSuppressed) const
{
    if (Kind == EDMInvestigator::Sapper && bSuppressed) { return 1.15f; }
    if (Kind == EDMInvestigator::Photographer)
    { for (const auto& Subject : Exposure) { if (Subject.Id == TargetId) { return 1 + Subject.Value * .01f; } } }
    return 1;
}
bool UDMInvestigatorComponent::CollectComponent()
{
    if (!Authority() || Kind != EDMInvestigator::Sapper || Charges >= ChargeCapacity) { return false; }
    Components+=FMath::RoundToInt(CastChecked<ADMCombatant>(GetOwner())->Relics->ResourceMultiplier());
    if (Components >= 2) { Components -= 2; ++Charges; }
    return true;
}
void UDMInvestigatorComponent::AddExposure(const FString& TargetId, float Amount, bool bVisibleCommitment, int32 Tick)
{
    if (!Authority() || Kind != EDMInvestigator::Photographer || TargetId.IsEmpty() || !FMath::IsFinite(Amount) || Amount <= 0) { return; }
    FDMSubjectResource* Subject = Exposure.FindByPredicate([&](const auto& S) { return S.Id == TargetId; });
    if (!Subject) { Subject = &Exposure.AddDefaulted_GetRef(); Subject->Id = TargetId; }
    Subject->Value = FMath::Min(100.f, Subject->Value + Amount * (bVisibleCommitment ? 1.5f : 1.f)*CastChecked<ADMCombatant>(GetOwner())->Relics->ResourceMultiplier());
    Subject->LastEngagedTick = Tick;
}
void UDMInvestigatorComponent::BindSpirit(const FString& SpiritId, const FString& TargetId, FVector Location)
{
    if (!Authority() || Kind != EDMInvestigator::Medium || SpiritId.IsEmpty() || Spirits.ContainsByPredicate([&](const auto& S) { return S.Id == SpiritId; })) { return; }
    auto& Spirit = Spirits.AddDefaulted_GetRef(); Spirit.Id = SpiritId; Spirit.Location = Location;
    SpiritTargets.Add(TargetId);
}
void UDMInvestigatorComponent::UpdateSpiritLocation(const FString& TargetId, FVector Location)
{
    if (!Authority()) { return; }
    for (int32 I = 0; I < Spirits.Num(); ++I) { if (SpiritTargets[I] == TargetId) { Spirits[I].Location = Location; } }
}
void UDMInvestigatorComponent::ThinPlace(FVector Location, float Strength)
{
    if (!Authority() || Kind != EDMInvestigator::Medium || !FMath::IsFinite(Strength) || Strength <= 0) { return; }
    for (auto& Spirit : Spirits)
    { if (FVector::DistSquared(Location, Spirit.Location) <= FMath::Square(600.f)) { Spirit.Value = FMath::Min(100.f, Spirit.Value + Strength * 1.5f*CastChecked<ADMCombatant>(GetOwner())->Relics->ResourceMultiplier()); } }
}
FString UDMInvestigatorComponent::ResourceSummary(const FString& TargetId) const
{
    FString Result;
    switch (Kind) {
    case EDMInvestigator::Sapper: Result = FString::Printf(TEXT("Charges %d/%d | Components %d/2"), Charges, ChargeCapacity, Components); break;
    case EDMInvestigator::Photographer: {
        Result = TEXT("Target Exposure 0/100 | Q: Frame");
        for (const auto& S : Exposure) { if (S.Id == TargetId) { Result = FString::Printf(TEXT("Target Exposure %.0f/100"), S.Value); break; } }
        break; }
    case EDMInvestigator::Medium: {
        if (Spirits.IsEmpty()) { Result = TEXT("Attention: no bound spirits"); break; }
        Result = TEXT("Attention ");
        for (const auto& S : Spirits) { Result += FString::Printf(TEXT("%s %.0f/100 "), *S.Id, S.Value); }
        break; }
    case EDMInvestigator::Smuggler: Result = FString::Printf(TEXT("Momentum %.0f/100 | Combo %d/3 | Resistance %.0f%%"), Momentum, Combo, Resistance() * 100); break;
    default: return TEXT(""); }
    return Result;
}
void UDMInvestigatorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UDMInvestigatorComponent, Kind); DOREPLIFETIME(UDMInvestigatorComponent, Momentum);
    DOREPLIFETIME(UDMInvestigatorComponent, Charges); DOREPLIFETIME(UDMInvestigatorComponent, Components);
    DOREPLIFETIME(UDMInvestigatorComponent, Combo); DOREPLIFETIME(UDMInvestigatorComponent, Exposure);
    DOREPLIFETIME(UDMInvestigatorComponent, Spirits); DOREPLIFETIME(UDMInvestigatorComponent, SpiritTargets);
}

void UDMInvestigatorComponent::AddAttention(const FString& SpiritId, float Amount)
{
    if (!Authority() || Kind != EDMInvestigator::Medium || !FMath::IsFinite(Amount) || Amount <= 0) { return; }
    for (auto& Spirit : Spirits) { if (Spirit.Id == SpiritId) { Spirit.Value = FMath::Min(100.f, Spirit.Value + Amount*CastChecked<ADMCombatant>(GetOwner())->Relics->ResourceMultiplier()); } }
}
