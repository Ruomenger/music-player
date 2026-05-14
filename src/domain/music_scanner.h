#pragma once

#include <string>
#include <vector>

namespace musicplayer {

// Recursively walks a directory and returns the absolute paths of every file
// whose extension matches the audio extension list. Pure std::filesystem —
// no Qt, no FFmpeg, easy to unit test. Permission / symlink-loop errors on
// individual entries are swallowed (the iterator skips them) so a broken
// child does not abort the entire scan.
class MusicScanner {
public:
    [[nodiscard]] static std::vector<std::string> scan(const std::string& root);
};

}  // namespace musicplayer
