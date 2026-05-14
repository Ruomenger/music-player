#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace musicplayer {

// Audio file extensions the library recognizes during a scan. The list is the
// product of what FFmpeg can decode and what README.md advertises (mp3 / flac
// / wav / aac / ogg / opus / m4a / ape / wma). All entries are lower-case;
// callers should fold case before comparing.
inline constexpr std::array<std::string_view, 11> kAudioExtensions = {
    ".mp3", ".flac", ".wav", ".aac", ".ogg", ".opus", ".m4a", ".ape", ".wma", ".alac", ".aiff",
};

inline bool isAudioExtension(std::string_view ext) {
    std::string lowered;
    lowered.reserve(ext.size());
    for (char c : ext)
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return std::ranges::any_of(kAudioExtensions,
                               [&](std::string_view candidate) { return candidate == lowered; });
}

}  // namespace musicplayer
