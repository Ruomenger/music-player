#pragma once

#include <QMainWindow>

#include "song_info.h"

class QLabel;

namespace musicplayer {

class ControlBar;
class CoverWidget;
class LibraryImporter;
class LyricManager;
class LyricWidget;
class PlayerController;
class PlaylistManager;
class PlaylistWidget;
class SqliteSongRepo;

// Top-level window. Owns the UI widgets and wires them to the four service
// objects supplied by main(): SqliteSongRepo (for the library view),
// PlayerController (transport + signals), PlaylistManager (system / user
// playlists — Phase 7 will surface these in a sidebar), LibraryImporter
// (File menu actions). All four pointers are non-owning.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(SqliteSongRepo* songs, PlaylistManager* playlists, PlayerController* player,
               LibraryImporter* importer, LyricManager* lyrics, QWidget* parent = nullptr);

    // Refreshes the playlist view from the songs repo. Called after import
    // and on startup. Public so main() can call it after restoreLastSession.
    void refreshSongList();

private slots:
    void onAddFiles();
    void onAddFolder();
    void onAbout();
    void onSongActivated(int songId);
    void onCurrentSongChanged(const SongInfo& song);
    void onPlayerStateChanged(int state);
    void onErrorOccurred(const QString& message);

private:
    void buildMenus();
    void buildLayout();
    void connectSignals();

    SqliteSongRepo* songs_;
    PlaylistManager* playlists_;
    PlayerController* player_;
    LibraryImporter* importer_;
    LyricManager* lyrics_;

    ControlBar* controlBar_ = nullptr;
    PlaylistWidget* playlistWidget_ = nullptr;
    LyricWidget* lyricWidget_ = nullptr;
    CoverWidget* coverWidget_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* artistLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

}  // namespace musicplayer
