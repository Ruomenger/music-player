#include <gtest/gtest.h>
#include "audio_engine.h"
#include "ffmpeg_decoder.h"
#include "portaudio_output.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

using namespace musicplayer;

static std::string testDataPath(const std::string& filename) {
    return (std::filesystem::current_path() / "testdata" / filename).string();
}

static void createSilentWav(const std::string& path, double durationSec) {
    std::ofstream out(path, std::ios::binary);
    int sampleRate = 44100;
    int channels = 2;
    int byteRate = sampleRate * channels * 2;
    int dataSize = static_cast<int>(durationSec * byteRate);

    out.write("RIFF", 4);
    int32_t chunkSize = 36 + dataSize;
    out.write(reinterpret_cast<const char*>(&chunkSize), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    int32_t fmtSize = 16;
    out.write(reinterpret_cast<const char*>(&fmtSize), 4);
    int16_t audioFormat = 1;
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    int16_t numChannels = static_cast<int16_t>(channels);
    out.write(reinterpret_cast<const char*>(&numChannels), 2);
    int32_t sr = sampleRate;
    out.write(reinterpret_cast<const char*>(&sr), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    int16_t blockAlign = static_cast<int16_t>(channels * 2);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    int16_t bitsPerSample = 16;
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);
    std::vector<char> silence(static_cast<size_t>(dataSize), 0);
    out.write(silence.data(), static_cast<std::streamsize>(silence.size()));
}

TEST(AudioPipeline, OpenAndPlaySilence) {
    std::string path = testDataPath("test_pipeline.wav");
    std::filesystem::create_directories(testDataPath(""));
    createSilentWav(path, 0.5);

    AudioEngine engine;
    engine.setDecoder(std::make_unique<FfmpegDecoder>());
    engine.setOutput(std::make_unique<PortAudioOutput>());

    ASSERT_TRUE(engine.open(path));
    EXPECT_EQ(engine.state(), AudioEngine::State::Stopped);

    engine.play();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(engine.state(), AudioEngine::State::Playing);

    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    engine.stop();
    EXPECT_EQ(engine.state(), AudioEngine::State::Stopped);
}

TEST(AudioPipeline, PauseAndResume) {
    std::string path = testDataPath("test_pause.wav");
    createSilentWav(path, 1.0);

    AudioEngine engine;
    engine.setDecoder(std::make_unique<FfmpegDecoder>());
    engine.setOutput(std::make_unique<PortAudioOutput>());

    ASSERT_TRUE(engine.open(path));
    engine.play();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    engine.pause();
    EXPECT_EQ(engine.state(), AudioEngine::State::Paused);

    double pos = engine.currentPosition();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(engine.currentPosition(), pos);

    engine.play();
    EXPECT_EQ(engine.state(), AudioEngine::State::Playing);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    engine.stop();
}

TEST(AudioPipeline, Seek) {
    std::string path = testDataPath("test_seek_pipeline.wav");
    createSilentWav(path, 1.0);

    AudioEngine engine;
    engine.setDecoder(std::make_unique<FfmpegDecoder>());
    engine.setOutput(std::make_unique<PortAudioOutput>());

    ASSERT_TRUE(engine.open(path));
    engine.play();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    engine.seek(0.5);
    EXPECT_NEAR(engine.currentPosition(), 0.5, 0.1);

    engine.stop();
}
