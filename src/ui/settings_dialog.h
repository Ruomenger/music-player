#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace musicplayer {

class SqliteSettingsRepo;

// User-facing preferences pane. Reads current values from SqliteSettingsRepo
// at construction and writes them back on accept. Each control is wired to a
// well-known setting key (volume / language / output_device / music_dir /
// auto_load_lyric / history_max_days). MainWindow connects the
// settingsApplied() signal to re-apply the values to live services
// (LanguageManager / PortAudioOutput / LyricManager / SqlitePlayHistoryRepo).
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(SqliteSettingsRepo* repo, QWidget* parent = nullptr);

signals:
    void settingsApplied();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onBrowseMusicDir();
    void onAccept();

private:
    void loadFromRepo();
    void retranslateUi();
    void populateLanguageCombo();
    void populateDeviceCombo();

    SqliteSettingsRepo* repo_;

    QFormLayout* form_ = nullptr;
    QLabel* languageLabel_ = nullptr;
    QLabel* deviceLabel_ = nullptr;
    QLabel* musicDirLabel_ = nullptr;
    QLabel* autoLyricLabel_ = nullptr;
    QLabel* historyLabel_ = nullptr;

    QComboBox* languageCombo_ = nullptr;
    QComboBox* deviceCombo_ = nullptr;
    QLineEdit* musicDirEdit_ = nullptr;
    QPushButton* musicDirBrowse_ = nullptr;
    QCheckBox* autoLyricCheck_ = nullptr;
    QSpinBox* historyDaysSpin_ = nullptr;
};

}  // namespace musicplayer
