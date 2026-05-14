#include <gtest/gtest.h>
#include "lyric_parser.h"
#include "lyric_widget.h"

#include <QListWidget>

#include <vector>

using musicplayer::LyricLine;
using musicplayer::LyricWidget;

TEST(LyricWidgetTest, SetLinesPopulatesList) {
    LyricWidget w;
    std::vector<LyricLine> lines{{0, "first"}, {1000, "second"}, {2000, "third"}};
    w.setLines(lines);
    EXPECT_EQ(w.lineCount(), 3);
    auto* list = w.findChild<QListWidget*>();
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->count(), 3);
    EXPECT_EQ(list->item(0)->text(), QStringLiteral("first"));
}

TEST(LyricWidgetTest, EmptyLinesShowsPlaceholderRow) {
    LyricWidget w;
    w.setLines({});
    auto* list = w.findChild<QListWidget*>();
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->count(), 1);  // (no lyrics) placeholder
    EXPECT_NE(list->item(0)->text(), QStringLiteral("first"));
}

TEST(LyricWidgetTest, SetCurrentLineUpdatesIndexAndBolds) {
    LyricWidget w;
    std::vector<LyricLine> lines{{0, "a"}, {1000, "b"}, {2000, "c"}};
    w.setLines(lines);
    w.setCurrentLine(1);
    EXPECT_EQ(w.currentLineIndex(), 1);
    auto* list = w.findChild<QListWidget*>();
    ASSERT_NE(list, nullptr);
    EXPECT_TRUE(list->item(1)->font().bold());
    EXPECT_FALSE(list->item(0)->font().bold());
}

TEST(LyricWidgetTest, ChangingCurrentLineRemovesPreviousBold) {
    LyricWidget w;
    std::vector<LyricLine> lines{{0, "a"}, {1000, "b"}, {2000, "c"}};
    w.setLines(lines);
    w.setCurrentLine(0);
    w.setCurrentLine(2);
    auto* list = w.findChild<QListWidget*>();
    ASSERT_NE(list, nullptr);
    EXPECT_FALSE(list->item(0)->font().bold());
    EXPECT_TRUE(list->item(2)->font().bold());
}

TEST(LyricWidgetTest, SetCurrentLineMinusOneClearsBold) {
    LyricWidget w;
    std::vector<LyricLine> lines{{0, "a"}, {1000, "b"}};
    w.setLines(lines);
    w.setCurrentLine(0);
    w.setCurrentLine(-1);
    auto* list = w.findChild<QListWidget*>();
    ASSERT_NE(list, nullptr);
    EXPECT_FALSE(list->item(0)->font().bold());
}
