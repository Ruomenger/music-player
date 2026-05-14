#include <QApplication>
#include <QMessageBox>
#include <QString>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "audio_engine.h"
#include "config_paths.h"
#include "constants.h"
#include "cover_cache.h"
#include "ffmpeg_decoder.h"
#include "library_importer.h"
#include "player_controller.h"
#include "player_state_repo.h"
#include "playlist_manager.h"
#include "portaudio_output.h"
#include "sqlite_database.h"
#include "sqlite_play_history_repo.h"
#include "sqlite_playlist_repo.h"
#include "sqlite_settings_repo.h"
#include "sqlite_song_repo.h"
#include "ui/main_window.h"

namespace {

QString toQString(std::string_view sv) {
    return QString::fromUtf8(sv.data(), static_cast<qsizetype>(sv.size()));
}

// Legacy "play one file from the command line" CLI mode. Preserves the manual
// listening test from Phase 1; the GUI is the default.
int runCli(const std::string& filePath) {
    std::cout << "Playing: " << filePath << '\n';
    musicplayer::AudioEngine engine;
    engine.setDecoder(std::make_unique<musicplayer::FfmpegDecoder>());
    engine.setOutput(std::make_unique<musicplayer::PortAudioOutput>());

    if (!engine.open(filePath)) {
        std::cerr << "Failed to open: " << filePath << '\n';
        return 1;
    }
    engine.play();
    while (engine.state() == musicplayer::AudioEngine::State::Playing)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        QApplication app(argc, argv);
        QApplication::setApplicationName(toQString(musicplayer::kAppName));
        QApplication::setApplicationVersion(toQString(musicplayer::kAppVersion));
        QApplication::setOrganizationName(toQString(musicplayer::kAppOrg));

        if (argc > 2 && std::string(argv[1]) == "--play")
            return runCli(argv[2]);

        // ── Persistence layer ────────────────────────────────────────────────
        const std::string dataDir = musicplayer::ConfigPaths::dataDir();
        const std::string dbPath = dataDir + "/library.db";
        musicplayer::SqliteDatabase db;
        if (!db.open(dbPath)) {
            QMessageBox::critical(nullptr, QObject::tr("Database error"),
                                  QObject::tr("Failed to open library database:\n%1")
                                      .arg(QString::fromStdString(dbPath)));
            return 1;
        }

        musicplayer::SqliteSongRepo songs(db.connectionName());
        musicplayer::SqlitePlaylistRepo playlists(db.connectionName());
        musicplayer::SqliteSettingsRepo settings(db.connectionName());
        musicplayer::SqlitePlayHistoryRepo history(db.connectionName());
        musicplayer::PlayerStateRepo playerState(db.connectionName());
        settings.seedDefaults();

        // ── Audio + import services ──────────────────────────────────────────
        musicplayer::CoverCache covers(musicplayer::ConfigPaths::cacheDir() + "/covers");
        musicplayer::LibraryImporter importer(&songs, &covers);

        musicplayer::AudioEngine engine;
        engine.setDecoder(std::make_unique<musicplayer::FfmpegDecoder>());
        engine.setOutput(std::make_unique<musicplayer::PortAudioOutput>());

        if (const auto savedVolume = settings.getDouble("volume"); savedVolume)
            engine.setVolume(*savedVolume);

        // ── App-layer orchestrators ──────────────────────────────────────────
        musicplayer::PlayerController player(&engine, &songs, &history, &playerState);
        musicplayer::PlaylistManager playlistManager(&playlists);
        playlistManager.ensureFavoritesPlaylist();

        // ── UI ───────────────────────────────────────────────────────────────
        musicplayer::MainWindow window(&songs, &playlistManager, &player, &importer);
        window.show();

        player.restoreLastSession();

        // Save state + history retention on quit. Pull the retention window
        // out of settings so the user's preference applies even though we
        // don't expose a UI for it yet.
        QObject::connect(&app, &QCoreApplication::aboutToQuit, [&] {
            player.persistSession();
            if (const auto days = settings.getInt("history_max_days"); days)
                history.purgeOlderThan(*days);
        });

        return QApplication::exec();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Fatal: unknown exception\n";
        return 1;
    }
}
