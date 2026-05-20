#include <gtest/gtest.h>
#include "metadata_extractor.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using musicplayer::MetadataExtractor;

namespace {

// Minimal but valid silent PCM WAV. Container has no metadata tags, but we
// can still verify the codec / format / sample-rate / channel fields.
void writeSilentWav(const fs::path& path, double durationSec) {
    std::ofstream out(path, std::ios::binary);
    const int sampleRate = 44100;
    const int channels = 2;
    const int byteRate = sampleRate * channels * 2;
    const auto dataSize = static_cast<int>(durationSec * byteRate);

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
    std::vector<char> silence(static_cast<std::size_t>(dataSize), 0);
    out.write(silence.data(), static_cast<std::streamsize>(silence.size()));
}

// Per-process seeded RNG — see test_library_importer.cpp for the race story.
std::uint64_t randomTag() {
    static std::mt19937_64 rng{std::random_device{}()};
    return rng();
}

class MetadataExtractorTest : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = fs::temp_directory_path() / ("musicplayer_meta_" + std::to_string(randomTag()));
        fs::create_directories(root_);
    }
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(root_, ec);
    }

    fs::path root_;
};

}  // namespace

TEST_F(MetadataExtractorTest, ExtractsCodecAndFormatFromSilentWav) {
    const auto wav = root_ / "silent.wav";
    writeSilentWav(wav, 0.5);

    auto md = MetadataExtractor::extract(wav.string());
    ASSERT_TRUE(md.has_value());
    EXPECT_NEAR(md->duration, 0.5, 0.05);
    EXPECT_EQ(md->channels, 2);
    EXPECT_EQ(md->sampleRate, 44100);
    EXPECT_FALSE(md->format.empty());
    EXPECT_TRUE(md->coverData.empty());
}

TEST_F(MetadataExtractorTest, NonexistentFileReturnsNullopt) {
    const auto path = (root_ / "no_such_file.mp3").string();
    EXPECT_FALSE(MetadataExtractor::extract(path).has_value());
}

TEST_F(MetadataExtractorTest, NonAudioFileReturnsNullopt) {
    const auto txt = root_ / "junk.txt";
    {
        std::ofstream out(txt);
        out << "this is not audio";
    }
    EXPECT_FALSE(MetadataExtractor::extract(txt.string()).has_value());
}
