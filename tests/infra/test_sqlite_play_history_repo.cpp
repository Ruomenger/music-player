#include <gtest/gtest.h>
#include "song_info.h"
#include "sqlite_database.h"
#include "sqlite_play_history_repo.h"
#include "sqlite_song_repo.h"

#include <atomic>
#include <memory>
#include <string>

using musicplayer::SongInfo;
using musicplayer::SqliteDatabase;
using musicplayer::SqlitePlayHistoryRepo;
using musicplayer::SqliteSongRepo;

namespace {

std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

class SqlitePlayHistoryRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        connName_ = uniqueConn("play_history");
        db_ = std::make_unique<SqliteDatabase>(connName_);
        ASSERT_TRUE(db_->open(":memory:"));
        songs_ = std::make_unique<SqliteSongRepo>(connName_);
        repo_ = std::make_unique<SqlitePlayHistoryRepo>(connName_);
    }

    void TearDown() override {
        repo_.reset();
        songs_.reset();
        db_.reset();
    }

    int insertSong(const std::string& path) {
        SongInfo s;
        s.title = "T";
        s.filePath = path;
        s.duration = 200.0;
        s.format = "mp3";
        return songs_->insertSong(s);
    }

    std::string connName_;
    std::unique_ptr<SqliteDatabase> db_;
    std::unique_ptr<SqliteSongRepo> songs_;
    std::unique_ptr<SqlitePlayHistoryRepo> repo_;
};

}  // namespace

TEST_F(SqlitePlayHistoryRepoTest, RecordAssignsIdAndIncrementsRowCount) {
    const int songId = insertSong("/m/a.mp3");
    ASSERT_GT(songId, 0);
    const int id = repo_->record(songId, 42.5);
    EXPECT_GT(id, 0);
    auto rows = repo_->recent(10);
    ASSERT_EQ(rows.size(), 1U);
    EXPECT_DOUBLE_EQ(rows[0].progress, 42.5);
    EXPECT_EQ(rows[0].song.id, songId);
}

TEST_F(SqlitePlayHistoryRepoTest, RecentReturnsNewestFirst) {
    const int s1 = insertSong("/m/1.mp3");
    const int s2 = insertSong("/m/2.mp3");
    const int s3 = insertSong("/m/3.mp3");
    ASSERT_GT(repo_->record(s1, 1.0), 0);
    ASSERT_GT(repo_->record(s2, 1.0), 0);
    ASSERT_GT(repo_->record(s3, 1.0), 0);
    auto rows = repo_->recent(10);
    ASSERT_EQ(rows.size(), 3U);
    // Ordering relies on history_id as the secondary key since played_at
    // resolution is seconds — three INSERTs in the same second tie on time
    // but disambiguate by id DESC.
    EXPECT_EQ(rows[0].song.id, s3);
    EXPECT_EQ(rows[1].song.id, s2);
    EXPECT_EQ(rows[2].song.id, s1);
}

TEST_F(SqlitePlayHistoryRepoTest, RecentRespectsLimit) {
    const int songId = insertSong("/m/x.mp3");
    for (int i = 0; i < 5; ++i)
        repo_->record(songId, static_cast<double>(i));
    EXPECT_EQ(repo_->recent(2).size(), 2U);
    EXPECT_EQ(repo_->recent(10).size(), 5U);
    EXPECT_TRUE(repo_->recent(0).empty());
}

TEST_F(SqlitePlayHistoryRepoTest, PurgeOlderThanZeroWipesAll) {
    const int songId = insertSong("/m/y.mp3");
    repo_->record(songId, 1.0);
    repo_->record(songId, 2.0);
    EXPECT_EQ(repo_->purgeOlderThan(0), 2);
    EXPECT_TRUE(repo_->recent(10).empty());
}

TEST_F(SqlitePlayHistoryRepoTest, PurgeOlderThanFutureKeepsAll) {
    const int songId = insertSong("/m/z.mp3");
    repo_->record(songId, 1.0);
    EXPECT_EQ(repo_->purgeOlderThan(30), 0);  // all rows are <30 days old
    EXPECT_EQ(repo_->recent(10).size(), 1U);
}

TEST_F(SqlitePlayHistoryRepoTest, DeletingSongCascadesHistory) {
    const int songId = insertSong("/m/del.mp3");
    ASSERT_GT(repo_->record(songId, 1.0), 0);
    ASSERT_TRUE(songs_->removeSong(songId));
    EXPECT_TRUE(repo_->recent(10).empty());
}
