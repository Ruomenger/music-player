#include <gtest/gtest.h>
#include "settings_dialog.h"
#include "sqlite_database.h"
#include "sqlite_settings_repo.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>

#include <atomic>
#include <memory>
#include <string>

using musicplayer::SettingsDialog;
using musicplayer::SqliteDatabase;
using musicplayer::SqliteSettingsRepo;

namespace {

std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

class SettingsDialogTest : public ::testing::Test {
protected:
    void SetUp() override {
        connName_ = uniqueConn("settings_dialog");
        db_ = std::make_unique<SqliteDatabase>(connName_);
        ASSERT_TRUE(db_->open(":memory:"));
        repo_ = std::make_unique<SqliteSettingsRepo>(connName_);
        repo_->seedDefaults();
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

TEST_F(SettingsDialogTest, LoadsCurrentSettingsIntoForm) {
    repo_->setString("language", "zh_CN");
    repo_->setBool("auto_load_lyric", false);
    repo_->setInt("history_max_days", 30);
    repo_->setString("music_dir", "/Users/me/Music");

    SettingsDialog dlg(repo_.get());
    auto* langCombo = dlg.findChildren<QComboBox*>().first();
    EXPECT_EQ(langCombo->currentData().toString(), QStringLiteral("zh_CN"));

    auto* checkBox = dlg.findChild<QCheckBox*>();
    ASSERT_NE(checkBox, nullptr);
    EXPECT_FALSE(checkBox->isChecked());

    auto* spin = dlg.findChild<QSpinBox*>();
    ASSERT_NE(spin, nullptr);
    EXPECT_EQ(spin->value(), 30);

    // QSpinBox keeps its own private QLineEdit; look up by name so we
    // grab the music dir field, not the spin's editor.
    auto* lineEdit = dlg.findChild<QLineEdit*>(QStringLiteral("musicDirEdit"));
    ASSERT_NE(lineEdit, nullptr);
    EXPECT_EQ(lineEdit->text(), QStringLiteral("/Users/me/Music"));
}

TEST_F(SettingsDialogTest, AcceptWritesBackToRepoAndEmits) {
    SettingsDialog dlg(repo_.get());

    // Edit a few fields then trigger accept.
    auto* langCombo = dlg.findChildren<QComboBox*>().first();
    const int zhIdx = langCombo->findData(QStringLiteral("zh_CN"));
    ASSERT_GE(zhIdx, 0);
    langCombo->setCurrentIndex(zhIdx);

    auto* spin = dlg.findChild<QSpinBox*>();
    spin->setValue(7);

    auto* checkBox = dlg.findChild<QCheckBox*>();
    checkBox->setChecked(false);

    QSignalSpy spy(&dlg, &SettingsDialog::settingsApplied);
    // Trigger the OK button.
    auto* buttons = dlg.findChild<QDialogButtonBox*>();
    ASSERT_NE(buttons, nullptr);
    buttons->button(QDialogButtonBox::Ok)->click();

    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(repo_->getString("language").value_or(""), "zh_CN");
    EXPECT_EQ(repo_->getInt("history_max_days").value_or(-1), 7);
    EXPECT_FALSE(repo_->getBool("auto_load_lyric").value_or(true));
}

TEST_F(SettingsDialogTest, LanguageComboHasBothLocales) {
    SettingsDialog dlg(repo_.get());
    auto* langCombo = dlg.findChildren<QComboBox*>().first();
    EXPECT_GE(langCombo->findData(QStringLiteral("en")), 0);
    EXPECT_GE(langCombo->findData(QStringLiteral("zh_CN")), 0);
}

TEST_F(SettingsDialogTest, DeviceComboHasDefaultRow) {
    SettingsDialog dlg(repo_.get());
    auto combos = dlg.findChildren<QComboBox*>();
    ASSERT_GE(combos.size(), 2);
    // Device combo is the second one. First row's userData is the empty
    // string (the "(default)" sentinel).
    auto* deviceCombo = combos.at(1);
    ASSERT_GT(deviceCombo->count(), 0);
    EXPECT_EQ(deviceCombo->itemData(0).toString(), QString());
}
