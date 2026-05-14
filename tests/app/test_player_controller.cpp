#include <gtest/gtest.h>
#include "audio_engine.h"
#include "iaudio_decoder.h"
#include "iaudio_output.h"
#include "player_controller.h"
#include "player_state_repo.h"
#include "song_info.h"
#include "sqlite_database.h"
#include "sqlite_play_history_repo.h"
#include "sqlite_song_repo.h"

#include <QSignalSpy>

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

using musicplayer::AudioDecoderInfo;
using musicplayer::AudioEngine;
using musicplayer::IAudioDecoder;
using musicplayer::IAudioOutput;
using musicplayer::PlayerController;
using musicplayer::PlayerStateRepo;
using musicplayer::PlayMode;
using musicplayer::SongInfo;
using musicplayer::SqliteDatabase;
using musicplayer::SqlitePlayHistoryRepo;
using musicplayer::SqliteSongRepo;

namespace {

// Minimal decoder that pretends to open any file, reports a fixed shape, and
// produces silent frames forever. Forever-frames keeps the engine in Playing
// state (no natural-end auto-advance), which is what the API tests want.
class FakeDecoder : public IAudioDecoder {
public:
    bool open(const std::string&) override {
        info_ = AudioDecoderInfo{};
        info_.sampleRate = 48000;
        info_.channels = 2;
        info_.duration = 60.0;
        info_.codecName = "fake";
        info_.formatName = "fake";
        return true;
    }
    std::vector<float> decode(std::size_t maxFrames) override {
        return std::vector<float>(maxFrames * 2, 0.0F);
    }
    bool seek(double) override { return true; }
    void close() override {}
    [[nodiscard]] AudioDecoderInfo info() const override { return info_; }
    void setTargetSampleRate(int) override {}

private:
    AudioDecoderInfo info_;
};

// Output stub: never actually drives the callback, so the engine's state
// transitions only happen via the explicit play/pause/stop API the tests use.
class FakeOutput : public IAudioOutput {
public:
    bool open(double, int) override { return true; }
    [[nodiscard]] double defaultSampleRate() const override { return 48000.0; }
    bool start() override { return true; }
    bool pause() override { return true; }
    bool stop() override { return true; }
    void setCallback(DataCallback /*cb*/) override {}
};

std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

class PlayerControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        connName_ = uniqueConn("player_ctrl");
        db_ = std::make_unique<SqliteDatabase>(connName_);
        ASSERT_TRUE(db_->open(":memory:"));
        songs_ = std::make_unique<SqliteSongRepo>(connName_);
        history_ = std::make_unique<SqlitePlayHistoryRepo>(connName_);
        playerState_ = std::make_unique<PlayerStateRepo>(connName_);

        engine_ = std::make_unique<AudioEngine>();
        engine_->setDecoder(std::make_unique<FakeDecoder>());
        engine_->setOutput(std::make_unique<FakeOutput>());

        controller_ = std::make_unique<PlayerController>(engine_.get(), songs_.get(),
                                                         history_.get(), playerState_.get());

        // Pre-populate three songs whose files don't exist on disk — the
        // fake decoder doesn't actually open them, so paths can be fictional.
        for (int i = 1; i <= 3; ++i) {
            SongInfo s;
            s.title = "Track " + std::to_string(i);
            s.filePath = "/fake/track" + std::to_string(i) + ".mp3";
            s.duration = 60.0;
            s.format = "mp3";
            ids_.push_back(songs_->insertSong(s));
        }
    }

    void TearDown() override {
        controller_.reset();
        engine_.reset();
        playerState_.reset();
        history_.reset();
        songs_.reset();
        db_.reset();
    }

    std::string connName_;
    std::unique_ptr<SqliteDatabase> db_;
    std::unique_ptr<SqliteSongRepo> songs_;
    std::unique_ptr<SqlitePlayHistoryRepo> history_;
    std::unique_ptr<PlayerStateRepo> playerState_;
    std::unique_ptr<AudioEngine> engine_;
    std::unique_ptr<PlayerController> controller_;
    std::vector<int> ids_;
};

}  // namespace

