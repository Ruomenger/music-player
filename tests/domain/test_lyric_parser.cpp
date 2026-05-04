#include <gtest/gtest.h>
#include "lyric_parser.h"

using namespace musicplayer;

TEST(LyricParser, ParseEmpty) {
    auto lines = LyricParser::parse("");
    EXPECT_TRUE(lines.empty());
}

TEST(LyricParser, ParseSingleLine) {
    auto lines = LyricParser::parse("[00:12.34]Hello world");
    EXPECT_FALSE(lines.empty());
}
