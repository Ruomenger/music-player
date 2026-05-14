#include <gtest/gtest.h>
#include "new_playlist_dialog.h"

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QString>

using musicplayer::NewPlaylistDialog;

TEST(NewPlaylistDialogTest, OkButtonStartsDisabled) {
    NewPlaylistDialog dlg;
    auto* buttonBox = dlg.findChild<QDialogButtonBox*>();
    ASSERT_NE(buttonBox, nullptr);
    EXPECT_FALSE(buttonBox->button(QDialogButtonBox::Ok)->isEnabled());
}

TEST(NewPlaylistDialogTest, OkButtonEnablesWhenNamePresent) {
    NewPlaylistDialog dlg;
    auto* buttonBox = dlg.findChild<QDialogButtonBox*>();
    ASSERT_NE(buttonBox, nullptr);
    // First QLineEdit is the name field by layout order.
    auto* nameEdit = dlg.findChildren<QLineEdit*>().first();
    nameEdit->setText(QStringLiteral("My mix"));
    EXPECT_TRUE(buttonBox->button(QDialogButtonBox::Ok)->isEnabled());
}

TEST(NewPlaylistDialogTest, WhitespaceOnlyNameKeepsOkDisabled) {
    NewPlaylistDialog dlg;
    auto* buttonBox = dlg.findChild<QDialogButtonBox*>();
    auto* nameEdit = dlg.findChildren<QLineEdit*>().first();
    nameEdit->setText(QStringLiteral("   "));
    EXPECT_FALSE(buttonBox->button(QDialogButtonBox::Ok)->isEnabled());
}

TEST(NewPlaylistDialogTest, GettersTrimWhitespace) {
    NewPlaylistDialog dlg;
    auto edits = dlg.findChildren<QLineEdit*>();
    ASSERT_EQ(edits.size(), 2);
    edits[0]->setText(QStringLiteral("  Mix  "));
    edits[1]->setText(QStringLiteral("  desc  "));
    EXPECT_EQ(dlg.playlistName(), QStringLiteral("Mix"));
    EXPECT_EQ(dlg.playlistDescription(), QStringLiteral("desc"));
}
