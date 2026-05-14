#include <gtest/gtest.h>
#include "playlist_manager.h"
#include "song_info.h"
#include "sqlite_database.h"
#include "sqlite_playlist_repo.h"
#include "sqlite_song_repo.h"

#include <QSignalSpy>

#include <atomic>
#include <memory>
#include <string>

using musicplayer::PlaylistManager;
using musicplayer::SongInfo;
using musicplayer::SqliteDatabase;
using musicplayer::SqlitePlaylistRepo;
using musicplayer::SqliteSongRepo;

namespace {

std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

class PlaylistManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        connName_ = uniqueConn("playlist_manager");
        db_ = std::make_unique<SqliteDatabase>(connName_);
        ASSERT_TRUE(db_->open(":memory:"));
        repo_ = std::make_unique<SqlitePlaylistRepo>(connName_);
        songs_ = std::make_unique<SqliteSongRepo>(connName_);
        manager_ = std::make_unique<PlaylistManager>(repo_.get());
    }

    void TearDown() override {
        manager_.reset();
        songs_.reset();
        repo_.reset();
        db_.reset();
    }

    int insertSampleSong(const std::string& path) {
        SongInfo s;
        s.title = "T";
        s.filePath = path;
        s.format = "mp3";
        return songs_->insertSong(s);
    }

    std::string connName_;
    std::unique_ptr<SqliteDatabase> db_;
    std::unique_ptr<SqlitePlaylistRepo> repo_;
    std::unique_ptr<SqliteSongRepo> songs_;
    std::unique_ptr<PlaylistManager> manager_;
};

}  // namespace

TEST_F(PlaylistManagerTest, CreatePlaylistEmitsSignal) {
    QSignalSpy spy(manager_.get(), &PlaylistManager::playlistCreated);
    const int id = manager_->createPlaylist("Mix");
    ASSERT_GT(id, 0);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toInt(), id);
}

TEST_F(PlaylistManagerTest, RenamePlaylistEmitsSignal) {
    const int id = manager_->createPlaylist("Old");
    QSignalSpy spy(manager_.get(), &PlaylistManager::playlistRenamed);
    EXPECT_TRUE(manager_->renamePlaylist(id, "New"));
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toInt(), id);
    EXPECT_EQ(manager_->playlistById(id)->name, "New");
}

TEST_F(PlaylistManagerTest, DeletePlaylistEmitsSignal) {
    const int id = manager_->createPlaylist("Bye");
    QSignalSpy spy(manager_.get(), &PlaylistManager::playlistDeleted);
    EXPECT_TRUE(manager_->deletePlaylist(id));
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(PlaylistManagerTest, AddRemoveReorderEmitSongsChanged) {
    const int pl = manager_->createPlaylist("L");
    const int s1 = insertSampleSong("/m/1.mp3");
    const int s2 = insertSampleSong("/m/2.mp3");
    const int s3 = insertSampleSong("/m/3.mp3");

    QSignalSpy spy(manager_.get(), &PlaylistManager::songsChanged);
    EXPECT_TRUE(manager_->addSong(pl, s1));
    EXPECT_TRUE(manager_->addSong(pl, s2));
    EXPECT_TRUE(manager_->addSong(pl, s3));
    EXPECT_EQ(spy.count(), 3);

    EXPECT_TRUE(manager_->reorderSongs(pl, {s3, s1, s2}));
    EXPECT_EQ(spy.count(), 4);
    auto songs = manager_->songsIn(pl);
    ASSERT_EQ(songs.size(), 3U);
    EXPECT_EQ(songs[0].id, s3);

    EXPECT_TRUE(manager_->removeSong(pl, s1));
    EXPECT_EQ(spy.count(), 5);
}

TEST_F(PlaylistManagerTest, FailedMutationDoesNotEmit) {
    QSignalSpy spy(manager_.get(), &PlaylistManager::playlistDeleted);
    EXPECT_FALSE(manager_->deletePlaylist(9999));  // no such playlist
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(PlaylistManagerTest, EnsureFavoritesIsIdempotent) {
    const int first = manager_->ensureFavoritesPlaylist();
    const int second = manager_->ensureFavoritesPlaylist();
    EXPECT_GT(first, 0);
    EXPECT_EQ(first, second);
    auto p = manager_->playlistById(first);
    ASSERT_TRUE(p.has_value());
    EXPECT_TRUE(p->isSystem);
    EXPECT_EQ(p->name, "我喜欢");
}

TEST_F(PlaylistManagerTest, ToggleFavoriteAddsThenRemoves) {
    manager_->ensureFavoritesPlaylist();
    const int songId = insertSampleSong("/m/heart.mp3");
    ASSERT_GT(songId, 0);

    QSignalSpy favSpy(manager_.get(), &PlaylistManager::favoriteChanged);
    EXPECT_FALSE(manager_->isFavorite(songId));
    EXPECT_TRUE(manager_->toggleFavorite(songId));
    EXPECT_TRUE(manager_->isFavorite(songId));
    EXPECT_TRUE(manager_->toggleFavorite(songId));
    EXPECT_FALSE(manager_->isFavorite(songId));

    ASSERT_EQ(favSpy.count(), 2);
    EXPECT_TRUE(favSpy.at(0).at(1).toBool());   // first toggle → favorite
    EXPECT_FALSE(favSpy.at(1).at(1).toBool());  // second → un-favorite
}

TEST_F(PlaylistManagerTest, ToggleFavoriteWithoutBootstrapFindsExistingRow) {
    // ensureFavoritesPlaylist isn't called explicitly here; toggleFavorite
    // should still find the system row if it exists in the DB.
    manager_->ensureFavoritesPlaylist();
    PlaylistManager fresh(repo_.get());  // never called ensureFavorites itself
    const int songId = insertSampleSong("/m/lazyfav.mp3");
    EXPECT_TRUE(fresh.toggleFavorite(songId));
    EXPECT_TRUE(fresh.isFavorite(songId));
}
