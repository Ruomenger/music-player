#include <gtest/gtest.h>
#include "song_info.h"
#include "sqlite_database.h"
#include "sqlite_song_repo.h"

#include <atomic>
#include <string>

using musicplayer::SongInfo;
using musicplayer::SqliteDatabase;
using musicplayer::SqliteSongRepo;

namespace {

// Each test gets a unique connection name so a fresh :memory: database is
// allocated per case (QSqlDatabase keys instances by connection name; the
// :memory: store lives inside the connection's underlying sqlite handle).
std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

class SqliteSongRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        connName_ = uniqueConn("song_repo");
        db_ = std::make_unique<SqliteDatabase>(connName_);
        ASSERT_TRUE(db_->open(":memory:"));
    }

    void TearDown() override { db_.reset(); }

    static SongInfo sampleSong(const std::string& path, const std::string& title) {
        SongInfo s;
        s.title = title;
        s.artist = "Artist";
        s.album = "Album";
        s.filePath = path;
        s.duration = 123.5;
        s.format = "mp3";
        s.lyricPath = "";
        s.hasLyric = false;
        return s;
    }

    std::string connName_;
    std::unique_ptr<SqliteDatabase> db_;
};

}  // namespace

TEST_F(SqliteSongRepoTest, InsertAssignsIdAndRoundTrips) {
    SqliteSongRepo repo(connName_);
    const int id = repo.insertSong(sampleSong("/music/a.mp3", "A"));
    ASSERT_GT(id, 0);
    auto loaded = repo.getSongById(id);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->id, id);
    EXPECT_EQ(loaded->title, "A");
    EXPECT_EQ(loaded->artist, "Artist");
    EXPECT_EQ(loaded->filePath, "/music/a.mp3");
    EXPECT_DOUBLE_EQ(loaded->duration, 123.5);
    EXPECT_FALSE(loaded->hasLyric);
}

TEST_F(SqliteSongRepoTest, InsertDuplicatePathFails) {
    SqliteSongRepo repo(connName_);
    ASSERT_GT(repo.insertSong(sampleSong("/music/dup.mp3", "First")), 0);
    EXPECT_EQ(repo.insertSong(sampleSong("/music/dup.mp3", "Second")), -1);
}

TEST_F(SqliteSongRepoTest, HasLyricMapsToLyricSource) {
    SqliteSongRepo repo(connName_);
    SongInfo s = sampleSong("/music/withlrc.mp3", "Title");
    s.hasLyric = true;
    s.lyricPath = "/music/withlrc.lrc";
    const int id = repo.insertSong(s);
    ASSERT_GT(id, 0);
    auto loaded = repo.getSongById(id);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_TRUE(loaded->hasLyric);
    EXPECT_EQ(loaded->lyricPath, "/music/withlrc.lrc");
}

TEST_F(SqliteSongRepoTest, UpdateChangesFieldsButPreservesId) {
    SqliteSongRepo repo(connName_);
    const int id = repo.insertSong(sampleSong("/music/u.mp3", "Old"));
    ASSERT_GT(id, 0);
    SongInfo updated = sampleSong("/music/u.mp3", "New");
    updated.id = id;
    updated.artist = "Updated";
    EXPECT_TRUE(repo.updateSong(updated));
    auto loaded = repo.getSongById(id);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->title, "New");
    EXPECT_EQ(loaded->artist, "Updated");
}

TEST_F(SqliteSongRepoTest, UpdateMissingIdReturnsFalse) {
    SqliteSongRepo repo(connName_);
    SongInfo s = sampleSong("/music/none.mp3", "X");
    s.id = 9999;
    EXPECT_FALSE(repo.updateSong(s));
}

TEST_F(SqliteSongRepoTest, RemoveDeletesRow) {
    SqliteSongRepo repo(connName_);
    const int id = repo.insertSong(sampleSong("/music/r.mp3", "R"));
    ASSERT_GT(id, 0);
    EXPECT_TRUE(repo.removeSong(id));
    EXPECT_FALSE(repo.getSongById(id).has_value());
    EXPECT_FALSE(repo.removeSong(id));  // idempotent second call returns false
}

TEST_F(SqliteSongRepoTest, GetAllSongsReturnsInsertionOrder) {
    SqliteSongRepo repo(connName_);
    const int id1 = repo.insertSong(sampleSong("/music/1.mp3", "One"));
    const int id2 = repo.insertSong(sampleSong("/music/2.mp3", "Two"));
    const int id3 = repo.insertSong(sampleSong("/music/3.mp3", "Three"));
    auto all = repo.getAllSongs();
    ASSERT_EQ(all.size(), 3U);
    EXPECT_EQ(all[0].id, id1);
    EXPECT_EQ(all[1].id, id2);
    EXPECT_EQ(all[2].id, id3);
}

TEST_F(SqliteSongRepoTest, GetByPathFindsExactMatch) {
    SqliteSongRepo repo(connName_);
    const int id = repo.insertSong(sampleSong("/music/path.mp3", "P"));
    ASSERT_GT(id, 0);
    auto byPath = repo.getSongByPath("/music/path.mp3");
    ASSERT_TRUE(byPath.has_value());
    EXPECT_EQ(byPath->id, id);
    EXPECT_FALSE(repo.getSongByPath("/music/missing.mp3").has_value());
}
