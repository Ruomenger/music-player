#include <gtest/gtest.h>
#include "lyric_matcher.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using musicplayer::LyricMatcher;

namespace {

class LyricMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = fs::temp_directory_path() /
                ("musicplayer_lyric_" +
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

TEST_F(LyricMatcherTest, ReturnsLrcWhenSiblingExists) {
    const auto song = root_ / "track.mp3";
    const auto lrc = root_ / "track.lrc";
    touch(song);
    touch(lrc);
    EXPECT_EQ(LyricMatcher::findSidecarLrc(song.string()), lrc.string());
}

TEST_F(LyricMatcherTest, ReturnsEmptyWhenNoSibling) {
    const auto song = root_ / "lonely.mp3";
    touch(song);
    EXPECT_EQ(LyricMatcher::findSidecarLrc(song.string()), "");
}

TEST_F(LyricMatcherTest, MatchesUppercaseExtension) {
    const auto song = root_ / "shouty.mp3";
    const auto lrc = root_ / "shouty.LRC";
    touch(song);
    touch(lrc);
    // On case-insensitive filesystems (macOS default, Windows) we get the lrc
    // path back; on case-sensitive ones (Linux) the uppercase variant matches
    // via the matcher's enumerated list.
    EXPECT_FALSE(LyricMatcher::findSidecarLrc(song.string()).empty());
}
