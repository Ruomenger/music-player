#pragma once

#include <string>
#include <vector>

namespace musicplayer {

struct LyricLine {
    double timestamp;
    std::string text;
};

class LyricParser {
public:
    static std::vector<LyricLine> parse(const std::string& content);
};

} // namespace musicplayer
