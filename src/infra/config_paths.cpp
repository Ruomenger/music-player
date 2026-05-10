#include "config_paths.h"

#include <QDir>
#include <QStandardPaths>

namespace musicplayer {

namespace {

std::string ensureDir(const QString& path) {
    if (path.isEmpty())
        return {};
    QDir().mkpath(path);
    return path.toStdString();
}

}  // namespace

std::string ConfigPaths::dataDir() {
    return ensureDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

std::string ConfigPaths::configDir() {
    return ensureDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
}

std::string ConfigPaths::cacheDir() {
    return ensureDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
}

}  // namespace musicplayer
