#include "settings_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QVBoxLayout>

#include "portaudio_output.h"
#include "sqlite_settings_repo.h"

namespace musicplayer {

namespace {

constexpr int kHistoryDaysMin = 0;
constexpr int kHistoryDaysMax = 3650;
constexpr int kHistoryDaysDefault = 90;

}  // namespace

SettingsDialog::SettingsDialog(SqliteSettingsRepo* repo, QWidget* parent)
    : QDialog(parent)
    , repo_(repo)
    , form_(new QFormLayout)
    , languageLabel_(new QLabel(this))
    , deviceLabel_(new QLabel(this))
    , musicDirLabel_(new QLabel(this))
    , autoLyricLabel_(new QLabel(this))
    , historyLabel_(new QLabel(this))
    , languageCombo_(new QComboBox(this))
    , deviceCombo_(new QComboBox(this))
    , musicDirEdit_(new QLineEdit(this))
    , musicDirBrowse_(new QPushButton(tr("Browse…"), this))
    , autoLyricCheck_(new QCheckBox(this))
    , historyDaysSpin_(new QSpinBox(this)) {
    setMinimumWidth(420);
    setWindowTitle(tr("Settings"));

    // objectName lets tests look up this control without colliding with the
    // QLineEdit that QSpinBox creates as a private child.
    musicDirEdit_->setObjectName(QStringLiteral("musicDirEdit"));

    auto* dirRow = new QWidget(this);
    auto* dirLayout = new QHBoxLayout(dirRow);
    dirLayout->setContentsMargins(0, 0, 0, 0);
    dirLayout->addWidget(musicDirEdit_, /*stretch=*/1);
    dirLayout->addWidget(musicDirBrowse_);

    historyDaysSpin_->setRange(kHistoryDaysMin, kHistoryDaysMax);
    historyDaysSpin_->setSuffix(tr(" days"));

    form_->addRow(languageLabel_, languageCombo_);
    form_->addRow(deviceLabel_, deviceCombo_);
    form_->addRow(musicDirLabel_, dirRow);
    form_->addRow(autoLyricLabel_, autoLyricCheck_);
    form_->addRow(historyLabel_, historyDaysSpin_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(musicDirBrowse_, &QPushButton::clicked, this, &SettingsDialog::onBrowseMusicDir);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form_);
    layout->addWidget(buttons);

    retranslateUi();
    populateLanguageCombo();
    populateDeviceCombo();
    loadFromRepo();
}

void SettingsDialog::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QDialog::changeEvent(event);
}

void SettingsDialog::onBrowseMusicDir() {
    const QString start = musicDirEdit_->text();
    const QString picked =
        QFileDialog::getExistingDirectory(this, tr("Choose music folder"), start);
    if (!picked.isEmpty())
        musicDirEdit_->setText(picked);
}

void SettingsDialog::onAccept() {
    if (repo_ == nullptr) {
        accept();
        return;
    }
    repo_->setString("language", languageCombo_->currentData().toString().toStdString());
    // currentData() returns the device's actual name (empty for the default
    // row); storing the empty string preserves the "auto" path in main.cpp.
    repo_->setString("output_device", deviceCombo_->currentData().toString().toStdString());
    repo_->setString("music_dir", musicDirEdit_->text().toStdString());
    repo_->setBool("auto_load_lyric", autoLyricCheck_->isChecked());
    repo_->setInt("history_max_days", historyDaysSpin_->value());
    emit settingsApplied();
    accept();
}

void SettingsDialog::loadFromRepo() {
    if (repo_ == nullptr)
        return;
    const auto lang = repo_->getString("language").value_or(std::string{"en"});
    const int langIdx = languageCombo_->findData(QString::fromStdString(lang));
    if (langIdx >= 0)
        languageCombo_->setCurrentIndex(langIdx);

    const auto device = repo_->getString("output_device").value_or(std::string{});
    const int devIdx = deviceCombo_->findData(QString::fromStdString(device));
    deviceCombo_->setCurrentIndex(devIdx >= 0 ? devIdx : 0);

    musicDirEdit_->setText(QString::fromStdString(repo_->getString("music_dir").value_or("")));
    autoLyricCheck_->setChecked(repo_->getBool("auto_load_lyric").value_or(true));
    historyDaysSpin_->setValue(repo_->getInt("history_max_days").value_or(kHistoryDaysDefault));
}

void SettingsDialog::retranslateUi() {
    setWindowTitle(tr("Settings"));
    languageLabel_->setText(tr("Language:"));
    deviceLabel_->setText(tr("Output device:"));
    musicDirLabel_->setText(tr("Music folder:"));
    autoLyricLabel_->setText(tr("Auto-load lyrics:"));
    historyLabel_->setText(tr("History retention:"));
    musicDirBrowse_->setText(tr("Browse…"));
    historyDaysSpin_->setSuffix(tr(" days"));
}

void SettingsDialog::populateLanguageCombo() {
    languageCombo_->clear();
    // Display name → locale code stored in QComboBox::userData. The display
    // strings are translatable; the locale code is the stable storage key.
    languageCombo_->addItem(tr("English"), QStringLiteral("en"));
    languageCombo_->addItem(tr("简体中文"), QStringLiteral("zh_CN"));
}

void SettingsDialog::populateDeviceCombo() {
    deviceCombo_->clear();
    // The first row is "(default)" — empty user data so output resolution
    // falls back to Pa_GetDefaultOutputDevice() at open() time.
    deviceCombo_->addItem(tr("(default)"), QString());
    for (const auto& info : PortAudioOutput::enumerateOutputDevices()) {
        deviceCombo_->addItem(QString::fromStdString(info.name), QString::fromStdString(info.name));
    }
}

}  // namespace musicplayer
