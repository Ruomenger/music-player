#pragma once

#include <cstdint>
#include <string>
#include "cover_cache.h"
#include "sqlite_song_repo.h"

namespace musicplayer {

// Orchestrator: directory scan → metadata extract → cover cache → lyric
// match → SqliteSongRepo insert/update. Designed to be called repeatedly
// (re-scans) — the same `(file_path, file_size, file_mtime)` triple acts as
// a fast skip key, so unchanged files don't pay the FFmpeg open cost twice.
struct ImportStats {
    int scanned = 0;
    int added = 0;
    int updated = 0;  // file existed but file_size/mtime changed
    int skipped = 0;  // file existed and was unchanged
    int failed = 0;   // metadata extraction or DB write failed
};

class LibraryImporter {
public:
    // All collaborators are non-owning; the importer never outlives them.
    // coverCache may be nullptr (cover extraction skipped) — handy in tests.
    LibraryImporter(SqliteSongRepo* songRepo, CoverCache* coverCache);

    // Walks the directory and imports each audio file found.
    ImportStats importDirectory(const std::string& root);

    // Imports a single file. Useful for drag-and-drop / file watcher hooks
    // and for unit testing the importer without filesystem scanning.
    ImportStats importFile(const std::string& filePath);

private:
    // Returns the action taken: 'added' / 'updated' / 'skipped' / 'failed'.
    // Centralizes the dedup + metadata + cover + lyric pipeline so both entry
    // points share the same logic.
    enum class Action : std::uint8_t { Added, Updated, Skipped, Failed };
    Action ingestOne(const std::string& filePath);

    SqliteSongRepo* songRepo_;
    CoverCache* coverCache_;
};

}  // namespace musicplayer
