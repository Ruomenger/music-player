#include "main_window.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "constants.h"
#include "control_bar.h"
#include "cover_widget.h"
#include "library_importer.h"
#include "lyric_manager.h"
#include "lyric_widget.h"
#include "new_playlist_dialog.h"
#include "player_controller.h"
#include "playlist_manager.h"
#include "playlist_sidebar.h"
#include "playlist_widget.h"
#include "song_info.h"
#include "sqlite_play_history_repo.h"
#include "sqlite_song_repo.h"

namespace musicplayer {

namespace {

constexpr int kCoverSize = 200;
constexpr int kRecentLimit = 50;

QString toQString(std::string_view sv) {
    return QString::fromUtf8(sv.data(), static_cast<qsizetype>(sv.size()));
}

QStringList audioFileFilters() {
    return {
        QObject::tr("Audio files (*.mp3 *.flac *.wav *.aac *.ogg *.opus *.m4a *.ape *.wma)"),
        QObject::tr("All files (*)"),
    };
}

}  // namespace

MainWindow::MainWindow(SqliteSongRepo* songs, PlaylistManager* playlists, PlayerController* player,
                       LibraryImporter* importer, LyricManager* lyrics,
                       SqlitePlayHistoryRepo* history, QWidget* parent)
    : QMainWindow(parent)
    , songs_(songs)
    , playlists_(playlists)
    , player_(player)
    , importer_(importer)
    , lyrics_(lyrics)
    , history_(history) {
    setWindowTitle(toQString(kAppName));
    setMinimumSize(900, 600);

    buildLayout();
    buildMenus();
    connectSignals();
    refreshSidebar();
    refreshSongList();
}

void MainWindow::buildLayout() {
    auto* central = new QWidget(this);

    coverWidget_ = new CoverWidget(central);
    coverWidget_->setFixedSize(kCoverSize, kCoverSize);
    titleLabel_ = new QLabel(QStringLiteral("—"), central);
    titleLabel_->setWordWrap(true);
    QFont titleFont = titleLabel_->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel_->setFont(titleFont);
    artistLabel_ = new QLabel(QStringLiteral("—"), central);
    artistLabel_->setWordWrap(true);

    auto* leftLayout = new QVBoxLayout();
    leftLayout->addWidget(coverWidget_);
    leftLayout->addWidget(titleLabel_);
    leftLayout->addWidget(artistLabel_);
    leftLayout->addStretch(1);
    auto* leftPanel = new QWidget(central);
    leftPanel->setLayout(leftLayout);

    sidebar_ = new PlaylistSidebar(central);
    playlistWidget_ = new PlaylistWidget(central);
    lyricWidget_ = new LyricWidget(central);

    auto* rightTabs = new QTabWidget(central);
    rightTabs->addTab(playlistWidget_, tr("Library"));
    rightTabs->addTab(lyricWidget_, tr("Lyrics"));

    // QSplitter lets the user resize the sidebar / cover-info / right-pane
    // proportions. Default stretch factors keep the right tabs dominant.
    auto* splitter = new QSplitter(Qt::Horizontal, central);
    splitter->addWidget(sidebar_);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightTabs);
    splitter->setStretchFactor(0, 0);  // sidebar fixed-ish
    splitter->setStretchFactor(1, 0);  // cover/info fixed-ish
    splitter->setStretchFactor(2, 1);  // right tabs absorb the rest
    sidebar_->setMinimumWidth(160);
    sidebar_->setMaximumWidth(260);

    controlBar_ = new ControlBar(central);

    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->addWidget(splitter, /*stretch=*/1);
    rootLayout->addWidget(controlBar_);

    setCentralWidget(central);

    statusLabel_ = new QLabel(tr("Ready"), this);
    statusBar()->addPermanentWidget(statusLabel_);
}

