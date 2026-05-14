#include <gtest/gtest.h>
#include "song_info.h"
#include "song_table_model.h"

#include <QMimeData>
#include <QModelIndex>
#include <QSignalSpy>
#include <QString>

#include <vector>

using musicplayer::SongInfo;
using musicplayer::SongTableModel;

namespace {

SongInfo make(int id, std::string title, std::string artist, std::string album, double duration) {
    SongInfo s;
    s.id = id;
    s.title = std::move(title);
    s.artist = std::move(artist);
    s.album = std::move(album);
    s.duration = duration;
    return s;
}

}  // namespace

TEST(SongTableModel, EmptyByDefault) {
    SongTableModel m;
    EXPECT_EQ(m.rowCount(), 0);
    EXPECT_EQ(m.columnCount(), 5);
}

TEST(SongTableModel, SetSongsUpdatesRowCount) {
    SongTableModel m;
    m.setSongs({make(1, "A", "X", "Album1", 30.0), make(2, "B", "Y", "Album1", 45.0),
                make(3, "C", "Z", "Album2", 60.0)});
    EXPECT_EQ(m.rowCount(), 3);
}

TEST(SongTableModel, DataReturnsExpectedDisplayValues) {
    SongTableModel m;
    m.setSongs({make(11, "Hello", "Adele", "25", 295.0)});
    EXPECT_EQ(m.data(m.index(0, SongTableModel::ColumnIndex), Qt::DisplayRole).toInt(), 1);
    EXPECT_EQ(m.data(m.index(0, SongTableModel::ColumnTitle), Qt::DisplayRole).toString(),
              QStringLiteral("Hello"));
    EXPECT_EQ(m.data(m.index(0, SongTableModel::ColumnArtist), Qt::DisplayRole).toString(),
              QStringLiteral("Adele"));
    EXPECT_EQ(m.data(m.index(0, SongTableModel::ColumnAlbum), Qt::DisplayRole).toString(),
              QStringLiteral("25"));
    EXPECT_EQ(m.data(m.index(0, SongTableModel::ColumnDuration), Qt::DisplayRole).toString(),
              QStringLiteral("4:55"));
}

TEST(SongTableModel, DurationZeroShowsPlaceholder) {
    SongTableModel m;
    m.setSongs({make(1, "T", "A", "B", 0.0)});
    EXPECT_EQ(m.data(m.index(0, SongTableModel::ColumnDuration), Qt::DisplayRole).toString(),
              QStringLiteral("--:--"));
}

TEST(SongTableModel, HeaderDataForKnownColumns) {
    SongTableModel m;
    EXPECT_EQ(m.headerData(SongTableModel::ColumnTitle, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("Title"));
    EXPECT_EQ(
        m.headerData(SongTableModel::ColumnArtist, Qt::Horizontal, Qt::DisplayRole).toString(),
        QStringLiteral("Artist"));
    EXPECT_EQ(m.headerData(SongTableModel::ColumnAlbum, Qt::Horizontal, Qt::DisplayRole).toString(),
              QStringLiteral("Album"));
}

TEST(SongTableModel, InvalidIndexReturnsEmpty) {
    SongTableModel m;
    m.setSongs({make(1, "T", "A", "B", 10.0)});
    EXPECT_FALSE(m.data(m.index(99, 0), Qt::DisplayRole).isValid());
    EXPECT_FALSE(m.data(QModelIndex(), Qt::DisplayRole).isValid());
}

TEST(SongTableModel, SongIdAtMapsRowsToIds) {
    SongTableModel m;
    m.setSongs({make(100, "A", "", "", 0.0), make(200, "B", "", "", 0.0)});
    EXPECT_EQ(m.songIdAt(0), 100);
    EXPECT_EQ(m.songIdAt(1), 200);
    EXPECT_EQ(m.songIdAt(-1), 0);
    EXPECT_EQ(m.songIdAt(2), 0);
}

// ── Drag-drop reorder ────────────────────────────────────────────────────────

TEST(SongTableModel, FlagsReflectReorderable) {
    SongTableModel m;
    m.setSongs({make(1, "A", "", "", 0.0)});
    EXPECT_FALSE(m.flags(m.index(0, 0)) & Qt::ItemIsDragEnabled);
    m.setReorderable(true);
    EXPECT_TRUE(m.flags(m.index(0, 0)) & Qt::ItemIsDragEnabled);
    EXPECT_TRUE(m.flags(QModelIndex()) & Qt::ItemIsDropEnabled);
}

TEST(SongTableModel, MimeDataRoundTripsRowIndex) {
    SongTableModel m;
    m.setSongs({make(10, "A", "", "", 0.0), make(20, "B", "", "", 0.0)});
    m.setReorderable(true);
    auto* mime = m.mimeData({m.index(1, 0)});
    ASSERT_NE(mime, nullptr);
    EXPECT_TRUE(mime->hasFormat(QStringLiteral("application/x-musicplayer-song-row")));
    delete mime;
}

TEST(SongTableModel, DropMimeDataMovesRowAndEmitsSignal) {
    SongTableModel m;
    m.setSongs({make(1, "A", "", "", 0.0), make(2, "B", "", "", 0.0), make(3, "C", "", "", 0.0)});
    m.setReorderable(true);
    QSignalSpy spy(&m, &SongTableModel::songsReordered);

    // Move row 0 to the position past row 2 (i.e. drop at row index 3, the
    // append slot). Expected order: B, C, A.
    auto* mime = m.mimeData({m.index(0, 0)});
    ASSERT_NE(mime, nullptr);
    const bool ok = m.dropMimeData(mime, Qt::MoveAction, /*row=*/3, /*column=*/0, QModelIndex());
    delete mime;
    EXPECT_TRUE(ok);

    ASSERT_EQ(spy.count(), 1);
    const auto ids = spy.takeFirst().at(0).value<QList<int>>();
    ASSERT_EQ(ids.size(), 3);
    EXPECT_EQ(ids[0], 2);
    EXPECT_EQ(ids[1], 3);
    EXPECT_EQ(ids[2], 1);
}

TEST(SongTableModel, RemoveRowsIsNoOp) {
    SongTableModel m;
    m.setSongs({make(1, "A", "", "", 0.0), make(2, "B", "", "", 0.0)});
    EXPECT_TRUE(m.removeRows(0, 1));
    // The "removal" doesn't actually drop anything — dropMimeData owns moves.
    EXPECT_EQ(m.rowCount(), 2);
}

TEST(SongTableModel, DropMimeDataRequiresReorderable) {
    SongTableModel m;
    m.setSongs({make(1, "A", "", "", 0.0), make(2, "B", "", "", 0.0)});
    // Reorderable defaults to false; mimeData is therefore declined too.
    QMimeData mime;
    EXPECT_FALSE(m.dropMimeData(&mime, Qt::MoveAction, 1, 0, QModelIndex()));
}
