#include <gtest/gtest.h>
#include "music_scanner.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace fs = std::filesystem;
using musicplayer::MusicScanner;

namespace {

class MusicScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test gets its own subtree under a unique temp dir so we don't
        // race other tests that also use tmp_path.
        root_ = fs::temp_directory_path() /
                ("musicplayer_scanner_" +
                 std::to_string(::testing::UnitTest::GetInstance()
                                    ->current_test_info()
                                    ->result()
                                    ->elapsed_time()) +
                 std::to_string(static_cast<int>(reinterpret_cast<std::uintptr_t>(this))));
        fs::create_directories(root_);
    }
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    static void touch(const fs::path& p) {
        fs::create_directories(p.parent_path());
        std::ofstream(p).put('x');
    }

    fs::path root_;
};

}  // namespace

TEST_F(MusicScannerTest, FindsCommonAudioExtensions) {
    touch(root_ / "song1.mp3");
    touch(root_ / "song2.FLAC");  // case-insensitive
    touch(root_ / "song3.opus");
    touch(root_ / "not_audio.txt");
    touch(root_ / "readme.md");

    auto results = MusicScanner::scan(root_.string());
    std::set<std::string> names;
    for (const auto& p : results)
        names.insert(fs::path(p).filename().string());

    EXPECT_TRUE(names.contains("song1.mp3"));
    EXPECT_TRUE(names.contains("song2.FLAC"));
    EXPECT_TRUE(names.contains("song3.opus"));
    EXPECT_FALSE(names.contains("not_audio.txt"));
    EXPECT_FALSE(names.contains("readme.md"));
}

TEST_F(MusicScannerTest, RecursesIntoSubdirectories) {
    touch(root_ / "a" / "b" / "c" / "deep.mp3");
    touch(root_ / "shallow.wav");

    auto results = MusicScanner::scan(root_.string());
    ASSERT_EQ(results.size(), 2U);
}

TEST_F(MusicScannerTest, EmptyDirectoryReturnsEmpty) {
    auto results = MusicScanner::scan(root_.string());
    EXPECT_TRUE(results.empty());
}

TEST_F(MusicScannerTest, NonexistentRootReturnsEmpty) {
    auto results = MusicScanner::scan((root_ / "missing").string());
    EXPECT_TRUE(results.empty());
}
