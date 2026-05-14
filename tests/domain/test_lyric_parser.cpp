#include <gtest/gtest.h>
#include "lyric_parser.h"

#include <vector>

using musicplayer::LyricLine;
using musicplayer::LyricParser;

TEST(LyricParser, EmptyInputReturnsNothing) {
    const auto result = LyricParser::parse("");
    EXPECT_TRUE(result.lines.empty());
    EXPECT_TRUE(result.metadata.empty());
}

TEST(LyricParser, ParsesSingleLineWithCentiseconds) {
    const auto result = LyricParser::parse("[00:12.34]Hello world");
    ASSERT_EQ(result.lines.size(), 1U);
    EXPECT_EQ(result.lines[0].timestampMs, 12340);
    EXPECT_EQ(result.lines[0].text, "Hello world");
}

TEST(LyricParser, ParsesMillisecondPrecision) {
    const auto result = LyricParser::parse("[00:01.234]ms");
    ASSERT_EQ(result.lines.size(), 1U);
    EXPECT_EQ(result.lines[0].timestampMs, 1234);
}

TEST(LyricParser, ParsesDecisecondPrecision) {
    const auto result = LyricParser::parse("[00:02.5]ds");
    ASSERT_EQ(result.lines.size(), 1U);
    EXPECT_EQ(result.lines[0].timestampMs, 2500);
}

TEST(LyricParser, NoFractionDefaultsToZero) {
    const auto result = LyricParser::parse("[01:30]plain");
    ASSERT_EQ(result.lines.size(), 1U);
    EXPECT_EQ(result.lines[0].timestampMs, 90'000);
}

TEST(LyricParser, MultipleTimestampsOnOneLineExpand) {
    const auto result = LyricParser::parse("[00:01.00][00:05.00]Refrain");
    ASSERT_EQ(result.lines.size(), 2U);
    EXPECT_EQ(result.lines[0].timestampMs, 1000);
    EXPECT_EQ(result.lines[0].text, "Refrain");
    EXPECT_EQ(result.lines[1].timestampMs, 5000);
    EXPECT_EQ(result.lines[1].text, "Refrain");
}

TEST(LyricParser, OutputIsSortedByTimestamp) {
    const auto result = LyricParser::parse("[00:30.00]Third\n[00:10.00]First\n[00:20.00]Second");
    ASSERT_EQ(result.lines.size(), 3U);
    EXPECT_EQ(result.lines[0].text, "First");
    EXPECT_EQ(result.lines[1].text, "Second");
    EXPECT_EQ(result.lines[2].text, "Third");
}

TEST(LyricParser, MetadataTagsCapturedSeparately) {
    const auto result = LyricParser::parse("[ti:Song]\n[ar:Artist]\n[al:Album]\n[00:00.00]start");
    EXPECT_EQ(result.metadata.at("ti"), "Song");
    EXPECT_EQ(result.metadata.at("ar"), "Artist");
    EXPECT_EQ(result.metadata.at("al"), "Album");
    ASSERT_EQ(result.lines.size(), 1U);
}

TEST(LyricParser, OffsetShiftsAllTimestamps) {
    // +500ms offset → every line shifts later by 500ms.
    const auto result = LyricParser::parse("[offset:500]\n[00:01.00]a\n[00:02.00]b");
    ASSERT_EQ(result.lines.size(), 2U);
    EXPECT_EQ(result.lines[0].timestampMs, 1500);
    EXPECT_EQ(result.lines[1].timestampMs, 2500);
}

TEST(LyricParser, OffsetClampsAtZero) {
    // -2000ms applied to a 1000ms line would underflow; expect clamp to 0.
    const auto result = LyricParser::parse("[offset:-2000]\n[00:01.00]a");
    ASSERT_EQ(result.lines.size(), 1U);
    EXPECT_EQ(result.lines[0].timestampMs, 0);
}

TEST(LyricParser, ToleratesCrlfAndBlankLines) {
    const auto result = LyricParser::parse("[00:01.00]a\r\n\r\n[00:02.00]b\r\n");
    ASSERT_EQ(result.lines.size(), 2U);
    EXPECT_EQ(result.lines[0].text, "a");
    EXPECT_EQ(result.lines[1].text, "b");
}

TEST(LyricParser, EmptyTextAllowed) {
    const auto result = LyricParser::parse("[00:01.00]");
    ASSERT_EQ(result.lines.size(), 1U);
    EXPECT_EQ(result.lines[0].text, "");
}

TEST(LyricParser, MalformedBracketIsSkipped) {
    const auto result = LyricParser::parse("[broken\n[00:01.00]valid");
    ASSERT_EQ(result.lines.size(), 1U);
    EXPECT_EQ(result.lines[0].text, "valid");
}

// ── findCurrentLineIndex ────────────────────────────────────────────────────

TEST(FindCurrentLineIndex, EmptyLinesReturnsMinusOne) {
    std::vector<LyricLine> lines;
    EXPECT_EQ(LyricParser::findCurrentLineIndex(lines, 0), -1);
    EXPECT_EQ(LyricParser::findCurrentLineIndex(lines, 1000), -1);
}

TEST(FindCurrentLineIndex, BeforeFirstReturnsMinusOne) {
    std::vector<LyricLine> lines = {{1000, "a"}, {2000, "b"}};
    EXPECT_EQ(LyricParser::findCurrentLineIndex(lines, 500), -1);
}

TEST(FindCurrentLineIndex, ExactBoundaryReturnsThatLine) {
    std::vector<LyricLine> lines = {{1000, "a"}, {2000, "b"}};
    EXPECT_EQ(LyricParser::findCurrentLineIndex(lines, 1000), 0);
    EXPECT_EQ(LyricParser::findCurrentLineIndex(lines, 2000), 1);
}

TEST(FindCurrentLineIndex, BetweenLinesReturnsPriorLine) {
    std::vector<LyricLine> lines = {{1000, "a"}, {3000, "b"}};
    EXPECT_EQ(LyricParser::findCurrentLineIndex(lines, 1500), 0);
    EXPECT_EQ(LyricParser::findCurrentLineIndex(lines, 2999), 0);
}

TEST(FindCurrentLineIndex, PastLastSticksToLast) {
    std::vector<LyricLine> lines = {{1000, "a"}, {2000, "b"}};
    EXPECT_EQ(LyricParser::findCurrentLineIndex(lines, 60'000), 1);
}
