#include <gtest/gtest.h>
#include "ffmpeg_decoder.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace musicplayer;

namespace {

std::string testDataPath(const std::string& filename) {
    std::filesystem::path p = std::filesystem::current_path() / "testdata" / filename;
    return p.string();
}

void createSilentWav(const std::string& path, double durationSec, int sampleRate = 44100,
                     int channels = 2) {
    std::ofstream out(path, std::ios::binary);
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
    auto numChannels = static_cast<int16_t>(channels);
    out.write(reinterpret_cast<const char*>(&numChannels), 2);
    int32_t sr = sampleRate;
    out.write(reinterpret_cast<const char*>(&sr), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    auto blockAlign = static_cast<int16_t>(channels * 2);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    int16_t bitsPerSample = 16;
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);

    std::vector<char> silence(static_cast<size_t>(dataSize), 0);
    out.write(silence.data(), static_cast<std::streamsize>(silence.size()));
}

}  // namespace

TEST(FfmpegDecoder, OpenMissingFileFails) {
    FfmpegDecoder dec;
    EXPECT_FALSE(dec.open("/nonexistent/file.mp3"));
}

TEST(FfmpegDecoder, OpenWavFile) {
    std::string path = testDataPath("test_silence.wav");
    std::filesystem::create_directories(testDataPath(""));
    createSilentWav(path, 0.5);

    FfmpegDecoder dec;
    ASSERT_TRUE(dec.open(path));

    auto info = dec.info();
    EXPECT_EQ(info.sampleRate, 44100);
    EXPECT_EQ(info.channels, 2);
    EXPECT_GT(info.duration, 0.0);

    dec.close();
}

TEST(FfmpegDecoder, DecodeProducesSamples) {
    std::string path = testDataPath("test_decode.wav");
    createSilentWav(path, 0.2);

    FfmpegDecoder dec;
    ASSERT_TRUE(dec.open(path));

    auto samples = dec.decode(1024);
    EXPECT_FALSE(samples.empty());

    dec.close();
}

TEST(FfmpegDecoder, SeekAndDecode) {
    std::string path = testDataPath("test_seek.wav");
    createSilentWav(path, 1.0);

    FfmpegDecoder dec;
    ASSERT_TRUE(dec.open(path));

    ASSERT_TRUE(dec.seek(0.5));
    auto samples = dec.decode(1024);
    EXPECT_FALSE(samples.empty());

    dec.close();
}

// Regression: previously initResampler hardcoded out_rate == in_rate, so
// setTargetSampleRate had no effect and the resampler couldn't actually
// convert sample rates.
TEST(FfmpegDecoder, ResamplesToTargetSampleRate) {
    std::string path = testDataPath("test_resample.wav");
    std::filesystem::create_directories(testDataPath(""));
    createSilentWav(path, 0.5, 44100, 2);

    FfmpegDecoder dec;
    dec.setTargetSampleRate(48000);
    ASSERT_TRUE(dec.open(path));

    auto info = dec.info();
    EXPECT_EQ(info.sampleRate, 48000);
    EXPECT_EQ(info.channels, 2);

    size_t totalFrames = 0;
    while (true) {
        auto chunk = dec.decode(4096);
        if (chunk.empty())
            break;
        totalFrames += chunk.size() / 2;
    }
    // Expected: 0.5s * 48000 = 24000 frames. Allow swr's startup latency.
    EXPECT_NEAR(static_cast<double>(totalFrames), 24000.0, 200.0);

    dec.close();
}

TEST(FfmpegDecoder, DecodeAfterEofReturnsEmpty) {
    std::string path = testDataPath("test_eof.wav");
    createSilentWav(path, 0.1);

    FfmpegDecoder dec;
    ASSERT_TRUE(dec.open(path));

    while (true) {
        auto samples = dec.decode(4096);
        if (samples.empty())
            break;
    }

    auto empty = dec.decode(1024);
    EXPECT_TRUE(empty.empty());
    dec.close();
}
