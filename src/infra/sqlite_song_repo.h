#pragma once

#include <optional>
#include <string>
#include <vector>
#include "song_info.h"

namespace musicplayer {

// Persistence for the `songs` table. The owning SqliteDatabase keeps the
// connection alive; this class just looks it up by name on every call.
//
// Note: Phase 2 only persists the SongInfo subset (title/artist/album/path/
// duration/format/lyric). Scanner-only columns (album_artist, fingerprint,
// file_mtime, ...) accept DDL defaults and will be populated in Phase 3.
class SqliteSongRepo {
public:
    explicit SqliteSongRepo(std::string connectionName = "musicplayer");

    // Returns the new song id on success, -1 on failure (path collision, no
    // open connection, etc.). Logged via qWarning.
    int insertSong(const SongInfo& song);

    bool updateSong(const SongInfo& song);  // matches on song.id
    bool removeSong(int id);

    std::optional<SongInfo> getSongById(int id);
    std::optional<SongInfo> getSongByPath(const std::string& filePath);
    std::vector<SongInfo> getAllSongs();

private:
    std::string connectionName_;
};

}  // namespace musicplayer
