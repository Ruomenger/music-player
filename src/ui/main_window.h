#pragma once

#include <QMainWindow>

#include "playlist_sidebar.h"
#include "song_info.h"

class QAction;
class QLabel;
class QMenu;

namespace musicplayer {

class ControlBar;
class CoverWidget;
class LanguageManager;
class LibraryImporter;
class LyricManager;
class LyricWidget;
class PlayerController;
class PlaylistManager;
class PlaylistSidebar;
class PlaylistWidget;
class SqlitePlayHistoryRepo;
class SqliteSettingsRepo;
class SqliteSongRepo;

// Top-level window. Owns the UI widgets and wires them to the service objects
// supplied by main(). All five service pointers are non-owning.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(SqliteSongRepo* songs, PlaylistManager* playlists, PlayerController* player,
               LibraryImporter* importer, LyricManager* lyrics, SqlitePlayHistoryRepo* history,
               SqliteSettingsRepo* settings, LanguageManager* language, QWidget* parent = nullptr);

    // Refreshes the playlist view from the currently selected source.
    void refreshSongList();

    // Rebuilds the playlist sidebar from the playlists repo. Called after
    // create / rename / delete signals.
    void refreshSidebar();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onAddFiles();
    void onAddFolder();
    void onAbout();
    void onShowPreferences();
    void onSettingsApplied();
    void onSongActivated(int songId);
    void onRemoveSongRequested(int songId);
    void onToggleFavorite(int songId);
    void onSongsReordered(const QList<int>& orderedSongIds);
    void onCurrentSongChanged(const SongInfo& song);
    void onPlayerStateChanged(int state);
    void onErrorOccurred(const QString& message);
    void onSourceSelected(LibrarySource source, int playlistId);
    void onNewPlaylistRequested();
    void onRenamePlaylistRequested(int playlistId);
    void onDeletePlaylistRequested(int playlistId);

private:
    void buildMenus();
    void buildLayout();
    void connectSignals();
    void retranslateUi();

    SqliteSongRepo* songs_;
    PlaylistManager* playlists_;
    PlayerController* player_;
    LibraryImporter* importer_;
    LyricManager* lyrics_;
    SqlitePlayHistoryRepo* history_;
    SqliteSettingsRepo* settings_;
    LanguageManager* language_;

    ControlBar* controlBar_ = nullptr;
    PlaylistSidebar* sidebar_ = nullptr;
    PlaylistWidget* playlistWidget_ = nullptr;
    LyricWidget* lyricWidget_ = nullptr;
    CoverWidget* coverWidget_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* artistLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    QMenu* fileMenu_ = nullptr;
    QMenu* helpMenu_ = nullptr;
    QMenu* editMenu_ = nullptr;
    QAction* addFilesAction_ = nullptr;
    QAction* addFolderAction_ = nullptr;
    QAction* preferencesAction_ = nullptr;
    QAction* quitAction_ = nullptr;
    QAction* aboutAction_ = nullptr;

    LibrarySource currentSource_ = LibrarySource::AllSongs;
    int currentPlaylistId_ = 0;  // valid when currentSource_ == UserPlaylist
};

}  // namespace musicplayer
