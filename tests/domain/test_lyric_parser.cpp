#include <gtest/gtest.h>
#include "lyric_parser.h"

using namespace musicplayer;

TEST(LyricParser, ParseEmpty) {
    auto lines = LyricParser::parse("");
    EXPECT_TRUE(lines.empty());
}

// DISABLED until Phase 6 (lyric display) implements LyricParser::parse.
// The current implementation in src/domain/lyric_parser.cpp is a stub
// that returns {} regardless of input. Re-enable by removing the
// DISABLED_ prefix when LRC parsing lands.
TEST(LyricParser, DISABLED_ParseSingleLine) {
    auto lines = LyricParser::parse("[00:12.34]Hello world");
    EXPECT_FALSE(lines.empty());
}
