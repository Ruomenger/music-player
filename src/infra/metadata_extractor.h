#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace musicplayer {

// FFmpeg-driven metadata reader. `extract()` opens the container, walks the
// av_dict tags (per-stream first, falling back to format-level), reads the
// first audio stream's codec parameters, and grabs any embedded cover bytes
// (the AV_DISPOSITION_ATTACHED_PIC video stream). Returns nullopt if the file
// is not openable or has no audio stream.
struct ExtractedMetadata {
    std::string title;
    std::string artist;
    std::string album;
    std::string albumArtist;
    std::string composer;
    std::string genre;
    int year = 0;
    int trackNumber = 0;
    int discNumber = 0;
    double duration = 0.0;  // seconds
    std::string format;
    int bitrate = 0;  // kbps
    int sampleRate = 0;
    int channels = 0;
    std::vector<std::byte> coverData;  // empty if no embedded cover
    std::string coverMime;             // "image/jpeg" / "image/png"; empty if no cover
};

class MetadataExtractor {
public:
    [[nodiscard]] static std::optional<ExtractedMetadata> extract(const std::string& filePath);
};

}  // namespace musicplayer
