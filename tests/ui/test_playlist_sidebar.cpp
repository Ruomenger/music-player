#include <gtest/gtest.h>
#include "playlist.h"
#include "playlist_sidebar.h"

#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>

#include <vector>

using musicplayer::LibrarySource;
using musicplayer::Playlist;
using musicplayer::PlaylistSidebar;

namespace {

Playlist makePlaylist(int id, std::string name, bool isSystem = false) {
    Playlist p;
    p.id = id;
    p.name = std::move(name);
    p.isSystem = isSystem;
    return p;
}

}  // namespace

TEST(PlaylistSidebarTest, StartsWithSourcesOnly) {
    PlaylistSidebar bar;
    auto* list = bar.findChild<QListWidget*>();
    ASSERT_NE(list, nullptr);
    // Headers "Sources" + 2 source rows + "Playlists" header + 0 playlists = 4.
    EXPECT_EQ(list->count(), 4);
}

TEST(PlaylistSidebarTest, SetPlaylistsAppendsRows) {
    PlaylistSidebar bar;
    bar.setPlaylists({makePlaylist(1, "我喜欢", true), makePlaylist(2, "Mix")});
    auto* list = bar.findChild<QListWidget*>();
    EXPECT_EQ(list->count(), 6);  // 4 fixed + 2 playlists
}

TEST(PlaylistSidebarTest, ClickingSourceRowEmitsSelected) {
    PlaylistSidebar bar;
    QSignalSpy spy(&bar, &PlaylistSidebar::sourceSelected);
    auto* list = bar.findChild<QListWidget*>();
    // Row 1 is "All Songs" (row 0 is the "Sources" header).
    list->setCurrentRow(1);
    emit list->itemClicked(list->item(1));
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).value<LibrarySource>(), LibrarySource::AllSongs);
}

TEST(PlaylistSidebarTest, ClickingPlaylistRowSendsId) {
    PlaylistSidebar bar;
    bar.setPlaylists({makePlaylist(7, "Workout")});
    QSignalSpy spy(&bar, &PlaylistSidebar::sourceSelected);
    auto* list = bar.findChild<QListWidget*>();
    // Last row should be the Workout playlist.
    auto* item = list->item(list->count() - 1);
    emit list->itemClicked(item);
    ASSERT_EQ(spy.count(), 1);
    const auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).value<LibrarySource>(), LibrarySource::UserPlaylist);
    EXPECT_EQ(args.at(1).toInt(), 7);
}

TEST(PlaylistSidebarTest, HeaderRowsAreNotSelectable) {
    PlaylistSidebar bar;
    auto* list = bar.findChild<QListWidget*>();
    // Row 0 is the "Sources" header — must not be selectable.
    EXPECT_FALSE(list->item(0)->flags() & Qt::ItemIsSelectable);
    QSignalSpy spy(&bar, &PlaylistSidebar::sourceSelected);
    emit list->itemClicked(list->item(0));
    EXPECT_EQ(spy.count(), 0);
}

TEST(PlaylistSidebarTest, NewButtonEmitsRequest) {
    PlaylistSidebar bar;
    QSignalSpy spy(&bar, &PlaylistSidebar::newPlaylistRequested);
    // The QPushButton is the only one in the widget.
    auto buttons = bar.findChildren<QPushButton*>();
    ASSERT_FALSE(buttons.isEmpty());
    emit buttons.first()->clicked();
    EXPECT_EQ(spy.count(), 1);
}
