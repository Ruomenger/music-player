#include <gtest/gtest.h>
#include "play_queue.h"
#include "song_info.h"

#include <set>
#include <vector>

using musicplayer::PlayMode;
using musicplayer::PlayQueue;

namespace {

PlayQueue makeQueue(std::vector<int> ids, PlayMode mode = PlayMode::Sequential) {
    PlayQueue q;
    q.setMode(mode);
    q.setSongs(std::move(ids));
    return q;
}

}  // namespace

// ─── Sequential ──────────────────────────────────────────────────────────────

TEST(PlayQueueSequential, AdvanceNaturalWalksToEndThenStops) {
    auto q = makeQueue({10, 20, 30});
    EXPECT_EQ(q.current(), 10);
    EXPECT_EQ(q.advanceNatural(), 20);
    EXPECT_EQ(q.advanceNatural(), 30);
    EXPECT_FALSE(q.advanceNatural().has_value());
    EXPECT_EQ(q.current(), 30);
}

TEST(PlayQueueSequential, ManualNextMatchesNatural) {
    auto q = makeQueue({1, 2, 3});
    EXPECT_EQ(q.advanceManual(), 2);
    EXPECT_EQ(q.advanceManual(), 3);
    EXPECT_FALSE(q.advanceManual().has_value());
}

TEST(PlayQueueSequential, ManualPrevWalksBackToFirstThenStops) {
    auto q = makeQueue({1, 2, 3});
    EXPECT_EQ(q.advanceManual(), 2);
    EXPECT_EQ(q.advanceManual(), 3);
    EXPECT_EQ(q.retreatManual(), 2);
    EXPECT_EQ(q.retreatManual(), 1);
    EXPECT_FALSE(q.retreatManual().has_value());
}

// ─── Single ──────────────────────────────────────────────────────────────────

TEST(PlayQueueSingle, NaturalEndReplaysCurrent) {
    auto q = makeQueue({1, 2, 3}, PlayMode::Single);
    EXPECT_EQ(q.advanceNatural(), 1);
    EXPECT_EQ(q.current(), 1);
}

TEST(PlayQueueSingle, ManualNextSkipsForward) {
    auto q = makeQueue({1, 2, 3}, PlayMode::Single);
    EXPECT_EQ(q.advanceManual(), 2);
    EXPECT_EQ(q.current(), 2);
}

// ─── ListLoop ────────────────────────────────────────────────────────────────

TEST(PlayQueueListLoop, WrapsAroundOnNaturalEnd) {
    auto q = makeQueue({1, 2, 3}, PlayMode::ListLoop);
    EXPECT_EQ(q.advanceNatural(), 2);
    EXPECT_EQ(q.advanceNatural(), 3);
    EXPECT_EQ(q.advanceNatural(), 1);  // wraps
}

TEST(PlayQueueListLoop, ManualPrevWrapsBackwards) {
    auto q = makeQueue({1, 2, 3}, PlayMode::ListLoop);
    EXPECT_EQ(q.retreatManual(), 3);  // wraps from index 0 to last
    EXPECT_EQ(q.retreatManual(), 2);
}

// ─── Random ──────────────────────────────────────────────────────────────────

TEST(PlayQueueRandom, ShuffleCoversAllSongsExactlyOnce) {
    auto q = makeQueue({10, 20, 30, 40, 50}, PlayMode::Random);
    q.seedForTesting(42);
    std::set<int> visited;
    visited.insert(*q.current());
    for (int i = 0; i < 4; ++i)
        visited.insert(*q.advanceManual());
    EXPECT_EQ(visited.size(), 5U);
    EXPECT_TRUE(visited.contains(10));
    EXPECT_TRUE(visited.contains(50));
}

TEST(PlayQueueRandom, ReshufflesAfterEnd) {
    auto q = makeQueue({1, 2, 3}, PlayMode::Random);
    q.seedForTesting(7);
    // Walk to the end of the first shuffle and then one more — that triggers
    // a reshuffle. We can't predict the exact id, just that we still get one.
    (void)q.advanceManual();
    (void)q.advanceManual();
    auto afterReshuffle = q.advanceManual();
    EXPECT_TRUE(afterReshuffle.has_value());
}

TEST(PlayQueueRandom, ManualPrevWalksBackThroughShuffleHistory) {
    auto q = makeQueue({1, 2, 3, 4, 5}, PlayMode::Random);
    q.seedForTesting(123);
    const int first = *q.current();
    const int second = *q.advanceManual();
    const int third = *q.advanceManual();
    EXPECT_NE(first, second);
    EXPECT_NE(second, third);
    EXPECT_EQ(q.retreatManual(), second);
    EXPECT_EQ(q.retreatManual(), first);
    EXPECT_FALSE(q.retreatManual().has_value());
}

TEST(PlayQueueRandom, SwitchingFromSequentialPreservesCurrent) {
    auto q = makeQueue({1, 2, 3, 4, 5});
    (void)q.advanceManual();  // now on 2
    ASSERT_EQ(q.current(), 2);
    q.seedForTesting(99);
    q.setMode(PlayMode::Random);
    EXPECT_EQ(q.current(), 2);  // cursor inside the new shuffle still points at 2
}

// ─── Mutation invalidation ───────────────────────────────────────────────────

TEST(PlayQueueMutation, SetSongsResetsCursor) {
    auto q = makeQueue({1, 2, 3});
    (void)q.advanceManual();
    EXPECT_EQ(q.current(), 2);
    q.setSongs({100, 200});
    EXPECT_EQ(q.current(), 100);
}

TEST(PlayQueueMutation, SetCurrentJumps) {
    auto q = makeQueue({10, 20, 30, 40});
    EXPECT_TRUE(q.setCurrent(30));
    EXPECT_EQ(q.current(), 30);
    EXPECT_FALSE(q.setCurrent(999));  // not in queue
}

TEST(PlayQueueMutation, EmptyQueueReturnsNullopt) {
    PlayQueue q;
    EXPECT_FALSE(q.current().has_value());
    EXPECT_FALSE(q.advanceManual().has_value());
    EXPECT_FALSE(q.advanceNatural().has_value());
    EXPECT_FALSE(q.retreatManual().has_value());
}
