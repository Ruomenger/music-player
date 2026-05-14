#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace musicplayer {

// One LRC timestamped line. Stored as milliseconds (int64) instead of double
// so equality / ordering are exact and 25h-tolerance is comfortable.
struct LyricLine {
    std::int64_t timestampMs = 0;
    std::string text;
};

struct ParsedLyrics {
    std::vector<LyricLine> lines;  // sorted ascending by timestampMs
    // Metadata tags ("ti", "ar", "al", "length", "offset", ...) → value.
    std::unordered_map<std::string, std::string> metadata;
};

// Parses LRC content (the body of a .lrc file). Tolerant: extra whitespace,
// CRLF, blank lines, missing timestamps, trailing junk — all skipped. A
// single line may carry multiple timestamps ("[00:01.00][00:05.00]text"),
// each producing its own LyricLine sharing the same text.
class LyricParser {
public:
    [[nodiscard]] static ParsedLyrics parse(std::string_view content);

    // Find the index of the line whose timestamp is the greatest one ≤ ms.
    // Returns -1 if `lines` is empty or ms precedes the first timestamp.
    // Caller's preferred usage: call as ms advances and compare to a
    // previously-cached index to detect crossings.
    [[nodiscard]] static int findCurrentLineIndex(const std::vector<LyricLine>& lines,
                                                  std::int64_t ms);
};

}  // namespace musicplayer
