#pragma once

#include <cstdint>
#include <string>

namespace musicplayer {

struct SongInfo {
    int id = 0;
    std::string title;
    std::string artist;
    std::string album;
    std::string filePath;
    double duration = 0.0;
    std::string format;
    std::string lyricPath;
    bool hasLyric = false;
};

enum class PlayMode { Single, Sequential, ListLoop, Random };

}  // namespace musicplayer
