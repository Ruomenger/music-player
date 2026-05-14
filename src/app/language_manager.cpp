#include "language_manager.h"

#include <QCoreApplication>
#include <QString>
#include <QtLogging>

namespace musicplayer {

LanguageManager::LanguageManager(QObject* parent) : QObject(parent) {}

bool LanguageManager::setLanguage(const QString& locale) {
    if (locale == current_)
        return true;

    // English is the source language: uninstalling falls back to the
    // literal tr() arguments, which are English.
    if (locale == QLatin1StringView("en")) {
        if (installed_) {
            QCoreApplication::removeTranslator(&translator_);
            installed_ = false;
        }
        current_ = locale;
        emit languageChanged(current_);
        return true;
    }

    if (installed_) {
        QCoreApplication::removeTranslator(&translator_);
        installed_ = false;
    }
    const QString qmPath = QStringLiteral(":/translations/musicplayer_%1.qm").arg(locale);
    if (!translator_.load(qmPath)) {
        qWarning("LanguageManager: failed to load translator %s", qmPath.toUtf8().constData());
        return false;
    }
    if (!QCoreApplication::installTranslator(&translator_)) {
        qWarning("LanguageManager: installTranslator(%s) returned false",
                 qmPath.toUtf8().constData());
        return false;
    }
    installed_ = true;
    current_ = locale;
    emit languageChanged(current_);
    return true;
}

}  // namespace musicplayer
