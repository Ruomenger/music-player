#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTranslator>

namespace musicplayer {

// Owns the QTranslator that drives the app's current UI language. Swapping
// languages installs / removes the translator on the running QApplication;
// Qt then dispatches QEvent::LanguageChange to every widget so each
// retranslateUi() override re-renders its strings.
//
// English is the source language (the literal arg to tr()), so loading the
// "en" locale uninstalls any translator. Other locales load the matching
// .qm file from ":/translations/musicplayer_<locale>.qm" (bundled via the
// resources/translations.qrc).
class LanguageManager : public QObject {
    Q_OBJECT
public:
    explicit LanguageManager(QObject* parent = nullptr);

    // "zh_CN" or "en". Returns true if the requested locale's translator
    // was loaded (or, for "en", successfully removed).
    bool setLanguage(const QString& locale);

    [[nodiscard]] QString currentLanguage() const { return current_; }
    [[nodiscard]] static QStringList availableLanguages() { return {"en", "zh_CN"}; }

signals:
    void languageChanged(const QString& locale);

private:
    QTranslator translator_;
    QString current_ = QStringLiteral("en");
    bool installed_ = false;
};

}  // namespace musicplayer
