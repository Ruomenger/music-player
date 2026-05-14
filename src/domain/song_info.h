#pragma once

#include <cstdint>
#include <string>

namespace musicplayer {

// Logical view of a song. Fields are filled progressively:
// - Phase 2 repos persist title / artist / album / file_path / duration /
//   format / lyric_path / hasLyric.
// - Phase 3 scanner adds the rest (album_artist, year, track/disc number,
//   file_size, file_mtime, bitrate, sample_rate, channels, cover_path).
// Defaults match the SQL schema in docs/database-schema.md so a default-
// constructed SongInfo round-trips through insert/select unchanged.
struct SongInfo {
    int id = 0;
    std::string title;
    std::string artist;
    std::string album;
    std::string albumArtist;
    std::string composer;
    std::string genre;
    int year = 0;
    int trackNumber = 0;
    int discNumber = 0;
    std::string filePath;
    std::int64_t fileSize = 0;
    std::int64_t fileMtime = 0;
    double duration = 0.0;
    std::string format;
    int bitrate = 0;
    int sampleRate = 0;
    int channels = 2;
    std::string coverPath;
    std::string lyricPath;
    bool hasLyric = false;
};

enum class PlayMode : std::uint8_t { Single, Sequential, ListLoop, Random };

}  // namespace musicplayer
