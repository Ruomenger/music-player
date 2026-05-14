#include "new_playlist_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace musicplayer {

NewPlaylistDialog::NewPlaylistDialog(QWidget* parent)
    : QDialog(parent), nameEdit_(new QLineEdit(this)), descEdit_(new QLineEdit(this)) {
    setWindowTitle(tr("New playlist"));
    setMinimumWidth(320);

    nameEdit_->setPlaceholderText(tr("Playlist name"));
    descEdit_->setPlaceholderText(tr("Description (optional)"));

    auto* form = new QFormLayout;
    form->addRow(tr("Name:"), nameEdit_);
    form->addRow(tr("Description:"), descEdit_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    // Keep OK disabled until there's something to commit; QLineEdit's
    // textChanged keeps it in sync as the user types.
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(nameEdit_, &QLineEdit::textChanged, this, [buttons](const QString& text) {
        buttons->button(QDialogButtonBox::Ok)->setEnabled(!text.trimmed().isEmpty());
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &NewPlaylistDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

QString NewPlaylistDialog::playlistName() const {
    return nameEdit_->text().trimmed();
}

QString NewPlaylistDialog::playlistDescription() const {
    return descEdit_->text().trimmed();
}

void NewPlaylistDialog::onAccept() {
    if (playlistName().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid name"), tr("Playlist name cannot be empty."));
        return;
    }
    accept();
}

std::optional<NewPlaylistDialog::Result> NewPlaylistDialog::run(QWidget* parent) {
    NewPlaylistDialog dlg(parent);
    if (dlg.exec() != QDialog::Accepted)
        return std::nullopt;
    return Result{dlg.playlistName(), dlg.playlistDescription()};
}

}  // namespace musicplayer
