#include "DMShubMinion.h"
#include "DMCombatant.h"
#include "DMCombatGameMode.h"
#include "DMCorpse.h"
#include "DMHealthAttributes.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
UDMShubMinion::UDMShubMinion() { SetIsReplicatedByDefault(true); }
ADMCombatant* UDMShubMinion::Self() const { return CastChecked<ADMCombatant>(GetOwner()); }
void UDMShubMinion::Initialize(EDMShubMinionKind Kind,bool bOffspring)
{
    if (!Self()->HasAuthority() || !Self()->bIsEnemy || Kind!=EDMShubMinionKind::Broodling) { return; }
    State={}; State.Kind=Kind; State.bHealing=bOffspring; State.LastDamage=Self()->LastDamageTick;
    Self()->EncounterLabel=TEXT("Broodling");
    Self()->Tags.AddUnique(TEXT("ShubLinked"));
    if (Kind==EDMShubMinionKind::Broodling) { Self()->Tags.AddUnique(TEXT("Broodling")); }
    Self()->bCommonEnemy=Kind==EDMShubMinionKind::Broodling; Self()->Resolve->Reset();
    if (bOffspring) { Self()->GetAbilitySystemComponent()->SetNumericAttributeBase(UDMHealthAttributes::GetHealthAttribute(),Self()->MaxHealth()*.5f); }
}
bool UDMShubMinion::ControlsMovement() const
{ return State.Kind!=EDMShubMinionKind::None && (State.Action!=EDMShubMinionAction::Hunting || !State.CorpseId.IsEmpty()); }
void UDMShubMinion::Step(int32 Tick)
{
    if (!Self()->HasAuthority() || Self()->IsDown() || State.Kind==EDMShubMinionKind::None || State.Action==EDMShubMinionAction::Retired) { return; }
    if (State.bHealing)
    { if (Self()->LastDamageTick!=State.LastDamage) { State.bHealing=false; Event(TEXT("offspring_healing_interrupted")); }
      else { Self()->HealHealth(1); if (Self()->Health()>=Self()->MaxHealth()) { State.bHealing=false; } } }
    if (State.Kind==EDMShubMinionKind::Broodling) { BroodStep(Tick); } else { GoatStep(Tick); }
}
void UDMShubMinion::BroodStep(int32 Tick)
{
    auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>(); if (!M) { return; }
    if (Self()->IsStunned() || Self()->IsRestrained() || Self()->ControlInterruptSerial!=State.LastInterrupt)
    { State.LastInterrupt=Self()->ControlInterruptSerial; State.Action=EDMShubMinionAction::Hunting; State.CorpseId.Reset(); Self()->SetAttackHold(false); Event(TEXT("interrupted")); return; }
    if (State.Action==EDMShubMinionAction::Splitting)
    {
        if (Tick<State.Until) { return; }
        // Spawn both offspring before retiring the parent. Roster iteration uses a stable copy.
        TArray<ADMCombatant*> Children;
        for (int32 I=0; I<2; ++I)
        {
            auto* Child=M->SpawnEncounterActor(Self()->EntityId+FString::Printf(TEXT(".child.%d"),I),Self()->GetActorLocation()+FVector(0,I==0 ? -70 : 70,0),60,5);
            if (Child) { auto* C=NewObject<UDMShubMinion>(Child); Child->AddInstanceComponent(C); C->RegisterComponent(); C->Initialize(EDMShubMinionKind::Broodling,true); Children.Add(Child); }
        }
        if (Children.Num()!=2) { for (auto* Child : Children) { Child->GetAbilitySystemComponent()->SetNumericAttributeBase(UDMHealthAttributes::GetHealthAttribute(),0); } State.Until=Tick+30; return; }
        State.Action=EDMShubMinionAction::Retired; Self()->StopGoal(); Self()->SetAttackHold(true);
        Self()->GetAbilitySystemComponent()->SetNumericAttributeBase(UDMHealthAttributes::GetHealthAttribute(),0); Self()->SetActorEnableCollision(false); Self()->SetActorHiddenInGame(true);
        Event(TEXT("split")); return; // Replacement is not a kill and does not create another corpse or XP.
    }
    if (State.Feeding>=1)
    { State.Action=EDMShubMinionAction::Splitting; State.Until=Tick+25; Self()->StopGoal(); Self()->SetAttackHold(true); Event(TEXT("split_started")); return; }
    ADMCorpse* Corpse=nullptr; float Best=FMath::Square(900.f);
    for (TActorIterator<ADMCorpse> It(GetWorld()); It; ++It)
    {
        if (It->State.bConsumed || It->State.SourceId.IsEmpty()) { continue; }
        if (It->State.SourceId==State.CorpseId) { Corpse=*It; break; }
        const float D=FVector::DistSquared2D(Self()->GetActorLocation(),It->GetActorLocation());
        if (D<Best || (D==Best && Corpse && It->State.SourceId<Corpse->State.SourceId)) { Corpse=*It; Best=D; }
    }
    if (!Corpse) { State.Action=EDMShubMinionAction::Hunting; State.CorpseId.Reset(); Self()->SetAttackHold(false); return; }
    if (State.CorpseId!=Corpse->State.SourceId) { State.Action=EDMShubMinionAction::Hunting; State.CorpseId=Corpse->State.SourceId; }
    Self()->SetAttackHold(true);
    if (FVector::DistSquared2D(Self()->GetActorLocation(),Corpse->GetActorLocation())>FMath::Square(120.f)) { Self()->MoveToward(Corpse->GetActorLocation()); return; }
    Self()->StopGoal();
    if (State.Action!=EDMShubMinionAction::Feeding) { State.Action=EDMShubMinionAction::Feeding; State.Until=Tick+20; Event(TEXT("feeding_started")); }
    else if (Tick>=State.Until) { State.Feeding+=Corpse->Consume(); State.CorpseId.Reset(); State.Action=EDMShubMinionAction::Hunting; Event(TEXT("fed")); }
}
void UDMShubMinion::GoatStep(int32 Tick) { }
void UDMShubMinion::Event(const FString& Action)
{
    Self()->EncounterLabel=State.Kind==EDMShubMinionKind::Broodling ? TEXT("Broodling") : TEXT("Spawn of the Black Goat");
    if (State.Action==EDMShubMinionAction::Feeding) { Self()->EncounterLabel+=TEXT(" | Feeding - interrupt!"); }
    if (State.Action==EDMShubMinionAction::Splitting) { Self()->EncounterLabel+=TEXT(" | Splitting - interrupt!"); }
    if (auto* M=GetWorld()->GetAuthGameMode<ADMCombatGameMode>()) { auto D=MakeShared<FJsonObject>(); D->SetStringField(TEXT("entity_id"),Self()->EntityId); D->SetStringField(TEXT("action"),Action); M->Emit(TEXT("boss.minion"),D); } Self()->ForceNetUpdate();
}
bool UDMShubMinion::Restore(const FDMShubMinionSnapshot& S)
{ if (!Self()->HasAuthority() || S.Version!=1 || S.Kind>EDMShubMinionKind::Goat || S.Action>EDMShubMinionAction::Retired || !FMath::IsFinite(S.Feeding) || S.Feeding<0 || S.Feeding>2 || S.Until<0 || S.Destination.ContainsNaN()) { return false; } State=S; Self()->SetAttackHold(ControlsMovement()); Self()->ForceNetUpdate(); return true; }
void UDMShubMinion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UDMShubMinion,State); }