void MainWindow::buildMenus() {
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    auto* addFilesAction = fileMenu->addAction(tr("Add Files…"));
    connect(addFilesAction, &QAction::triggered, this, &MainWindow::onAddFiles);
    auto* addFolderAction = fileMenu->addAction(tr("Add Folder…"));
    connect(addFolderAction, &QAction::triggered, this, &MainWindow::onAddFolder);
    fileMenu->addSeparator();
    auto* quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    auto* aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::connectSignals() {
    // UI → controller
    connect(controlBar_, &ControlBar::playRequested, player_, &PlayerController::resume);
    connect(controlBar_, &ControlBar::pauseRequested, player_, &PlayerController::pause);
    connect(controlBar_, &ControlBar::stopRequested, player_, &PlayerController::stop);
    connect(controlBar_, &ControlBar::previousRequested, player_, &PlayerController::previous);
    connect(controlBar_, &ControlBar::nextRequested, player_, &PlayerController::next);
    connect(controlBar_, &ControlBar::seekRequested, player_, &PlayerController::seek);
    connect(controlBar_, &ControlBar::volumeChangeRequested, player_, &PlayerController::setVolume);
    connect(controlBar_, &ControlBar::playModeChangeRequested, player_,
            &PlayerController::setPlayMode);

    // Library view → controller / manager
    connect(playlistWidget_, &PlaylistWidget::songActivated, this, &MainWindow::onSongActivated);
    connect(playlistWidget_, &PlaylistWidget::removeSongRequested, this,
            &MainWindow::onRemoveSongRequested);
    connect(playlistWidget_, &PlaylistWidget::toggleFavoriteRequested, this,
            &MainWindow::onToggleFavorite);
    connect(playlistWidget_, &PlaylistWidget::songsReorderedSignal, this,
            &MainWindow::onSongsReordered);

    // Sidebar → main window
    connect(sidebar_, &PlaylistSidebar::sourceSelected, this, &MainWindow::onSourceSelected);
    connect(sidebar_, &PlaylistSidebar::newPlaylistRequested, this,
            &MainWindow::onNewPlaylistRequested);
    connect(sidebar_, &PlaylistSidebar::renamePlaylistRequested, this,
            &MainWindow::onRenamePlaylistRequested);
    connect(sidebar_, &PlaylistSidebar::deletePlaylistRequested, this,
            &MainWindow::onDeletePlaylistRequested);

    // Playlist manager → sidebar refresh
    if (playlists_) {
        connect(playlists_, &PlaylistManager::playlistCreated, this, &MainWindow::refreshSidebar);
        connect(playlists_, &PlaylistManager::playlistRenamed, this, &MainWindow::refreshSidebar);
        connect(playlists_, &PlaylistManager::playlistDeleted, this, [this](int id) {
            // If the user deleted the currently-displayed playlist, fall
            // back to All Songs so we don't keep showing a stale view.
            if (currentSource_ == LibrarySource::UserPlaylist && currentPlaylistId_ == id) {
                currentSource_ = LibrarySource::AllSongs;
                currentPlaylistId_ = 0;
                refreshSongList();
                sidebar_->selectAllSongs();
            }
            refreshSidebar();
        });
        connect(playlists_, &PlaylistManager::songsChanged, this, [this](int id) {
            if (currentSource_ == LibrarySource::UserPlaylist && currentPlaylistId_ == id)
                refreshSongList();
        });
    }

    // Controller → UI
    connect(player_, &PlayerController::currentSongChanged, this,
            &MainWindow::onCurrentSongChanged);
    connect(player_, &PlayerController::positionChanged, controlBar_, &ControlBar::setPosition);
    connect(player_, &PlayerController::volumeChanged, controlBar_, &ControlBar::setVolume);
    connect(player_, &PlayerController::playModeChanged, this,
            [this](int mode) { controlBar_->setPlayMode(static_cast<PlayMode>(mode)); });
    connect(player_, &PlayerController::stateChanged, this, &MainWindow::onPlayerStateChanged);
    connect(player_, &PlayerController::errorOccurred, this, &MainWindow::onErrorOccurred);

    // Lyrics: controller → manager → widget.
    if (lyrics_) {
        connect(player_, &PlayerController::currentSongChanged, lyrics_,
                &LyricManager::loadForSong);
        connect(player_, &PlayerController::positionChanged, lyrics_,
                &LyricManager::updatePosition);
        connect(lyrics_, &LyricManager::lyricsChanged, lyricWidget_, &LyricWidget::setLines);
        connect(lyrics_, &LyricManager::currentLineChanged, lyricWidget_,
                &LyricWidget::setCurrentLine);
        connect(lyricWidget_, &LyricWidget::manualLyricsRequested, lyrics_,
                &LyricManager::loadFromPath);
    }

    controlBar_->setVolume(player_->volume());
    controlBar_->setPlayMode(player_->playMode());
}

void MainWindow::refreshSidebar() {
    if (!sidebar_ || !playlists_)
        return;
    sidebar_->setPlaylists(playlists_->allPlaylists());
}

void MainWindow::refreshSongList() {
    if (!songs_)
        return;
    switch (currentSource_) {
        case LibrarySource::AllSongs:
            playlistWidget_->setSongs(songs_->getAllSongs());
            playlistWidget_->setReorderable(false);
            playlistWidget_->setRemovable(false);
            break;
        case LibrarySource::RecentlyPlayed: {
            std::vector<SongInfo> result;
            if (history_) {
                for (const auto& entry : history_->recent(kRecentLimit))
                    result.push_back(entry.song);
            }
            playlistWidget_->setSongs(std::move(result));
            playlistWidget_->setReorderable(false);
            playlistWidget_->setRemovable(false);
            break;
        }
        case LibrarySource::UserPlaylist:
            playlistWidget_->setSongs(playlists_->songsIn(currentPlaylistId_));
            playlistWidget_->setReorderable(true);
            playlistWidget_->setRemovable(true);
            break;
    }
}

void MainWindow::onAddFiles() {
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Add audio files"), QString(), audioFileFilters().join(QStringLiteral(";;")));
    if (paths.isEmpty())
        return;
    ImportStats stats;
    for (const QString& p : paths) {
        const auto s = importer_->importFile(p.toStdString());
        stats.scanned += s.scanned;
        stats.added += s.added;
        stats.updated += s.updated;
        stats.skipped += s.skipped;
        stats.failed += s.failed;
    }
    statusLabel_->setText(tr("Imported %1 of %2 files (skipped %3)")
                              .arg(stats.added)
                              .arg(stats.scanned)
                              .arg(stats.skipped));
    refreshSongList();
}

void MainWindow::onAddFolder() {
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Add folder"));
    if (dir.isEmpty())
        return;
    const auto stats = importer_->importDirectory(dir.toStdString());
    statusLabel_->setText(tr("Imported %1 of %2 files (updated %3, skipped %4)")
                              .arg(stats.added)
                              .arg(stats.scanned)
                              .arg(stats.updated)
                              .arg(stats.skipped));
    refreshSongList();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("About %1").arg(toQString(kAppName)),
                       tr("%1 %2\nCross-platform desktop music player.")
                           .arg(toQString(kAppName))
                           .arg(toQString(kAppVersion)));
}

