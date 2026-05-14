#include "music_scanner.h"

#include "audio_extensions.h"

#include <filesystem>
#include <system_error>

namespace musicplayer {

std::vector<std::string> MusicScanner::scan(const std::string& root) {
    std::vector<std::string> result;
    std::error_code ec;
    const std::filesystem::path rootPath(root);
    if (!std::filesystem::is_directory(rootPath, ec))
        return result;

    // skip_permission_denied lets the iterator swallow EACCES on individual
    // children; we still pass an error_code to catch fatal failures.
    auto iter = std::filesystem::recursive_directory_iterator(
        rootPath, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec)
        return result;

    for (const auto& entry : iter) {
        std::error_code entryEc;
        if (!entry.is_regular_file(entryEc) || entryEc)
            continue;
        if (isAudioExtension(entry.path().extension().string()))
            result.push_back(entry.path().string());
    }
    return result;
}

}  // namespace musicplayer
