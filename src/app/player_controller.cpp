#include "player_controller.h"

#include <QString>

#include <algorithm>

namespace musicplayer {

namespace {

constexpr int kPositionTickMs = 250;

// A play counts toward play_history once we've heard at least kHistoryThreshold
// seconds *or* the track ends naturally. 30s matches the network-music
// convention; both anchors avoid logging skim-through navigation.
constexpr double kHistoryThresholdSec = 30.0;

}  // namespace

PlayerController::PlayerController(AudioEngine* engine, SqliteSongRepo* songs,
                                   SqlitePlayHistoryRepo* history, PlayerStateRepo* playerState,
                                   QObject* parent)
    : QObject(parent)
    , engine_(engine)
    , songs_(songs)
    , history_(history)
    , playerState_(playerState) {
    positionTimer_.setInterval(kPositionTickMs);
    connect(&positionTimer_, &QTimer::timeout, this, &PlayerController::onPositionTick);
}

PlayerController::~PlayerController() = default;

double PlayerController::volume() const {
    return engine_->volume();
}

void PlayerController::play(int songId) {
    // play(id) replaces the queue with just this one song so manual next /
    // prev have a sensible no-op effect.
    queue_.setSongs({songId});
    queue_.setCurrent(songId);
    if (!openAndPlay(songId))
        return;
}

void PlayerController::playQueue(const std::vector<int>& songIds, std::size_t startIndex) {
    if (songIds.empty() || startIndex >= songIds.size())
        return;
    queue_.setSongs(songIds);
    const int startId = songIds[startIndex];
    queue_.setCurrent(startId);
    openAndPlay(startId);
}

void PlayerController::resume() {
    if (engine_->state() == AudioEngine::State::Paused) {
        engine_->play();
        emitStateIfChanged(engine_->state());
    }
}

void PlayerController::pause() {
    if (engine_->state() == AudioEngine::State::Playing) {
        engine_->pause();
        emitStateIfChanged(engine_->state());
    }
}

void PlayerController::stop() {
    userStopRequested_ = true;
    engine_->stop();
    positionTimer_.stop();
    emitStateIfChanged(engine_->state());
}

void PlayerController::next() {
    const auto nextId = queue_.advanceManual();
    if (!nextId.has_value())
        return;
    openAndPlay(*nextId);
}

void PlayerController::previous() {
    const auto prevId = queue_.retreatManual();
    if (!prevId.has_value())
        return;
    openAndPlay(*prevId);
}

void PlayerController::seek(double seconds) {
    engine_->seek(std::max(0.0, seconds));
    emit positionChanged(engine_->currentPosition(), engine_->duration());
}

void PlayerController::setVolume(double v) {
    const double clamped = std::clamp(v, 0.0, 1.0);
    engine_->setVolume(clamped);
    emit volumeChanged(clamped);
}

void PlayerController::setPlayMode(PlayMode mode) {
    if (mode == queue_.mode())
        return;
    queue_.setMode(mode);
    emit playModeChanged(static_cast<int>(mode));
}

void PlayerController::restoreLastSession() {
    if (!playerState_)
        return;
    const auto state = playerState_->load();
    if (!state || state->currentSongId <= 0)
        return;
    const auto song = songs_->getSongById(state->currentSongId);
    if (!song)
        return;
    // Build a one-song queue and stop after seek so the user can press play.
    queue_.setSongs({song->id});
    queue_.setCurrent(song->id);
    currentSongId_ = song->id;
    emit currentSongChanged(*song);
    if (!engine_->open(song->filePath)) {
        emit errorOccurred(QString::fromLatin1("Failed to reopen last song"));
        return;
    }
    if (state->position > 0.0)
        engine_->seek(state->position);
    emit positionChanged(engine_->currentPosition(), engine_->duration());
}

void PlayerController::persistSession(int playlistId) {
    if (!playerState_)
        return;
    PlayerState s;
    s.currentSongId = currentSongId_;
    s.playlistId = playlistId;
    s.position = engine_->currentPosition();
    playerState_->save(s);
}

void PlayerController::onPositionTick() {
    const AudioEngine::State now = engine_->state();
    if (now == AudioEngine::State::Playing) {
        emit positionChanged(engine_->currentPosition(), engine_->duration());
        // Threshold reached → record a play. Done once per song; reset on
        // openAndPlay so the next track re-arms the check.
        if (!historyRecorded_ && engine_->currentPosition() >= kHistoryThresholdSec)
            logHistoryIfWorthwhile();
    }

    if (now != lastSeenState_) {
        const bool fromPlaying = lastSeenState_ == AudioEngine::State::Playing;
        emitStateIfChanged(now);
        // Natural end: state slipped from Playing to Stopped without the user
        // asking. Record the play (if not already) and advance via PlayMode.
        if (fromPlaying && now == AudioEngine::State::Stopped && !userStopRequested_ &&
            engine_->eofReached()) {
            logHistoryIfWorthwhile();
            handleNaturalEnd();
        }
        userStopRequested_ = false;
    }
}

void PlayerController::emitStateIfChanged(AudioEngine::State current) {
    if (current == lastSeenState_)
        return;
    lastSeenState_ = current;
    emit stateChanged(static_cast<int>(current));
}

void PlayerController::handleNaturalEnd() {
    const auto next = queue_.advanceNatural();
    if (!next.has_value()) {
        positionTimer_.stop();
        return;
    }
    openAndPlay(*next);
}

bool PlayerController::openAndPlay(int songId, double startPosition) {
    const auto info = songs_->getSongById(songId);
    if (!info.has_value()) {
        emit errorOccurred(QString::fromLatin1("Song id %1 not found").arg(songId));
        return false;
    }
    if (!engine_->open(info->filePath)) {
        emit errorOccurred(
            QString::fromLatin1("Failed to open %1").arg(QString::fromStdString(info->filePath)));
        return false;
    }
    currentSongId_ = songId;
    historyRecorded_ = false;
    userStopRequested_ = false;
    if (startPosition > 0.0)
        engine_->seek(startPosition);
    engine_->play();
    if (!positionTimer_.isActive())
        positionTimer_.start();
    emit currentSongChanged(*info);
    emit positionChanged(engine_->currentPosition(), engine_->duration());
    emitStateIfChanged(engine_->state());
    return true;
}

void PlayerController::logHistoryIfWorthwhile() {
    if (historyRecorded_ || !history_ || currentSongId_ <= 0)
        return;
    history_->record(currentSongId_, engine_->currentPosition());
    historyRecorded_ = true;
}

}  // namespace musicplayer