void MainWindow::onSongActivated(int songId) {
    player_->play(songId);
}

void MainWindow::onRemoveSongRequested(int songId) {
    if (currentSource_ != LibrarySource::UserPlaylist || currentPlaylistId_ <= 0)
        return;
    playlists_->removeSong(currentPlaylistId_, songId);
}

void MainWindow::onToggleFavorite(int songId) {
    if (playlists_)
        playlists_->toggleFavorite(songId);
}

void MainWindow::onSongsReordered(const QList<int>& orderedSongIds) {
    if (currentSource_ != LibrarySource::UserPlaylist || currentPlaylistId_ <= 0)
        return;
    const std::vector<int> ids(orderedSongIds.cbegin(), orderedSongIds.cend());
    playlists_->reorderSongs(currentPlaylistId_, ids);
}

void MainWindow::onCurrentSongChanged(const SongInfo& song) {
    titleLabel_->setText(song.title.empty() ? QStringLiteral("—")
                                            : QString::fromStdString(song.title));
    artistLabel_->setText(song.artist.empty() ? QStringLiteral("—")
                                              : QString::fromStdString(song.artist));
    coverWidget_->setCoverPath(QString::fromStdString(song.coverPath));
}

void MainWindow::onPlayerStateChanged(int state) {
    const auto s = static_cast<AudioEngine::State>(state);
    controlBar_->setPlayingState(s == AudioEngine::State::Playing);
    switch (s) {
        case AudioEngine::State::Playing:
            statusLabel_->setText(tr("Playing"));
            break;
        case AudioEngine::State::Paused:
            statusLabel_->setText(tr("Paused"));
            break;
        case AudioEngine::State::Stopped:
            statusLabel_->setText(tr("Stopped"));
            break;
    }
}

void MainWindow::onErrorOccurred(const QString& message) {
    statusLabel_->setText(message);
    QMessageBox::warning(this, tr("Playback error"), message);
}

void MainWindow::onSourceSelected(LibrarySource source, int playlistId) {
    currentSource_ = source;
    currentPlaylistId_ = playlistId;
    refreshSongList();
}

void MainWindow::onNewPlaylistRequested() {
    const auto result = NewPlaylistDialog::run(this);
    if (!result)
        return;
    const int id = playlists_->createPlaylist(result->name, result->description);
    if (id > 0) {
        // refreshSidebar already runs via the playlistCreated signal — but
        // it races with this slot, so call it explicitly to be sure the
        // sidebar shows the new row before we ask it to select.
        refreshSidebar();
        sidebar_->selectPlaylist(id);
    }
}

void MainWindow::onRenamePlaylistRequested(int playlistId) {
    const auto existing = playlists_->playlistById(playlistId);
    if (!existing)
        return;
    bool ok = false;
    const QString newName =
        QInputDialog::getText(this, tr("Rename playlist"), tr("New name:"), QLineEdit::Normal,
                              QString::fromStdString(existing->name), &ok);
    if (!ok || newName.trimmed().isEmpty())
        return;
    playlists_->renamePlaylist(playlistId, newName.trimmed());
}

void MainWindow::onDeletePlaylistRequested(int playlistId) {
    const auto existing = playlists_->playlistById(playlistId);
    if (!existing)
        return;
    const auto answer = QMessageBox::question(
        this, tr("Delete playlist"),
        tr("Delete playlist '%1'?").arg(QString::fromStdString(existing->name)));
    if (answer == QMessageBox::Yes)
        playlists_->deletePlaylist(playlistId);
}

}  // namespace musicplayer
