#include "sqlite_song_repo.h"

#include <stdexcept>

namespace musicplayer {

namespace {

[[noreturn]] void notImplemented(const char* method) {
    throw std::logic_error(std::string("SqliteSongRepo::") + method +
                           " is not implemented (Phase 2)");
}

}  // namespace

// Phase 2: stubs that throw. Once a real SQLite handle becomes a member, these
// methods will be non-static; silence "can be made static" until then.
// NOLINTBEGIN(readability-convert-member-functions-to-static)
bool SqliteSongRepo::initialize(const std::string& /*dbPath*/) {
    notImplemented("initialize");
}
std::vector<SongInfo> SqliteSongRepo::getAllSongs() {
    notImplemented("getAllSongs");
}
std::optional<SongInfo> SqliteSongRepo::getSongById(int /*id*/) {
    notImplemented("getSongById");
}
int SqliteSongRepo::insertSong(const SongInfo& /*song*/) {
    notImplemented("insertSong");
}
bool SqliteSongRepo::removeSong(int /*id*/) {
    notImplemented("removeSong");
}
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace musicplayer
