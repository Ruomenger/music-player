#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace musicplayer {

// Content-addressed cache for embedded cover art. Files are named with their
// MD5 hex prefix + an extension picked from the mime type, so identical cover
// bytes across different songs land on the same file (one disk write, many
// references). The store() call is idempotent: re-storing the same bytes is
// a no-op (existing file is left intact) and returns the same path.
class CoverCache {
public:
    explicit CoverCache(std::string cacheDir);

    // Returns the absolute path of the cached file, or "" on failure (missing
    // cache dir, write error, unknown mime / empty data). Caller stores this
    // path in `songs.cover_path`.
    [[nodiscard]] std::string store(const std::byte* data, std::size_t size,
                                    std::string_view mime) const;

    [[nodiscard]] const std::string& cacheDir() const { return cacheDir_; }

private:
    std::string cacheDir_;
};

}  // namespace musicplayer
