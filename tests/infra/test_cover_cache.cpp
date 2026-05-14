#include <gtest/gtest.h>
#include "cover_cache.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using musicplayer::CoverCache;

namespace {

// Minimal JPEG and PNG magic byte sequences. The cache only ever sniffs the
// header (it stores the bytes as-is), so we don't need a full image. Stored
// as constexpr std::array so the no-throw static-init lint is happy.
constexpr std::array<std::byte, 11> kJpegBytes = {
    std::byte{0xFF}, std::byte{0xD8}, std::byte{0xFF}, std::byte{0xE0},
    std::byte{'p'},  std::byte{'a'},  std::byte{'y'},  std::byte{'l'},
    std::byte{'o'},  std::byte{'a'},  std::byte{'d'},
};
constexpr std::array<std::byte, 10> kPngBytes = {
    std::byte{0x89}, std::byte{'P'},  std::byte{'N'},  std::byte{'G'}, std::byte{0x0D},
    std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A}, std::byte{'h'}, std::byte{'i'},
};

class CoverCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        cacheDir_ = fs::temp_directory_path() /
                    ("musicplayer_cover_" +
                     std::to_string(static_cast<int>(reinterpret_cast<std::uintptr_t>(this))));
        fs::remove_all(cacheDir_);
    }
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(cacheDir_, ec);
    }

    fs::path cacheDir_;
};

}  // namespace

TEST_F(CoverCacheTest, StoresJpegAndReturnsPath) {
    CoverCache cache(cacheDir_.string());
    const std::string path = cache.store(kJpegBytes.data(), kJpegBytes.size(), "image/jpeg");
    ASSERT_FALSE(path.empty());
    EXPECT_TRUE(fs::exists(path));
    EXPECT_EQ(fs::path(path).extension(), ".jpg");
}

TEST_F(CoverCacheTest, StoresPngAndReturnsPath) {
    CoverCache cache(cacheDir_.string());
    const std::string path = cache.store(kPngBytes.data(), kPngBytes.size(), "image/png");
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(fs::path(path).extension(), ".png");
}

TEST_F(CoverCacheTest, IdenticalBytesProduceSamePath) {
    CoverCache cache(cacheDir_.string());
    const auto a = cache.store(kJpegBytes.data(), kJpegBytes.size(), "image/jpeg");
    const auto b = cache.store(kJpegBytes.data(), kJpegBytes.size(), "image/jpeg");
    EXPECT_EQ(a, b);
}

TEST_F(CoverCacheTest, RejectsEmptyData) {
    CoverCache cache(cacheDir_.string());
    EXPECT_TRUE(cache.store(nullptr, 0, "image/jpeg").empty());
}

TEST_F(CoverCacheTest, RejectsUnknownMime) {
    CoverCache cache(cacheDir_.string());
    EXPECT_TRUE(cache.store(kJpegBytes.data(), kJpegBytes.size(), "image/webp").empty());
}
