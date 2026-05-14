#pragma once

#include <QMetaType>
#include <QWidget>

#include <cstdint>
#include <vector>

#include "playlist.h"

class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace musicplayer {

// Identifies which view the user has selected in the sidebar. AllSongs and
// RecentlyPlayed are derived views; UserPlaylist refers to a specific row
// in the playlists table (system "我喜欢" included).
enum class LibrarySource : std::uint8_t {
    AllSongs,
    RecentlyPlayed,
    UserPlaylist,
};

// Left-side navigation: two sections, "Sources" (fixed) and "Playlists"
// (system + user). A "+ New Playlist" button at the top is the
// canonical way for the user to create a playlist; right-click on a user
// playlist row offers Rename / Delete.
class PlaylistSidebar : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistSidebar(QWidget* parent = nullptr);

    // Replaces the Playlists section. Called by MainWindow whenever
    // PlaylistManager fires create/rename/delete signals.
    void setPlaylists(const std::vector<Playlist>& playlists);

    // Programmatically select a row. Useful for "after creating a new
    // playlist, jump to it".
    void selectPlaylist(int playlistId);
    void selectAllSongs();

signals:
    void sourceSelected(LibrarySource source, int playlistId);
    void newPlaylistRequested();
    void renamePlaylistRequested(int playlistId);
    void deletePlaylistRequested(int playlistId);

private slots:
    void onItemActivated(QListWidgetItem* item);
    void onContextMenu(const QPoint& pos);

private:
    void rebuild();

    std::vector<Playlist> playlists_;
    QListWidget* list_;
    QPushButton* newBtn_;
};

}  // namespace musicplayer

// Register the enum with Qt's meta-system so QSignalSpy / QVariant can
// marshal it across signal connections.
Q_DECLARE_METATYPE(musicplayer::LibrarySource)
