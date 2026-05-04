#pragma once

#include <optional>
#include <string>
#include <vector>
#include "song_info.h"

namespace musicplayer {

class SqliteSongRepo {
public:
    bool initialize(const std::string& dbPath);
    std::vector<SongInfo> getAllSongs();
    std::optional<SongInfo> getSongById(int id);
    int insertSong(const SongInfo& song);
    bool removeSong(int id);
};

}  // namespace musicplayer
