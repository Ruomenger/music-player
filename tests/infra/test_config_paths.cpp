#include <gtest/gtest.h>
#include "config_paths.h"

#include <QDir>
#include <QStandardPaths>

#include <filesystem>

using musicplayer::ConfigPaths;

namespace {

class ConfigPathsTest : public ::testing::Test {
protected:
    void SetUp() override { QStandardPaths::setTestModeEnabled(true); }
    void TearDown() override { QStandardPaths::setTestModeEnabled(false); }
};

}  // namespace

TEST_F(ConfigPathsTest, DataDirIsNonEmptyAndExists) {
    const std::string dir = ConfigPaths::dataDir();
    ASSERT_FALSE(dir.empty());
    EXPECT_TRUE(std::filesystem::exists(dir));
    EXPECT_TRUE(std::filesystem::is_directory(dir));
}

TEST_F(ConfigPathsTest, ConfigDirIsNonEmptyAndExists) {
    const std::string dir = ConfigPaths::configDir();
    ASSERT_FALSE(dir.empty());
    EXPECT_TRUE(std::filesystem::exists(dir));
    EXPECT_TRUE(std::filesystem::is_directory(dir));
}

TEST_F(ConfigPathsTest, CacheDirIsNonEmptyAndExists) {
    const std::string dir = ConfigPaths::cacheDir();
    ASSERT_FALSE(dir.empty());
    EXPECT_TRUE(std::filesystem::exists(dir));
    EXPECT_TRUE(std::filesystem::is_directory(dir));
}
