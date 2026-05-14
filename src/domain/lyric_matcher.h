#pragma once

#include <string>

namespace musicplayer {

// Sidecar lyric resolution: given "/music/song.mp3" return "/music/song.lrc"
// if the file exists, empty string otherwise. The .lrc lookup tries the exact
// case first; on case-sensitive filesystems (Linux), the canonical lowercase
// extension is also tried before giving up.
class LyricMatcher {
public:
    [[nodiscard]] static std::string findSidecarLrc(const std::string& songPath);
};

}  // namespace musicplayer
