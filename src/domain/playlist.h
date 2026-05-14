#pragma once

#include <string>

namespace musicplayer {

struct Playlist {
    int id = 0;
    std::string name;
    std::string description;
    std::string coverPath;
    bool isSystem = false;
    int sortOrder = 0;
};

struct PlayerState {
    int currentSongId = 0;  // 0 means "no song"
    int playlistId = 0;     // 0 means "no playlist"
    double position = 0.0;  // seconds
};

}  // namespace musicplayer
