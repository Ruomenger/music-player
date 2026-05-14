#include "playlist_manager.h"

#include <algorithm>

namespace musicplayer {

namespace {

constexpr const char* kFavoritesName = "我喜欢";

}  // namespace

PlaylistManager::PlaylistManager(SqlitePlaylistRepo* repo, QObject* parent)
    : QObject(parent), repo_(repo) {}

int PlaylistManager::ensureFavoritesPlaylist() {
    const auto existing = repo_->getAllPlaylists();
    const auto it = std::ranges::find_if(
        existing, [](const Playlist& p) { return p.isSystem && p.name == kFavoritesName; });
    if (it != existing.end())
        return it->id;
    const int id = repo_->createPlaylist(kFavoritesName, /*description=*/"", /*isSystem=*/true);
    if (id > 0)
        emit playlistCreated(id);
    return id;
}

int PlaylistManager::createPlaylist(const QString& name, const QString& description) {
    const int id = repo_->createPlaylist(name.toStdString(), description.toStdString());
    if (id > 0)
        emit playlistCreated(id);
    return id;
}

bool PlaylistManager::renamePlaylist(int playlistId, const QString& newName) {
    if (!repo_->renamePlaylist(playlistId, newName.toStdString()))
        return false;
    emit playlistRenamed(playlistId);
    return true;
}

bool PlaylistManager::deletePlaylist(int playlistId) {
    if (!repo_->deletePlaylist(playlistId))
        return false;
    emit playlistDeleted(playlistId);
    return true;
}

std::optional<Playlist> PlaylistManager::playlistById(int playlistId) const {
    return repo_->getPlaylistById(playlistId);
}

std::vector<Playlist> PlaylistManager::allPlaylists() const {
    return repo_->getAllPlaylists();
}

bool PlaylistManager::addSong(int playlistId, int songId) {
    if (!repo_->addSongToPlaylist(playlistId, songId))
        return false;
    emit songsChanged(playlistId);
    return true;
}

bool PlaylistManager::removeSong(int playlistId, int songId) {
    if (!repo_->removeSongFromPlaylist(playlistId, songId))
        return false;
    emit songsChanged(playlistId);
    return true;
}

bool PlaylistManager::reorderSongs(int playlistId, const std::vector<int>& songIdsInOrder) {
    if (!repo_->reorderSongs(playlistId, songIdsInOrder))
        return false;
    emit songsChanged(playlistId);
    return true;
}

std::vector<SongInfo> PlaylistManager::songsIn(int playlistId) const {
    return repo_->getSongsInPlaylist(playlistId);
}

}  // namespace musicplayer
