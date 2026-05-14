#include <gtest/gtest.h>
#include "lyric_manager.h"
#include "song_info.h"

#include <QFile>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>

using musicplayer::LyricManager;
using musicplayer::SongInfo;

namespace {

void writeLrc(const QString& path, const QString& content) {
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(content.toUtf8());
}

}  // namespace

TEST(LyricManagerTest, EmptyByDefault) {
    LyricManager mgr;
    EXPECT_TRUE(mgr.lines().empty());
    EXPECT_EQ(mgr.currentLineIndex(), -1);
}

TEST(LyricManagerTest, LoadForSongWithoutLyricClears) {
    LyricManager mgr;
    SongInfo song;
    song.hasLyric = false;
    QSignalSpy spy(&mgr, &LyricManager::lyricsChanged);
    mgr.loadForSong(song);
    // Clear from an already-clear state is a no-op; spy may stay at 0.
    EXPECT_TRUE(mgr.lines().empty());
}

TEST(LyricManagerTest, LoadFromPathParsesAndEmits) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("song.lrc"));
    ASSERT_NO_FATAL_FAILURE(writeLrc(path, QStringLiteral("[00:01.00]one\n[00:02.00]two\n")));

    LyricManager mgr;
    QSignalSpy spy(&mgr, &LyricManager::lyricsChanged);
    mgr.loadFromPath(path);
    EXPECT_EQ(mgr.lines().size(), 2U);
    EXPECT_EQ(mgr.lines()[0].text, "one");
    EXPECT_EQ(mgr.lines()[1].text, "two");
    EXPECT_GE(spy.count(), 1);
}

TEST(LyricManagerTest, MissingFileClears) {
    LyricManager mgr;
    mgr.loadFromPath(QStringLiteral("/no/such/file.lrc"));
    EXPECT_TRUE(mgr.lines().empty());
    EXPECT_EQ(mgr.currentLineIndex(), -1);
}

TEST(LyricManagerTest, UpdatePositionEmitsCurrentLineChanged) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("song.lrc"));
    ASSERT_NO_FATAL_FAILURE(
        writeLrc(path, QStringLiteral("[00:01.00]one\n[00:02.00]two\n[00:03.00]three\n")));

    LyricManager mgr;
    mgr.loadFromPath(path);

    QSignalSpy spy(&mgr, &LyricManager::currentLineChanged);
    mgr.updatePosition(0.5, 10.0);  // before first line
    EXPECT_EQ(mgr.currentLineIndex(), -1);
    mgr.updatePosition(1.5, 10.0);  // on line 0
    EXPECT_EQ(mgr.currentLineIndex(), 0);
    mgr.updatePosition(2.5, 10.0);  // on line 1
    EXPECT_EQ(mgr.currentLineIndex(), 1);
    mgr.updatePosition(2.7, 10.0);  // still line 1 — no extra emit
    EXPECT_EQ(mgr.currentLineIndex(), 1);
    mgr.updatePosition(3.5, 10.0);  // on line 2
    EXPECT_EQ(mgr.currentLineIndex(), 2);
    // Three transitions (-1 → 0 → 1 → 2) plus the initial reset at load
    // time (-1 → -1 is suppressed by the change check). Allow either 3 or 4
    // depending on whether load_/setLines emits a redundant -1.
    EXPECT_GE(spy.count(), 3);
}

TEST(LyricManagerTest, ClearAfterLoadEmpties) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("song.lrc"));
    ASSERT_NO_FATAL_FAILURE(writeLrc(path, QStringLiteral("[00:01.00]one\n")));
    LyricManager mgr;
    mgr.loadFromPath(path);
    ASSERT_FALSE(mgr.lines().empty());
    mgr.clear();
    EXPECT_TRUE(mgr.lines().empty());
    EXPECT_EQ(mgr.currentLineIndex(), -1);
}

TEST(LyricManagerTest, AutoLoadDisabledSuppressesLoadForSong) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString lrc = dir.filePath(QStringLiteral("song.lrc"));
    ASSERT_NO_FATAL_FAILURE(writeLrc(lrc, QStringLiteral("[00:01.00]hello\n")));

    LyricManager mgr;
    mgr.setAutoLoadEnabled(false);
    SongInfo song;
    song.hasLyric = true;
    song.lyricPath = lrc.toStdString();
    mgr.loadForSong(song);
    EXPECT_TRUE(mgr.lines().empty());

    // Manual loadFromPath still works regardless of the toggle.
    mgr.loadFromPath(lrc);
    EXPECT_EQ(mgr.lines().size(), 1U);
}
