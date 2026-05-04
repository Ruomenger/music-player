#include "sqlite_song_repo.h"

namespace musicplayer {

bool SqliteSongRepo::initialize(const std::string& /*dbPath*/) { return false; }
std::vector<SongInfo> SqliteSongRepo::getAllSongs() { return {}; }
std::optional<SongInfo> SqliteSongRepo::getSongById(int /*id*/) { return std::nullopt; }
int SqliteSongRepo::insertSong(const SongInfo& /*song*/) { return -1; }
bool SqliteSongRepo::removeSong(int /*id*/) { return false; }

} // namespace musicplayer
