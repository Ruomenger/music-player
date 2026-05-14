#include <gtest/gtest.h>
#include "cover_cache.h"
#include "library_importer.h"
#include "sqlite_database.h"
#include "sqlite_song_repo.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using musicplayer::CoverCache;
using musicplayer::ImportStats;
using musicplayer::LibraryImporter;
using musicplayer::SqliteDatabase;
using musicplayer::SqliteSongRepo;

namespace {

void writeSilentWav(const fs::path& path, double durationSec) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    const int sampleRate = 44100;
    const int channels = 2;
    const int byteRate = sampleRate * channels * 2;
    const auto dataSize = static_cast<int>(durationSec * byteRate);

    out.write("RIFF", 4);
    int32_t chunkSize = 36 + dataSize;
    out.write(reinterpret_cast<const char*>(&chunkSize), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    int32_t fmtSize = 16;
    out.write(reinterpret_cast<const char*>(&fmtSize), 4);
    int16_t audioFormat = 1;
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    auto numChannels = static_cast<int16_t>(channels);
    out.write(reinterpret_cast<const char*>(&numChannels), 2);
    int32_t sr = sampleRate;
    out.write(reinterpret_cast<const char*>(&sr), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    auto blockAlign = static_cast<int16_t>(channels * 2);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    int16_t bitsPerSample = 16;
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);
    std::vector<char> silence(static_cast<std::size_t>(dataSize), 0);
    out.write(silence.data(), static_cast<std::streamsize>(silence.size()));
}

std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

class LibraryImporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        connName_ = uniqueConn("importer");
        db_ = std::make_unique<SqliteDatabase>(connName_);
        ASSERT_TRUE(db_->open(":memory:"));
        songs_ = std::make_unique<SqliteSongRepo>(connName_);

        root_ = fs::temp_directory_path() /
                ("musicplayer_importer_" +
                 std::to_string(static_cast<int>(reinterpret_cast<std::uintptr_t>(this))));
        fs::remove_all(root_);
        fs::create_directories(root_);
        cacheDir_ = root_ / "covers";
        coverCache_ = std::make_unique<CoverCache>(cacheDir_.string());
    }

    void TearDown() override {
        coverCache_.reset();
        songs_.reset();
        db_.reset();
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    std::string connName_;
    std::unique_ptr<SqliteDatabase> db_;
    std::unique_ptr<SqliteSongRepo> songs_;
    std::unique_ptr<CoverCache> coverCache_;
    fs::path root_;
    fs::path cacheDir_;
};

}  // namespace

TEST_F(LibraryImporterTest, ImportsSingleFileAndPopulatesDb) {
    const auto wav = root_ / "track.wav";
    writeSilentWav(wav, 0.3);

    LibraryImporter importer(songs_.get(), coverCache_.get());
    const ImportStats stats = importer.importFile(wav.string());

    EXPECT_EQ(stats.scanned, 1);
    EXPECT_EQ(stats.added, 1);
    EXPECT_EQ(stats.skipped, 0);
    EXPECT_EQ(stats.failed, 0);

    auto rows = songs_->getAllSongs();
    ASSERT_EQ(rows.size(), 1U);
    EXPECT_EQ(rows[0].filePath, wav.string());
    EXPECT_EQ(rows[0].sampleRate, 44100);
    EXPECT_EQ(rows[0].channels, 2);
    EXPECT_GT(rows[0].fileSize, 0);
    EXPECT_NE(rows[0].fileMtime, 0);
    // No tags in the synthesized WAV → title falls back to the stem.
    EXPECT_EQ(rows[0].title, "track");
}

TEST_F(LibraryImporterTest, ScansDirectoryRecursively) {
    writeSilentWav(root_ / "a" / "one.wav", 0.1);
    writeSilentWav(root_ / "b" / "two.wav", 0.1);
    writeSilentWav(root_ / "c" / "three.wav", 0.1);
    // Non-audio file must not crash or count.
    std::ofstream(root_ / "notes.txt") << "hello";

    LibraryImporter importer(songs_.get(), coverCache_.get());
    const ImportStats stats = importer.importDirectory(root_.string());

    EXPECT_EQ(stats.scanned, 3);
    EXPECT_EQ(stats.added, 3);
    EXPECT_EQ(songs_->getAllSongs().size(), 3U);
}

TEST_F(LibraryImporterTest, ReimportSkipsUnchangedFiles) {
    const auto wav = root_ / "song.wav";
    writeSilentWav(wav, 0.2);

    LibraryImporter importer(songs_.get(), coverCache_.get());
    auto first = importer.importDirectory(root_.string());
    EXPECT_EQ(first.added, 1);

    auto second = importer.importDirectory(root_.string());
    EXPECT_EQ(second.scanned, 1);
    EXPECT_EQ(second.skipped, 1);
    EXPECT_EQ(second.added, 0);
    EXPECT_EQ(second.updated, 0);
}

TEST_F(LibraryImporterTest, ChangedFileTriggersUpdate) {
    const auto wav = root_ / "edit.wav";
    writeSilentWav(wav, 0.2);

    LibraryImporter importer(songs_.get(), coverCache_.get());
    auto first = importer.importDirectory(root_.string());
    EXPECT_EQ(first.added, 1);

    // Rewrite with a longer duration so size + mtime both change. Some
    // filesystems quantize mtime to 1 second so wait briefly.
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    writeSilentWav(wav, 0.5);

    auto second = importer.importDirectory(root_.string());
    EXPECT_EQ(second.scanned, 1);
    EXPECT_EQ(second.updated, 1);
    EXPECT_EQ(second.skipped, 0);
    EXPECT_EQ(songs_->getAllSongs().size(), 1U);
}

TEST_F(LibraryImporterTest, PicksUpSidecarLyricFile) {
    const auto wav = root_ / "withlrc.wav";
    writeSilentWav(wav, 0.2);
    std::ofstream(root_ / "withlrc.lrc") << "[00:00.00]hello\n";

    LibraryImporter importer(songs_.get(), coverCache_.get());
    auto stats = importer.importFile(wav.string());
    EXPECT_EQ(stats.added, 1);

    auto rows = songs_->getAllSongs();
    ASSERT_EQ(rows.size(), 1U);
    EXPECT_TRUE(rows[0].hasLyric);
    EXPECT_EQ(rows[0].lyricPath, (root_ / "withlrc.lrc").string());
}

TEST_F(LibraryImporterTest, NullCoverCacheStillImports) {
    const auto wav = root_ / "nocover.wav";
    writeSilentWav(wav, 0.2);
    LibraryImporter importer(songs_.get(), nullptr);
    auto stats = importer.importFile(wav.string());
    EXPECT_EQ(stats.added, 1);
    EXPECT_EQ(songs_->getAllSongs().size(), 1U);
}

TEST_F(LibraryImporterTest, FailsOnNonAudioFile) {
    const auto txt = root_ / "junk.txt";
    std::ofstream(txt) << "not audio";
    LibraryImporter importer(songs_.get(), coverCache_.get());
    auto stats = importer.importFile(txt.string());
    EXPECT_EQ(stats.failed, 1);
    EXPECT_EQ(stats.added, 0);
}
