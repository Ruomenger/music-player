#pragma once

#include <optional>
#include <string>
#include <vector>
#include "playlist.h"
#include "song_info.h"

namespace musicplayer {

// Persistence for `playlists` + `playlist_songs`. Cross-table reordering uses
// a single transaction; addSong picks `max(sort_index) + 1` so the new entry
// always lands at the tail unless reorderSongs is called afterwards.
class SqlitePlaylistRepo {
public:
    explicit SqlitePlaylistRepo(std::string connectionName = "musicplayer");

    // Returns new playlist id on success, -1 on failure.
    int createPlaylist(const std::string& name, const std::string& description = "",
                       bool isSystem = false);

    bool renamePlaylist(int playlistId, const std::string& newName);
    bool deletePlaylist(int playlistId);

    std::optional<Playlist> getPlaylistById(int playlistId);
    std::vector<Playlist> getAllPlaylists();

    // Inserts a (playlist_id, song_id) row at the tail of the sort order.
    // Returns false on UNIQUE-violation (song already in playlist) or other
    // failure.
    bool addSongToPlaylist(int playlistId, int songId);
    bool removeSongFromPlaylist(int playlistId, int songId);

    // Songs in this playlist, ordered by their stored sort_index.
    std::vector<SongInfo> getSongsInPlaylist(int playlistId);

    // Rewrites sort_index for every entry in `playlistId` so that the order
    // matches `songIdsInOrder` (positions 0..N-1). Atomically applied. Returns
    // false if any song id in the list is not a member of the playlist.
    bool reorderSongs(int playlistId, const std::vector<int>& songIdsInOrder);

private:
    std::string connectionName_;
};

}  // namespace musicplayer