TEST_F(PlayerControllerTest, PlayEmitsCurrentSongAndPosition) {
    QSignalSpy currentSpy(controller_.get(), &PlayerController::currentSongChanged);
    QSignalSpy positionSpy(controller_.get(), &PlayerController::positionChanged);

    controller_->play(ids_[0]);
    EXPECT_EQ(currentSpy.count(), 1);
    EXPECT_GE(positionSpy.count(), 1);
    EXPECT_EQ(controller_->currentSongId(), ids_[0]);
    EXPECT_EQ(controller_->state(), AudioEngine::State::Playing);
    controller_->stop();
}

TEST_F(PlayerControllerTest, PauseAndResume) {
    controller_->play(ids_[0]);
    QSignalSpy stateSpy(controller_.get(), &PlayerController::stateChanged);
    controller_->pause();
    EXPECT_EQ(controller_->state(), AudioEngine::State::Paused);
    controller_->resume();
    EXPECT_EQ(controller_->state(), AudioEngine::State::Playing);
    EXPECT_GE(stateSpy.count(), 2);  // at least Paused then Playing
    controller_->stop();
}

TEST_F(PlayerControllerTest, NextAdvancesQueue) {
    QSignalSpy currentSpy(controller_.get(), &PlayerController::currentSongChanged);
    controller_->playQueue(ids_, 0);
    EXPECT_EQ(controller_->currentSongId(), ids_[0]);
    controller_->next();
    EXPECT_EQ(controller_->currentSongId(), ids_[1]);
    controller_->next();
    EXPECT_EQ(controller_->currentSongId(), ids_[2]);
    EXPECT_EQ(currentSpy.count(), 3);  // one per openAndPlay
    controller_->stop();
}

TEST_F(PlayerControllerTest, PreviousWalksBack) {
    controller_->playQueue(ids_, 2);
    controller_->previous();
    EXPECT_EQ(controller_->currentSongId(), ids_[1]);
    controller_->previous();
    EXPECT_EQ(controller_->currentSongId(), ids_[0]);
    controller_->stop();
}

TEST_F(PlayerControllerTest, SetVolumeUpdatesEngineAndEmits) {
    QSignalSpy spy(controller_.get(), &PlayerController::volumeChanged);
    controller_->setVolume(0.42);
    // engine_->volume() goes through atomic<float>, so we lose double precision
    // — assert with a tolerance instead of exact equality.
    EXPECT_NEAR(controller_->volume(), 0.42, 1e-6);
    EXPECT_NEAR(engine_->volume(), 0.42, 1e-6);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy.takeFirst().at(0).toDouble(), 0.42);
}

TEST_F(PlayerControllerTest, SetVolumeClampsOutOfRange) {
    controller_->setVolume(2.0);
    EXPECT_DOUBLE_EQ(controller_->volume(), 1.0);
    controller_->setVolume(-1.0);
    EXPECT_DOUBLE_EQ(controller_->volume(), 0.0);
}

TEST_F(PlayerControllerTest, SetPlayModeEmitsOnlyOnChange) {
    QSignalSpy spy(controller_.get(), &PlayerController::playModeChanged);
    controller_->setPlayMode(PlayMode::ListLoop);
    EXPECT_EQ(controller_->playMode(), PlayMode::ListLoop);
    EXPECT_EQ(spy.count(), 1);
    controller_->setPlayMode(PlayMode::ListLoop);  // same mode → no signal
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(PlayerControllerTest, PersistAndRestoreSession) {
    controller_->play(ids_[1]);
    controller_->seek(7.5);
    controller_->persistSession(/*playlistId=*/0);
    controller_->stop();

    // Fresh controller backed by the same DB should pick the session up.
    AudioEngine engine2;
    engine2.setDecoder(std::make_unique<FakeDecoder>());
    engine2.setOutput(std::make_unique<FakeOutput>());
    PlayerController c2(&engine2, songs_.get(), history_.get(), playerState_.get());
    QSignalSpy currentSpy(&c2, &PlayerController::currentSongChanged);
    c2.restoreLastSession();
    EXPECT_EQ(c2.currentSongId(), ids_[1]);
    ASSERT_EQ(currentSpy.count(), 1);
}

TEST_F(PlayerControllerTest, StopEmitsStateChange) {
    controller_->play(ids_[0]);
    QSignalSpy spy(controller_.get(), &PlayerController::stateChanged);
    controller_->stop();
    EXPECT_EQ(controller_->state(), AudioEngine::State::Stopped);
    EXPECT_GE(spy.count(), 1);
}
