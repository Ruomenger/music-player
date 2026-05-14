#include <gtest/gtest.h>
#include "song_info.h"
#include "sqlite_database.h"
#include "sqlite_playlist_repo.h"
#include "sqlite_song_repo.h"

#include <atomic>
#include <string>

using musicplayer::SongInfo;
using musicplayer::SqliteDatabase;
using musicplayer::SqlitePlaylistRepo;
using musicplayer::SqliteSongRepo;

namespace {

std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

class SqlitePlaylistRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        connName_ = uniqueConn("playlist_repo");
        db_ = std::make_unique<SqliteDatabase>(connName_);
        ASSERT_TRUE(db_->open(":memory:"));
        songs_ = std::make_unique<SqliteSongRepo>(connName_);
        playlists_ = std::make_unique<SqlitePlaylistRepo>(connName_);
    }

    void TearDown() override {
        playlists_.reset();
        songs_.reset();
        db_.reset();
    }

    int insertSampleSong(const std::string& path, const std::string& title) {
        SongInfo s;
        s.title = title;
        s.artist = "Artist";
        s.filePath = path;
        s.duration = 100.0;
        s.format = "mp3";
        return songs_->insertSong(s);
    }

    std::string connName_;
    std::unique_ptr<SqliteDatabase> db_;
    std::unique_ptr<SqliteSongRepo> songs_;
    std::unique_ptr<SqlitePlaylistRepo> playlists_;
};

}  // namespace

TEST_F(SqlitePlaylistRepoTest, CreateAndGet) {
    const int id = playlists_->createPlaylist("Favorites", "best of", false);
    ASSERT_GT(id, 0);
    auto p = playlists_->getPlaylistById(id);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->name, "Favorites");
    EXPECT_EQ(p->description, "best of");
    EXPECT_FALSE(p->isSystem);
}

TEST_F(SqlitePlaylistRepoTest, RenameAndDelete) {
    const int id = playlists_->createPlaylist("Old name");
    ASSERT_GT(id, 0);
    EXPECT_TRUE(playlists_->renamePlaylist(id, "New name"));
    auto p = playlists_->getPlaylistById(id);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->name, "New name");
    EXPECT_TRUE(playlists_->deletePlaylist(id));
    EXPECT_FALSE(playlists_->getPlaylistById(id).has_value());
}

TEST_F(SqlitePlaylistRepoTest, GetAllReturnsAllPlaylists) {
    ASSERT_GT(playlists_->createPlaylist("A"), 0);
    ASSERT_GT(playlists_->createPlaylist("B"), 0);
    ASSERT_GT(playlists_->createPlaylist("C"), 0);
    EXPECT_EQ(playlists_->getAllPlaylists().size(), 3U);
}

TEST_F(SqlitePlaylistRepoTest, AddSongToPlaylistAppendsAtTail) {
    const int pl = playlists_->createPlaylist("L");
    const int s1 = insertSampleSong("/m/1.mp3", "One");
    const int s2 = insertSampleSong("/m/2.mp3", "Two");
    const int s3 = insertSampleSong("/m/3.mp3", "Three");
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s1));
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s2));
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s3));
    auto songs = playlists_->getSongsInPlaylist(pl);
    ASSERT_EQ(songs.size(), 3U);
    EXPECT_EQ(songs[0].id, s1);
    EXPECT_EQ(songs[1].id, s2);
    EXPECT_EQ(songs[2].id, s3);
}

TEST_F(SqlitePlaylistRepoTest, AddSameSongTwiceFailsOnUnique) {
    const int pl = playlists_->createPlaylist("L");
    const int s1 = insertSampleSong("/m/dup.mp3", "Dup");
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s1));
    EXPECT_FALSE(playlists_->addSongToPlaylist(pl, s1));
}

TEST_F(SqlitePlaylistRepoTest, RemoveSongFromPlaylist) {
    const int pl = playlists_->createPlaylist("L");
    const int s1 = insertSampleSong("/m/1.mp3", "One");
    const int s2 = insertSampleSong("/m/2.mp3", "Two");
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s1));
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s2));
    EXPECT_TRUE(playlists_->removeSongFromPlaylist(pl, s1));
    auto songs = playlists_->getSongsInPlaylist(pl);
    ASSERT_EQ(songs.size(), 1U);
    EXPECT_EQ(songs[0].id, s2);
}

TEST_F(SqlitePlaylistRepoTest, DeletePlaylistCascadesPlaylistSongs) {
    const int pl = playlists_->createPlaylist("L");
    const int s1 = insertSampleSong("/m/1.mp3", "One");
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s1));
    ASSERT_TRUE(playlists_->deletePlaylist(pl));
    EXPECT_TRUE(playlists_->getSongsInPlaylist(pl).empty());
}

TEST_F(SqlitePlaylistRepoTest, DeleteSongCascadesPlaylistSongs) {
    const int pl = playlists_->createPlaylist("L");
    const int s1 = insertSampleSong("/m/1.mp3", "One");
    const int s2 = insertSampleSong("/m/2.mp3", "Two");
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s1));
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s2));
    ASSERT_TRUE(songs_->removeSong(s1));
    auto songs = playlists_->getSongsInPlaylist(pl);
    ASSERT_EQ(songs.size(), 1U);
    EXPECT_EQ(songs[0].id, s2);
}

TEST_F(SqlitePlaylistRepoTest, ReorderRewritesSortIndex) {
    const int pl = playlists_->createPlaylist("L");
    const int s1 = insertSampleSong("/m/1.mp3", "One");
    const int s2 = insertSampleSong("/m/2.mp3", "Two");
    const int s3 = insertSampleSong("/m/3.mp3", "Three");
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s1));
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s2));
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s3));
    EXPECT_TRUE(playlists_->reorderSongs(pl, {s3, s1, s2}));
    auto songs = playlists_->getSongsInPlaylist(pl);
    ASSERT_EQ(songs.size(), 3U);
    EXPECT_EQ(songs[0].id, s3);
    EXPECT_EQ(songs[1].id, s1);
    EXPECT_EQ(songs[2].id, s2);
}

TEST_F(SqlitePlaylistRepoTest, ReorderRejectsForeignSongAndRollsBack) {
    const int pl = playlists_->createPlaylist("L");
    const int s1 = insertSampleSong("/m/1.mp3", "One");
    const int s2 = insertSampleSong("/m/2.mp3", "Two");
    const int stranger = insertSampleSong("/m/strange.mp3", "Stranger");
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s1));
    ASSERT_TRUE(playlists_->addSongToPlaylist(pl, s2));
    EXPECT_FALSE(playlists_->reorderSongs(pl, {s2, stranger, s1}));
    // Original order preserved.
    auto songs = playlists_->getSongsInPlaylist(pl);
    ASSERT_EQ(songs.size(), 2U);
    EXPECT_EQ(songs[0].id, s1);
    EXPECT_EQ(songs[1].id, s2);
}
