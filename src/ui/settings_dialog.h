#pragma once

#include <QDialog>

namespace musicplayer {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
};

} // namespace musicplayer
