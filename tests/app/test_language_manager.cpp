#include <gtest/gtest.h>
#include "language_manager.h"

#include <QSignalSpy>
#include <QString>

using musicplayer::LanguageManager;

// The compiled .qm files are attached to the `musicplayer` executable target
// via qt6_add_translations; test executables don't bundle them, so these
// cases assert the API contract without exercising the actual translation
// content. That coverage lives in the manual UI smoke test documented in
// docs/todo.md Phase 8 verification.

TEST(LanguageManagerTest, AvailableLanguagesIncludesEnAndZhCn) {
    const auto langs = LanguageManager::availableLanguages();
    EXPECT_TRUE(langs.contains(QStringLiteral("en")));
    EXPECT_TRUE(langs.contains(QStringLiteral("zh_CN")));
}

TEST(LanguageManagerTest, DefaultsToEnglish) {
    LanguageManager mgr;
    EXPECT_EQ(mgr.currentLanguage(), QStringLiteral("en"));
}

TEST(LanguageManagerTest, SetEnglishIsNoOpFromInitialState) {
    LanguageManager mgr;
    QSignalSpy spy(&mgr, &LanguageManager::languageChanged);
    EXPECT_TRUE(mgr.setLanguage(QStringLiteral("en")));
    EXPECT_EQ(spy.count(), 0);  // same locale → no signal
}

TEST(LanguageManagerTest, UnknownLocaleReturnsFalse) {
    LanguageManager mgr;
    EXPECT_FALSE(mgr.setLanguage(QStringLiteral("klingon")));
    EXPECT_EQ(mgr.currentLanguage(), QStringLiteral("en"));
}
