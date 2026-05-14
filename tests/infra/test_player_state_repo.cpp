#include <gtest/gtest.h>
#include "player_state_repo.h"
#include "playlist.h"
#include "song_info.h"
#include "sqlite_database.h"
#include "sqlite_playlist_repo.h"
#include "sqlite_song_repo.h"

#include <atomic>
#include <string>

using musicplayer::PlayerState;
using musicplayer::PlayerStateRepo;
using musicplayer::SongInfo;
using musicplayer::SqliteDatabase;
using musicplayer::SqlitePlaylistRepo;
using musicplayer::SqliteSongRepo;

namespace {

std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

class PlayerStateRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        connName_ = uniqueConn("player_state_repo");
        db_ = std::make_unique<SqliteDatabase>(connName_);
        ASSERT_TRUE(db_->open(":memory:"));
        repo_ = std::make_unique<PlayerStateRepo>(connName_);
    }

    void TearDown() override {
        repo_.reset();
        db_.reset();
    }

    std::string connName_;
    std::unique_ptr<SqliteDatabase> db_;
    std::unique_ptr<PlayerStateRepo> repo_;
};

}  // namespace

TEST_F(PlayerStateRepoTest, LoadFromFreshDbReturnsNullopt) {
    EXPECT_FALSE(repo_->load().has_value());
}

TEST_F(PlayerStateRepoTest, SaveAndLoadRoundTrip) {
    SqliteSongRepo songs(connName_);
    SongInfo s;
    s.title = "T";
    s.filePath = "/m/t.mp3";
    s.duration = 200.0;
    s.format = "mp3";
    const int songId = songs.insertSong(s);
    ASSERT_GT(songId, 0);

    SqlitePlaylistRepo playlists(connName_);
    const int plId = playlists.createPlaylist("P");
    ASSERT_GT(plId, 0);

    PlayerState state{.currentSongId = songId, .playlistId = plId, .position = 42.5};
    ASSERT_TRUE(repo_->save(state));
    auto loaded = repo_->load();
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->currentSongId, songId);
    EXPECT_EQ(loaded->playlistId, plId);
    EXPECT_DOUBLE_EQ(loaded->position, 42.5);
}

TEST_F(PlayerStateRepoTest, SaveTwiceUpsertsSingleRow) {
    PlayerState a{.currentSongId = 0, .playlistId = 0, .position = 10.0};
    ASSERT_TRUE(repo_->save(a));
    PlayerState b{.currentSongId = 0, .playlistId = 0, .position = 99.0};
    ASSERT_TRUE(repo_->save(b));
    auto loaded = repo_->load();
    ASSERT_TRUE(loaded.has_value());
    EXPECT_DOUBLE_EQ(loaded->position, 99.0);
}

TEST_F(PlayerStateRepoTest, DeletedSongIdBecomesZeroOnLoad) {
    SqliteSongRepo songs(connName_);
    SongInfo s;
    s.title = "T";
    s.filePath = "/m/t.mp3";
    s.format = "mp3";
    const int songId = songs.insertSong(s);
    ASSERT_GT(songId, 0);

    ASSERT_TRUE(repo_->save({.currentSongId = songId, .playlistId = 0, .position = 5.0}));
    ASSERT_TRUE(songs.removeSong(songId));  // FK ON DELETE SET NULL
    auto loaded = repo_->load();
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->currentSongId, 0);
    EXPECT_DOUBLE_EQ(loaded->position, 5.0);
}
