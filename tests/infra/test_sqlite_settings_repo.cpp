#include <gtest/gtest.h>
#include "sqlite_database.h"
#include "sqlite_settings_repo.h"

#include <atomic>
#include <string>

using musicplayer::SqliteDatabase;
using musicplayer::SqliteSettingsRepo;

namespace {

std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

class SqliteSettingsRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        connName_ = uniqueConn("settings_repo");
        db_ = std::make_unique<SqliteDatabase>(connName_);
        ASSERT_TRUE(db_->open(":memory:"));
        repo_ = std::make_unique<SqliteSettingsRepo>(connName_);
    }

    void TearDown() override {
        repo_.reset();
        db_.reset();
    }

    std::string connName_;
    std::unique_ptr<SqliteDatabase> db_;
    std::unique_ptr<SqliteSettingsRepo> repo_;
};

}  // namespace

TEST_F(SqliteSettingsRepoTest, StringRoundTrip) {
    ASSERT_TRUE(repo_->setString("theme", "dark"));
    auto v = repo_->getString("theme");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "dark");
}

TEST_F(SqliteSettingsRepoTest, IntRoundTrip) {
    ASSERT_TRUE(repo_->setInt("history_max_days", 30));
    auto v = repo_->getInt("history_max_days");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 30);
}

TEST_F(SqliteSettingsRepoTest, DoubleRoundTripIsLossless) {
    constexpr double kInput = 0.123456789012345;
    ASSERT_TRUE(repo_->setDouble("volume", kInput));
    auto v = repo_->getDouble("volume");
    ASSERT_TRUE(v.has_value());
    EXPECT_DOUBLE_EQ(*v, kInput);
}

TEST_F(SqliteSettingsRepoTest, BoolRoundTrip) {
    ASSERT_TRUE(repo_->setBool("auto_load_lyric", true));
    auto t = repo_->getBool("auto_load_lyric");
    ASSERT_TRUE(t.has_value());
    EXPECT_TRUE(*t);

    ASSERT_TRUE(repo_->setBool("auto_load_lyric", false));
    auto f = repo_->getBool("auto_load_lyric");
    ASSERT_TRUE(f.has_value());
    EXPECT_FALSE(*f);
}

TEST_F(SqliteSettingsRepoTest, GetMissingReturnsNullopt) {
    EXPECT_FALSE(repo_->getString("nope").has_value());
    EXPECT_FALSE(repo_->getInt("nope").has_value());
}

TEST_F(SqliteSettingsRepoTest, OverwriteReplacesValue) {
    ASSERT_TRUE(repo_->setString("language", "zh_CN"));
    ASSERT_TRUE(repo_->setString("language", "en_US"));
    auto v = repo_->getString("language");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "en_US");
}

TEST_F(SqliteSettingsRepoTest, RemoveDeletesEntry) {
    ASSERT_TRUE(repo_->setString("transient", "value"));
    EXPECT_TRUE(repo_->remove("transient"));
    EXPECT_FALSE(repo_->getString("transient").has_value());
    EXPECT_FALSE(repo_->remove("transient"));  // already gone
}

TEST_F(SqliteSettingsRepoTest, SeedDefaultsInsertsPresetsOnce) {
    ASSERT_TRUE(repo_->seedDefaults());
    auto volume = repo_->getDouble("volume");
    ASSERT_TRUE(volume.has_value());
    EXPECT_DOUBLE_EQ(*volume, 0.8);
    auto mode = repo_->getString("play_mode");
    ASSERT_TRUE(mode.has_value());
    EXPECT_EQ(*mode, "sequential");

    // Second invocation must not clobber user overrides.
    ASSERT_TRUE(repo_->setDouble("volume", 0.25));
    ASSERT_TRUE(repo_->seedDefaults());
    auto after = repo_->getDouble("volume");
    ASSERT_TRUE(after.has_value());
    EXPECT_DOUBLE_EQ(*after, 0.25);
}
