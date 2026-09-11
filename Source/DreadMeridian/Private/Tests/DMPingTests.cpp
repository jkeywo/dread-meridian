#include "DMPing.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMPingBoardTest, "DreadMeridian.Foundation.Ping.Board",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FDMPingBoardTest::RunTest(const FString& Parameters)
{
    FDMPingBoard B;
    TArray<FDMPingEnded> Ended;
    const FString Ann(TEXT("ann")), Bob(TEXT("bob")), Bot(TEXT("bot-1")), Enemy1(TEXT("enemy-1")), None(TEXT(""));
    const FVector Loc(100, 0, 0);
    // Target rules and author validation. Rejections must not stamp the author cooldown.
    TestTrue(TEXT("Empty author rejected"), B.Create(EDMPingKind::GoHere, None, false, Loc, None, 0, Ended) == INDEX_NONE);
    for (EDMPingKind Kind : { EDMPingKind::Enemy, EDMPingKind::Focus, EDMPingKind::Ignore, EDMPingKind::Help })
    {
        TestTrue(TEXT("Kind needs a target"), FDMPingBoard::NeedsTarget(Kind) && FDMPingBoard::AllowsTarget(Kind));
        TestTrue(TEXT("Targeted kind without target rejected"), B.Create(Kind, Ann, false, Loc, None, 0, Ended) == INDEX_NONE);
    }
    for (EDMPingKind Kind : { EDMPingKind::GoHere, EDMPingKind::Defend, EDMPingKind::Retreat, EDMPingKind::Pickup, EDMPingKind::Perceive })
    { TestFalse(TEXT("Ground kind needs no target"), FDMPingBoard::NeedsTarget(Kind)); }
    TestFalse(TEXT("Perceive never allows a target"), FDMPingBoard::AllowsTarget(EDMPingKind::Perceive));
    TestTrue(TEXT("Perceive with target rejected"), B.Create(EDMPingKind::Perceive, Ann, false, Loc, Enemy1, 0, Ended) == INDEX_NONE);
    TestTrue(TEXT("Count is not a kind"), B.Create(EDMPingKind::Count, Ann, false, Loc, None, 0, Ended) == INDEX_NONE);
    TestEqual(TEXT("Rejections create nothing"), B.Pings.Num(), 0);
    TestEqual(TEXT("Rejections end nothing"), Ended.Num(), 0);
    // Creation and cooldown.
    const int32 P1 = B.Create(EDMPingKind::Enemy, Ann, false, Loc, Enemy1, 0, Ended);
    TestEqual(TEXT("Rejections consumed no cooldown; first id is 1"), P1, 1);
    const FDMPing* Ping = B.Find(P1);
    TestNotNull(TEXT("Created ping is findable"), Ping);
    TestTrue(TEXT("Enemy ping fields"), Ping && Ping->Kind == EDMPingKind::Enemy && Ping->AuthorId == Ann && !Ping->bAuthorBot
        && Ping->Location == Loc && Ping->TargetId == Enemy1 && Ping->CreatedTick == 0 && Ping->ExpiresTick == 150 && !Ping->bSubjective);
    TestTrue(TEXT("Live before expiry tick"), Ping && Ping->IsLive(149) && !Ping->IsLive(150));
    TestTrue(TEXT("Author cooldown rejects"), B.Create(EDMPingKind::GoHere, Ann, false, Loc, None, 4, Ended) == INDEX_NONE);
    const int32 P2 = B.Create(EDMPingKind::GoHere, Bob, false, Loc, None, 4, Ended);
    TestTrue(TEXT("Cooldown is per author"), P2 != INDEX_NONE);
    const int32 P3 = B.Create(EDMPingKind::GoHere, Ann, false, FVector(200, 0, 0), None, 5, Ended);
    TestTrue(TEXT("Cooldown lifts after AuthorCooldownTicks"), P3 != INDEX_NONE);
    const int32 P4 = B.Create(EDMPingKind::Perceive, Ann, false, Loc, None, 10, Ended);
    Ping = B.Find(P4);
    TestTrue(TEXT("Perceive is subjective, location only, 100 ticks"), Ping && Ping->bSubjective && Ping->TargetId.IsEmpty() && Ping->ExpiresTick == 110);
    TestEqual(TEXT("Three live for Ann"), B.LiveCountFor(Ann), 3);
    TestEqual(TEXT("One live for Bob"), B.LiveCountFor(Bob), 1);
    TestEqual(TEXT("Nothing ended yet"), Ended.Num(), 0);
    // Same-kind replacement.
    const int32 P5 = B.Create(EDMPingKind::GoHere, Ann, false, FVector(300, 0, 0), None, 15, Ended);
    TestTrue(TEXT("Same kind replaces the oldest of that kind by the same author"), P5 != INDEX_NONE && Ended.Num() == 1
        && Ended[0].Ping.Id == P3 && Ended[0].Reason == EDMPingEnd::Replaced);
    TestNull(TEXT("Replaced ping is gone"), B.Find(P3));
    TestEqual(TEXT("Replacement keeps the count"), B.LiveCountFor(Ann), 3);
    Ended.Reset();
    // Cap: beyond MaxPerAuthor the oldest of any kind goes.
    const int32 P6 = B.Create(EDMPingKind::Defend, Ann, false, Loc, None, 20, Ended);
    TestTrue(TEXT("Cap replaces the oldest ping of any kind"), P6 != INDEX_NONE && Ended.Num() == 1
        && Ended[0].Ping.Id == P1 && Ended[0].Reason == EDMPingEnd::Replaced);
    TestTrue(TEXT("Survivors after cap"), !B.Find(P1) && B.Find(P4) && B.Find(P5) && B.Find(P6) && B.Find(P2));
    TestEqual(TEXT("Cap holds at three"), B.LiveCountFor(Ann), FDMPingBoard::MaxPerAuthor);
    TestEqual(TEXT("Other authors untouched"), B.LiveCountFor(Bob), 1);
    Ended.Reset();
    const int32 P7 = B.Create(EDMPingKind::Defend, Bob, false, Loc, Enemy1, 20, Ended);
    Ping = B.Find(P7);
    TestTrue(TEXT("Ground kind accepts a target but carries only a location"), Ping && Ping->TargetId.IsEmpty());
    const int32 Ids[] = { P1, P2, P3, P4, P5, P6, P7 };
    for (int32 I = 1; I < static_cast<int32>(UE_ARRAY_COUNT(Ids)); ++I) { TestTrue(TEXT("Ids increase monotonically"), Ids[I] > Ids[I - 1]); }
    // Cancel only by the author.
    TestFalse(TEXT("Cancel by non-author fails"), B.Cancel(P5, Bob, Ended));
    TestFalse(TEXT("Cancel with empty author fails"), B.Cancel(P5, None, Ended));
    TestTrue(TEXT("Failed cancel changes nothing"), Ended.Num() == 0 && B.Find(P5) != nullptr);
    TestTrue(TEXT("Cancel by author"), B.Cancel(P5, Ann, Ended) && Ended.Num() == 1 && Ended[0].Ping.Id == P5 && Ended[0].Reason == EDMPingEnd::Cancelled);
    TestNull(TEXT("Cancelled ping is gone"), B.Find(P5));
    TestFalse(TEXT("Cancel unknown id fails"), B.Cancel(P5, Ann, Ended));
    TestEqual(TEXT("Cancel frees a slot"), B.LiveCountFor(Ann), 2);
    Ended.Reset();
    // Acknowledge by others only, idempotent.
    TestFalse(TEXT("Author cannot acknowledge own ping"), B.Acknowledge(P6, Ann));
    TestFalse(TEXT("Empty acknowledger rejected"), B.Acknowledge(P6, None));
    TestTrue(TEXT("Others acknowledge"), B.Acknowledge(P6, Bob));
    TestTrue(TEXT("Acknowledge is idempotent"), B.Acknowledge(P6, Bob));
    Ping = B.Find(P6);
    TestTrue(TEXT("Acknowledged once"), Ping && Ping->Acknowledged.Num() == 1 && Ping->Acknowledged[0] == Bob);
    TestFalse(TEXT("Acknowledge unknown id fails"), B.Acknowledge(999, Bob));
    // Respond: busy/on_it once per responder, on_it upgrades busy, never downgrades.
    TestFalse(TEXT("No response yet"), B.HasResponse(P6, Bot));
    TestTrue(TEXT("Busy recorded"), B.Respond(P6, Bot, false));
    TestTrue(TEXT("Busy shows as a response"), B.HasResponse(P6, Bot));
    TestFalse(TEXT("Other responder has no response"), B.HasResponse(P6, TEXT("bot-2")));
    TestFalse(TEXT("Unknown id has no response"), B.HasResponse(999, Bot));
    TestFalse(TEXT("Busy recorded once"), B.Respond(P6, Bot, false));
    TestTrue(TEXT("Busy list"), Ping && Ping->Busy.Num() == 1 && Ping->OnIt.Num() == 0);
    TestTrue(TEXT("On it upgrades busy"), B.Respond(P6, Bot, true));
    TestTrue(TEXT("Upgrade moves the responder"), Ping && Ping->OnIt.Num() == 1 && Ping->OnIt[0] == Bot && Ping->Busy.Num() == 0);
    TestFalse(TEXT("On it recorded once"), B.Respond(P6, Bot, true));
    TestFalse(TEXT("Busy cannot downgrade on it"), B.Respond(P6, Bot, false));
    TestTrue(TEXT("Still on it"), Ping && Ping->OnIt.Num() == 1 && Ping->Busy.Num() == 0 && B.HasResponse(P6, Bot));
    TestFalse(TEXT("Empty responder rejected"), B.Respond(P6, None, true));
    TestFalse(TEXT("Respond unknown id fails"), B.Respond(999, Bot, true));
    // Trace names.
    TestEqual(TEXT("enemy"), FString(FDMPingBoard::KindName(EDMPingKind::Enemy)), TEXT("enemy"));
    TestEqual(TEXT("focus"), FString(FDMPingBoard::KindName(EDMPingKind::Focus)), TEXT("focus"));
    TestEqual(TEXT("ignore"), FString(FDMPingBoard::KindName(EDMPingKind::Ignore)), TEXT("ignore"));
    TestEqual(TEXT("go_here"), FString(FDMPingBoard::KindName(EDMPingKind::GoHere)), TEXT("go_here"));
    TestEqual(TEXT("defend"), FString(FDMPingBoard::KindName(EDMPingKind::Defend)), TEXT("defend"));
    TestEqual(TEXT("retreat"), FString(FDMPingBoard::KindName(EDMPingKind::Retreat)), TEXT("retreat"));
    TestEqual(TEXT("help"), FString(FDMPingBoard::KindName(EDMPingKind::Help)), TEXT("help"));
    TestEqual(TEXT("pickup"), FString(FDMPingBoard::KindName(EDMPingKind::Pickup)), TEXT("pickup"));
    TestEqual(TEXT("perceive"), FString(FDMPingBoard::KindName(EDMPingKind::Perceive)), TEXT("perceive"));
    TestEqual(TEXT("expired"), FString(FDMPingBoard::EndName(EDMPingEnd::Expired)), TEXT("expired"));
    TestEqual(TEXT("fulfilled"), FString(FDMPingBoard::EndName(EDMPingEnd::Fulfilled)), TEXT("fulfilled"));
    TestEqual(TEXT("cancelled"), FString(FDMPingBoard::EndName(EDMPingEnd::Cancelled)), TEXT("cancelled"));
    TestEqual(TEXT("replaced"), FString(FDMPingBoard::EndName(EDMPingEnd::Replaced)), TEXT("replaced"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMPingLifecycleTest, "DreadMeridian.Foundation.Ping.Lifecycle",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FDMPingLifecycleTest::RunTest(const FString& Parameters)
{
    FDMPingBoard B;
    TArray<FDMPingEnded> Ended;
    const FString None(TEXT("")), Enemy1(TEXT("enemy-1"));
    const FVector Loc(50, 50, 0);
    auto Never = [](const FDMPing&) { return false; };
    auto Always = [](const FDMPing&) { return true; };
    // Expiry per kind: one ping of each kind from a distinct author at tick 0.
    struct FLife { EDMPingKind Kind; int32 Ticks; };
    const FLife Table[] = { { EDMPingKind::Enemy, 150 }, { EDMPingKind::Focus, 300 }, { EDMPingKind::Ignore, 300 }, { EDMPingKind::GoHere, 300 },
        { EDMPingKind::Defend, 300 }, { EDMPingKind::Retreat, 200 }, { EDMPingKind::Help, 300 }, { EDMPingKind::Pickup, 300 }, { EDMPingKind::Perceive, 100 } };
    for (int32 I = 0; I < static_cast<int32>(UE_ARRAY_COUNT(Table)); ++I)
    {
        const FLife& Row = Table[I];
        TestEqual(TEXT("Lifetime per kind"), FDMPingBoard::LifetimeTicks(Row.Kind), Row.Ticks);
        const FString Author = FString::Printf(TEXT("author-%d"), I);
        const int32 Id = B.Create(Row.Kind, Author, I % 2 == 1, Loc, FDMPingBoard::NeedsTarget(Row.Kind) ? Enemy1 : None, 0, Ended);
        const FDMPing* Ping = B.Find(Id);
        TestTrue(TEXT("Ping created with kind lifetime"), Ping && Ping->ExpiresTick == Row.Ticks && Ping->bAuthorBot == (I % 2 == 1)
            && Ping->bSubjective == (Row.Kind == EDMPingKind::Perceive) && Ping->AgeFraction(0) == 0.f);
    }
    TestEqual(TEXT("One ping per kind"), B.Pings.Num(), static_cast<int32>(EDMPingKind::Count));
    B.Step(99, Never, Ended);
    TestEqual(TEXT("Nothing expires before the shortest lifetime"), Ended.Num(), 0);
    B.Step(100, Never, Ended);
    TestTrue(TEXT("Perceive expires at 100"), Ended.Num() == 1 && Ended[0].Ping.Kind == EDMPingKind::Perceive && Ended[0].Reason == EDMPingEnd::Expired);
    Ended.Reset();
    B.Step(150, Never, Ended);
    TestTrue(TEXT("Enemy expires at 150"), Ended.Num() == 1 && Ended[0].Ping.Kind == EDMPingKind::Enemy && Ended[0].Reason == EDMPingEnd::Expired);
    Ended.Reset();
    B.Step(200, Never, Ended);
    TestTrue(TEXT("Retreat expires at 200"), Ended.Num() == 1 && Ended[0].Ping.Kind == EDMPingKind::Retreat && Ended[0].Reason == EDMPingEnd::Expired);
    Ended.Reset();
    B.Step(299, Never, Ended);
    TestEqual(TEXT("Six 300-tick kinds still live"), B.Pings.Num(), 6);
    B.Step(300, Never, Ended);
    TestEqual(TEXT("Remaining kinds expire at 300"), Ended.Num(), 6);
    TestEqual(TEXT("Board empty after all expire"), B.Pings.Num(), 0);
    Ended.Reset();
    // Fulfilment through the predicate.
    const int32 EnemyPing = B.Create(EDMPingKind::Enemy, TEXT("ann"), false, Loc, Enemy1, 400, Ended);
    const int32 FocusPing = B.Create(EDMPingKind::Focus, TEXT("bob"), false, Loc, TEXT("enemy-2"), 400, Ended);
    const int32 HelpPing = B.Create(EDMPingKind::Help, TEXT("cara"), false, Loc, TEXT("hero-1"), 400, Ended);
    const int32 PickupPing = B.Create(EDMPingKind::Pickup, TEXT("dan"), false, Loc, None, 400, Ended);
    const int32 GoPing = B.Create(EDMPingKind::GoHere, TEXT("eve"), false, Loc, None, 400, Ended);
    TestTrue(TEXT("Five pings live"), B.Pings.Num() == 5 && Ended.Num() == 0);
    B.Step(401, [&](const FDMPing& P) { return P.TargetId == Enemy1; }, Ended);
    TestTrue(TEXT("Dead target fulfils the Enemy ping only"), Ended.Num() == 1 && Ended[0].Ping.Id == EnemyPing && Ended[0].Reason == EDMPingEnd::Fulfilled);
    TestTrue(TEXT("Others untouched"), B.Find(FocusPing) && B.Find(HelpPing) && B.Find(PickupPing) && B.Find(GoPing));
    Ended.Reset();
    B.Step(402, [](const FDMPing& P) { return P.Kind == EDMPingKind::Pickup; }, Ended);
    TestTrue(TEXT("Collection fulfils the Pickup ping"), Ended.Num() == 1 && Ended[0].Ping.Id == PickupPing && Ended[0].Reason == EDMPingEnd::Fulfilled);
    Ended.Reset();
    const int32 LatePing = B.Create(EDMPingKind::Perceive, TEXT("fay"), true, Loc, None, 402, Ended);
    TestTrue(TEXT("Late Perceive ends at 502"), B.Find(LatePing) && B.Find(LatePing)->ExpiresTick == 502);
    B.Step(502, [](const FDMPing& P) { return P.Kind == EDMPingKind::GoHere; }, Ended);
    TestTrue(TEXT("Expired precede fulfilled"), Ended.Num() == 2 && Ended[0].Ping.Id == LatePing && Ended[0].Reason == EDMPingEnd::Expired
        && Ended[1].Ping.Id == GoPing && Ended[1].Reason == EDMPingEnd::Fulfilled);
    Ended.Reset();
    TestTrue(TEXT("Response recorded before ending"), B.Respond(FocusPing, TEXT("bot-1"), true));
    B.Step(700, Always, Ended);
    TestTrue(TEXT("Expiry wins over fulfilment"), Ended.Num() == 2 && Ended[0].Reason == EDMPingEnd::Expired && Ended[1].Reason == EDMPingEnd::Expired);
    TestTrue(TEXT("Ended ping keeps its responses"), Ended.Num() == 2 && Ended[0].Ping.Id == FocusPing && Ended[0].Ping.OnIt.Num() == 1);
    TestEqual(TEXT("Board empty"), B.Pings.Num(), 0);
    Ended.Reset();
    B.Step(701, Always, Ended);
    TestEqual(TEXT("Empty board steps quietly"), Ended.Num(), 0);
    // Age fraction is clamped and safe for a zero-length window.
    FDMPing Age;
    Age.CreatedTick = 400;
    Age.ExpiresTick = 700;
    TestEqual(TEXT("Age at creation"), Age.AgeFraction(400), 0.f);
    TestEqual(TEXT("Age at midpoint"), Age.AgeFraction(550), .5f);
    TestEqual(TEXT("Age at expiry"), Age.AgeFraction(700), 1.f);
    TestEqual(TEXT("Age clamps past expiry"), Age.AgeFraction(800), 1.f);
    TestEqual(TEXT("Age clamps before creation"), Age.AgeFraction(300), 0.f);
    Age.ExpiresTick = 400;
    TestEqual(TEXT("Zero-length window is fully aged"), Age.AgeFraction(400), 1.f);
    return true;
}
#endif
