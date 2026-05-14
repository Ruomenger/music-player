#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include <vector>

#include "audio_engine.h"
#include "play_queue.h"
#include "player_state_repo.h"
#include "song_info.h"
#include "sqlite_play_history_repo.h"
#include "sqlite_song_repo.h"

namespace musicplayer {

// Application-layer playback controller. Wires the domain AudioEngine to the
// infra repos and exposes a Qt signal/slot façade for the UI.
//
// Ownership: AudioEngine is owned by the controller; repos are non-owning
// pointers supplied by the application bootstrap.
//
// Threading: positionTimer_ runs on the controller's thread (main). The audio
// callback drives state transitions internally to AudioEngine; the timer
// polls AudioEngine::state() at ~250ms and re-emits stateChanged whenever it
// changes, plus drives the next-track auto-advance on natural EOF.
class PlayerController : public QObject {
    Q_OBJECT
public:
    PlayerController(AudioEngine* engine, SqliteSongRepo* songs, SqlitePlayHistoryRepo* history,
                     PlayerStateRepo* playerState, QObject* parent = nullptr);
    ~PlayerController() override;

    PlayerController(const PlayerController&) = delete;
    PlayerController& operator=(const PlayerController&) = delete;
    PlayerController(PlayerController&&) = delete;
    PlayerController& operator=(PlayerController&&) = delete;

    // Play a single song by id. The queue is replaced with just this song
    // unless setQueue() has already been called.
    void play(int songId);

    // Set the active queue (vector of song ids) and start playing at index.
    // `startIndex` must be < queue.size(); behaviour is undefined otherwise.
    void playQueue(const std::vector<int>& songIds, std::size_t startIndex);

    void resume();
    void pause();
    void stop();
    void next();
    void previous();
    void seek(double seconds);

    void setVolume(double volume);
    void setPlayMode(PlayMode mode);

    [[nodiscard]] PlayMode playMode() const { return queue_.mode(); }
    [[nodiscard]] double volume() const;
    [[nodiscard]] int currentSongId() const { return currentSongId_; }
    [[nodiscard]] AudioEngine::State state() const { return engine_->state(); }

    // Loads (current_song_id, playlist_id, position) from player_state and
    // resumes the last song in Stopped state at the saved position. Caller
    // must call play() / resume() to actually start audio.
    void restoreLastSession();

    // Persists the current (song, position, ...) to player_state. Call this
    // on quit and periodically while playing.
    void persistSession(int playlistId = 0);

signals:
    void stateChanged(int state);  // AudioEngine::State serialized as int
    void positionChanged(double current, double total);
    void currentSongChanged(const SongInfo& song);
    void volumeChanged(double volume);
    void playModeChanged(int mode);  // PlayMode serialized as int
    void errorOccurred(const QString& message);

private:
    void onPositionTick();
    void emitStateIfChanged(AudioEngine::State current);
    void handleNaturalEnd();
    bool openAndPlay(int songId, double startPosition = 0.0);
    void logHistoryIfWorthwhile();

    AudioEngine* engine_;
    SqliteSongRepo* songs_;
    SqlitePlayHistoryRepo* history_;
    PlayerStateRepo* playerState_;

    PlayQueue queue_;
    QTimer positionTimer_;
    AudioEngine::State lastSeenState_ = AudioEngine::State::Stopped;
    int currentSongId_ = 0;
    bool userStopRequested_ = false;  // distinguishes user stop from natural end
    bool historyRecorded_ = false;    // ensure one record() per play
};

}  // namespace musicplayer
