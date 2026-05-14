#include "library_importer.h"

#include "lyric_matcher.h"
#include "metadata_extractor.h"
#include "music_scanner.h"

#include <cstdint>
#include <filesystem>
#include <system_error>
#include <utility>

namespace musicplayer {

namespace {

struct FileStat {
    std::int64_t size = 0;
    std::int64_t mtime = 0;  // seconds since epoch
    bool valid = false;
};

FileStat statFile(const std::string& path) {
    FileStat s;
    std::error_code ec;
    const auto p = std::filesystem::path(path);
    const auto sz = std::filesystem::file_size(p, ec);
    if (ec)
        return s;
    s.size = static_cast<std::int64_t>(sz);
    const auto mt = std::filesystem::last_write_time(p, ec);
    if (ec)
        return s;
    // file_time_type's epoch is implementation-defined, so this number isn't
    // calendar-seconds — but for dedup ("did the file change") we only need a
    // stable integer that differs when the file is touched, which `count()`
    // on the native duration gives us.
    s.mtime = static_cast<std::int64_t>(mt.time_since_epoch().count());
    s.valid = true;
    return s;
}

// Fill in tag/codec fields from FFmpeg, then file-stat fields from std::fs,
// then the cover + lyric paths from their respective resolvers. The output
// already has its file path written before this is called.
SongInfo buildSongRecord(const std::string& filePath, const FileStat& stat,
                         const ExtractedMetadata& md, std::string coverPath,
                         std::string lyricPath) {
    SongInfo s;
    s.filePath = filePath;
    s.title = md.title.empty() ? std::filesystem::path(filePath).stem().string() : md.title;
    s.artist = md.artist;
    s.album = md.album;
    s.albumArtist = md.albumArtist;
    s.composer = md.composer;
    s.genre = md.genre;
    s.year = md.year;
    s.trackNumber = md.trackNumber;
    s.discNumber = md.discNumber;
    s.fileSize = stat.size;
    s.fileMtime = stat.mtime;
    s.duration = md.duration;
    s.format = md.format;
    s.bitrate = md.bitrate;
    s.sampleRate = md.sampleRate;
    s.channels = md.channels;
    s.coverPath = std::move(coverPath);
    s.lyricPath = std::move(lyricPath);
    s.hasLyric = !s.lyricPath.empty();
    return s;
}

}  // namespace

LibraryImporter::LibraryImporter(SqliteSongRepo* songRepo, CoverCache* coverCache)
    : songRepo_(songRepo), coverCache_(coverCache) {}

ImportStats LibraryImporter::importDirectory(const std::string& root) {
    ImportStats stats;
    for (const auto& path : MusicScanner::scan(root)) {
        ++stats.scanned;
        switch (ingestOne(path)) {
            case Action::Added:
                ++stats.added;
                break;
            case Action::Updated:
                ++stats.updated;
                break;
            case Action::Skipped:
                ++stats.skipped;
                break;
            case Action::Failed:
                ++stats.failed;
                break;
        }
    }
    return stats;
}

ImportStats LibraryImporter::importFile(const std::string& filePath) {
    ImportStats stats;
    ++stats.scanned;
    switch (ingestOne(filePath)) {
        case Action::Added:
            ++stats.added;
            break;
        case Action::Updated:
            ++stats.updated;
            break;
        case Action::Skipped:
            ++stats.skipped;
            break;
        case Action::Failed:
            ++stats.failed;
            break;
    }
    return stats;
}

LibraryImporter::Action LibraryImporter::ingestOne(const std::string& filePath) {
    const FileStat stat = statFile(filePath);
    if (!stat.valid)
        return Action::Failed;

    // Fast skip: same path + same size/mtime → file is unchanged, leave the
    // existing row alone. Anything else (size or mtime drift) is treated as a
    // modified file and re-extracted.
    auto existing = songRepo_->getSongByPath(filePath);
    if (existing && existing->fileSize == stat.size && existing->fileMtime == stat.mtime)
        return Action::Skipped;

    auto md = MetadataExtractor::extract(filePath);
    if (!md)
        return Action::Failed;

    std::string coverPath;
    if (coverCache_ && !md->coverData.empty() && !md->coverMime.empty()) {
        coverPath = coverCache_->store(md->coverData.data(), md->coverData.size(), md->coverMime);
    }
    const std::string lyricPath = LyricMatcher::findSidecarLrc(filePath);

    SongInfo record = buildSongRecord(filePath, stat, *md, std::move(coverPath), lyricPath);

    if (existing) {
        record.id = existing->id;
        if (!songRepo_->updateSong(record))
            return Action::Failed;
        return Action::Updated;
    }
    const int newId = songRepo_->insertSong(record);
    if (newId <= 0)
        return Action::Failed;
    return Action::Added;
}

}  // namespace musicplayer
