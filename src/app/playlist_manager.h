#pragma once

#include <QObject>
#include <QString>

#include <optional>
#include <vector>

#include "playlist.h"
#include "song_info.h"
#include "sqlite_playlist_repo.h"

namespace musicplayer {

// Application-layer wrapper over SqlitePlaylistRepo. Adds:
// - Qt signals so widgets can react to mutations without polling.
// - "我喜欢" system playlist bootstrap (created on first use).
// - QObject lifetime → owned by main window / controller.
//
// The repo pointer is non-owning; the manager is constructed after the repo
// and dies before it.
class PlaylistManager : public QObject {
    Q_OBJECT
public:
    explicit PlaylistManager(SqlitePlaylistRepo* repo, QObject* parent = nullptr);

    // Ensures the system playlist '我喜欢' exists; returns its id. Idempotent
    // — call on startup, the second call finds the existing row.
    int ensureFavoritesPlaylist();

    int createPlaylist(const QString& name, const QString& description = {});
    bool renamePlaylist(int playlistId, const QString& newName);
    bool deletePlaylist(int playlistId);

    [[nodiscard]] std::optional<Playlist> playlistById(int playlistId) const;
    [[nodiscard]] std::vector<Playlist> allPlaylists() const;

    bool addSong(int playlistId, int songId);
    bool removeSong(int playlistId, int songId);
    bool reorderSongs(int playlistId, const std::vector<int>& songIdsInOrder);

    [[nodiscard]] std::vector<SongInfo> songsIn(int playlistId) const;

    // Favorites are stored as ordinary playlist_songs rows against the
    // system playlist created by ensureFavoritesPlaylist(). These helpers
    // wrap the (lookup-id, add/remove) dance so call sites don't have to
    // know the playlist's row id.
    bool toggleFavorite(int songId);
    [[nodiscard]] bool isFavorite(int songId) const;

signals:
    void playlistCreated(int playlistId);
    void playlistRenamed(int playlistId);
    void playlistDeleted(int playlistId);
    void songsChanged(int playlistId);  // fired for add / remove / reorder
    void favoriteChanged(int songId, bool isFavorite);

private:
    [[nodiscard]] int favoritesPlaylistId() const;

    SqlitePlaylistRepo* repo_;
    mutable int favoritesId_ = 0;  // cached after first ensureFavoritesPlaylist call
};

}  // namespace musicplayer
