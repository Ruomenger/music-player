#include "main_window.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
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
#include "player_controller.h"
#include "playlist_manager.h"
#include "playlist_widget.h"
#include "song_info.h"
#include "sqlite_song_repo.h"

namespace musicplayer {

namespace {

constexpr int kCoverSize = 200;

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
                       LibraryImporter* importer, LyricManager* lyrics, QWidget* parent)
    : QMainWindow(parent)
    , songs_(songs)
    , playlists_(playlists)
    , player_(player)
    , importer_(importer)
    , lyrics_(lyrics) {
    setWindowTitle(toQString(kAppName));
    setMinimumSize(800, 600);

    buildLayout();
    buildMenus();
    connectSignals();
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

    playlistWidget_ = new PlaylistWidget(central);
    lyricWidget_ = new LyricWidget(central);

    // QTabWidget lets the user flip between the library view and the lyric
    // pane without losing screen real estate. Phase 7 will probably move the
    // library to a sidebar of its own.
    auto* rightTabs = new QTabWidget(central);
    rightTabs->addTab(playlistWidget_, tr("Library"));
    rightTabs->addTab(lyricWidget_, tr("Lyrics"));

    auto* topLayout = new QHBoxLayout();
    topLayout->addLayout(leftLayout);
    topLayout->addWidget(rightTabs, /*stretch=*/1);

    controlBar_ = new ControlBar(central);

    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->addLayout(topLayout, /*stretch=*/1);
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
    connect(playlistWidget_, &PlaylistWidget::songActivated, this, &MainWindow::onSongActivated);

    // Controller → UI
    connect(player_, &PlayerController::currentSongChanged, this,
            &MainWindow::onCurrentSongChanged);
    connect(player_, &PlayerController::positionChanged, controlBar_, &ControlBar::setPosition);
    connect(player_, &PlayerController::volumeChanged, controlBar_, &ControlBar::setVolume);
    connect(player_, &PlayerController::playModeChanged, this,
            [this](int mode) { controlBar_->setPlayMode(static_cast<PlayMode>(mode)); });
    connect(player_, &PlayerController::stateChanged, this, &MainWindow::onPlayerStateChanged);
    connect(player_, &PlayerController::errorOccurred, this, &MainWindow::onErrorOccurred);

    // Lyrics: controller → manager → widget. The manager is loaded from the
    // song's lyricPath on currentSongChanged and advances on positionChanged.
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

    // Initial reflection of player state so the volume slider isn't stale.
    controlBar_->setVolume(player_->volume());
    controlBar_->setPlayMode(player_->playMode());
}

void MainWindow::refreshSongList() {
    if (!songs_)
        return;
    playlistWidget_->setSongs(songs_->getAllSongs());
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

}  // namespace musicplayer
