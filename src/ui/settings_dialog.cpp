#include "settings_dialog.h"

namespace musicplayer {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Settings"));
    setMinimumSize(400, 300);
}

}  // namespace musicplayer
