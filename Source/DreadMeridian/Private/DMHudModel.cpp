#include "DMHudModel.h"
#include "DMCombatant.h"
#include "DMGameState.h"
#include "EngineUtils.h"

FDMHudUnit FDMHudModel::Read(const ADMCombatant& Unit, const FString& LocalEntityId)
{
    FDMHudUnit Row;
    Row.Actor = const_cast<ADMCombatant*>(&Unit);
    Row.EntityId = Unit.EntityId;
    Row.Name = Unit.DisplayName();
    Row.Kind = Unit.Investigator->Kind;
    Row.Tint = Unit.Investigator->Color();
    Row.Health = Unit.Health();
    Row.MaxHealth = Unit.MaxHealth();
    Row.Shield = Unit.Shield();
    Row.bEnemy = Unit.bIsEnemy;
    Row.bDown = Unit.IsDown();
    Row.bLocal = !LocalEntityId.IsEmpty() && Unit.EntityId == LocalEntityId;
    Row.Injuries = Unit.InjuryCount;
    Row.Grievous = Unit.GrievousCount;
    Row.Control = Unit.ControlKind;
    Row.Resources = Unit.bIsEnemy ? Unit.Smuggler->Status() : Unit.Investigator->ResourceSummary();
    Row.ReviverId = Unit.ReviverId;
    Row.ReviveProgress = Unit.ReviveProgress;
    Row.bTargetingLocal = !LocalEntityId.IsEmpty() && Unit.AttackTargetId == LocalEntityId;
    Row.Location = Unit.GetActorLocation();
    return Row;
}

FDMHudModel FDMHudModel::Build(UWorld* World, const ADMCombatant* LocalPawn, const ADMCombatant* SelectedTarget)
{
    FDMHudModel Model;
    if (!World) { return Model; }
    if (const ADMGameState* State = World->GetGameState<ADMGameState>())
    {
        const FDMRunState Run = State->GetRunState();
        Model.Phase = Run.Phase;
        Model.RitualStage = Run.RitualStage;
        Model.RitualProgress = Run.RitualProgress;
        Model.CombatTick = State->GetCombatTick();
        Model.EncounterObjective = State->EncounterObjective;
    }
    const FString LocalId = LocalPawn ? LocalPawn->EntityId : FString();

    TArray<ADMCombatant*> Roster;
    for (TActorIterator<ADMCombatant> It(World); It; ++It) { Roster.Add(*It); }
    Roster.Sort([](const ADMCombatant& A, const ADMCombatant& B) { return A.EntityId < B.EntityId; });

    for (const ADMCombatant* Unit : Roster)
    {
        const FDMHudUnit Row = Read(*Unit, LocalId);
        if (Row.bEnemy)
        {
            ++Model.EnemyCount;
            Model.EnemiesStanding += Row.bDown ? 0 : 1;
            Model.Enemies.Add(Row);
        }
        else
        {
            ++Model.InvestigatorCount;
            Model.InvestigatorsStanding += Row.bDown ? 0 : 1;
            if (Row.bLocal) { Model.Self = Row; Model.bHasSelf = true; }
            else { Model.Allies.Add(Row); }
        }
        if (SelectedTarget && Unit == SelectedTarget) { Model.Target = Row; Model.bHasTarget = true; }
    }

    if (LocalPawn && LocalPawn->AttackIntervalTicks > 0)
    {
        const int32 Remaining = LocalPawn->NextAttackTick - Model.CombatTick;
        Model.BasicCooldown = FMath::Clamp(static_cast<float>(Remaining) / LocalPawn->AttackIntervalTicks, 0.f, 1.f);
        Model.BasicCooldownSeconds = FMath::Max(0, Remaining) * CombatTickSeconds;
    }

    // Live pings from the replicated board; author tint and target position come from the roster.
    if (const ADMGameState* State = World->GetGameState<ADMGameState>())
    {
        auto Find = [&Roster](const FString& Id) -> const ADMCombatant*
        {
            if (Id.IsEmpty()) { return nullptr; }
            for (const ADMCombatant* Unit : Roster) { if (Unit->EntityId == Id) { return Unit; } }
            return nullptr;
        };
        for (const FDMPing& Ping : State->Pings)
        {
            if (!Ping.IsLive(Model.CombatTick)) { continue; }
            FDMHudPing Row;
            Row.Id = Ping.Id;
            Row.Kind = Ping.Kind;
            Row.Label = PingLabel(Ping.Kind);
            Row.AuthorId = Ping.AuthorId;
            Row.bAuthorBot = Ping.bAuthorBot;
            Row.bSubjective = Ping.bSubjective;
            Row.bMine = !LocalId.IsEmpty() && Ping.AuthorId == LocalId;
            if (const ADMCombatant* Author = Find(Ping.AuthorId)) { Row.AuthorName = Author->DisplayName(); Row.Tint = Author->Investigator->Color(); }
            else { Row.AuthorName = Ping.AuthorId; Row.Tint = FLinearColor::White; }
            const ADMCombatant* Target = Find(Ping.TargetId);
            Row.Location = Target ? Target->GetActorLocation() : Ping.Location;
            Row.Age = Ping.AgeFraction(Model.CombatTick);
            Row.OnIt = Ping.OnIt.Num();
            Row.Busy = Ping.Busy.Num();
            Row.Acknowledged = Ping.Acknowledged.Num();
            Model.Pings.Add(Row);
        }
    }
    return Model;
}

FString FDMHudModel::PingLabel(EDMPingKind Kind)
{
    switch (Kind)
    {
    case EDMPingKind::Enemy: return TEXT("ENEMY");
    case EDMPingKind::Focus: return TEXT("FOCUS");
    case EDMPingKind::Ignore: return TEXT("IGNORE");
    case EDMPingKind::GoHere: return TEXT("GO");
    case EDMPingKind::Defend: return TEXT("DEFEND");
    case EDMPingKind::Retreat: return TEXT("RETREAT");
    case EDMPingKind::Help: return TEXT("HELP");
    case EDMPingKind::Pickup: return TEXT("PICKUP");
    case EDMPingKind::Perceive: return TEXT("?");
    default: return TEXT("");
    }
}
