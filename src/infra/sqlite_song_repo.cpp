#include "sqlite_song_repo.h"

namespace musicplayer {

// Phase 2: stubs. Methods will use the SQLite handle once added; for now
// silence "can be made static" since the eventual signature is non-static.
// NOLINTBEGIN(readability-convert-member-functions-to-static)
bool SqliteSongRepo::initialize(const std::string& /*dbPath*/) {
    return false;
}
std::vector<SongInfo> SqliteSongRepo::getAllSongs() {
    return {};
}
std::optional<SongInfo> SqliteSongRepo::getSongById(int /*id*/) {
    return std::nullopt;
}
int SqliteSongRepo::insertSong(const SongInfo& /*song*/) {
    return -1;
}
bool SqliteSongRepo::removeSong(int /*id*/) {
    return false;
}
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace musicplayer
