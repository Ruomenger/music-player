#include "lyric_matcher.h"

#include <array>
#include <filesystem>
#include <string_view>
#include <system_error>

namespace musicplayer {

namespace {

// Try a small set of common-case variants before giving up. macOS / Windows
// are case-insensitive at the filesystem layer so any of these will match;
// Linux is case-sensitive so we have to enumerate.
constexpr std::array<std::string_view, 3> kLrcExtensions = {".lrc", ".LRC", ".Lrc"};

}  // namespace

std::string LyricMatcher::findSidecarLrc(const std::string& songPath) {
    std::error_code ec;
    std::filesystem::path base(songPath);
    base.replace_extension();  // strip the song's own extension
    for (auto ext : kLrcExtensions) {
        auto candidate = base;
        candidate += std::filesystem::path(ext);
        if (std::filesystem::exists(candidate, ec) && !ec)
            return candidate.string();
    }
    return {};
}

}  // namespace musicplayer
