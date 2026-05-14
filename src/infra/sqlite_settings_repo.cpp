#include "sqlite_settings_repo.h"

#include <QLatin1StringView>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QtLogging>

#include <charconv>
#include <string>
#include <system_error>
#include <utility>

namespace musicplayer {

namespace {

QSqlDatabase conn(const std::string& name) {
    return QSqlDatabase::database(QString::fromStdString(name));
}

struct DefaultEntry {
    const char* key;
    const char* value;
    const char* valueType;
};

// Phase 2 presets per database-schema.md §settings.
constexpr DefaultEntry kDefaults[] = {
    {"volume", "0.8", "double"},       {"play_mode", "sequential", "string"},
    {"language", "zh_CN", "string"},   {"theme", "system", "string"},
    {"output_device", "", "string"},   {"music_dir", "", "string"},
    {"auto_load_lyric", "1", "bool"},  {"close_to_tray", "0", "bool"},
    {"history_max_days", "90", "int"},
};

}  // namespace

SqliteSettingsRepo::SqliteSettingsRepo(std::string connectionName)
    : connectionName_(std::move(connectionName)) {}

bool SqliteSettingsRepo::setRaw(const std::string& key, const std::string& value,
                                const char* valueType) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSettingsRepo::setRaw: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    QSqlQuery q(db);
    // INSERT OR REPLACE clears the previous row (so display_name resets to
    // default ''); use UPSERT to preserve any other columns added later.
    q.prepare(
        QLatin1StringView("INSERT INTO settings(key, value, value_type) "
                          "VALUES (:key, :value, :type) "
                          "ON CONFLICT(key) DO UPDATE SET "
                          "    value = excluded.value,"
                          "    value_type = excluded.value_type,"
                          "    updated_at = datetime('now')"));
    q.bindValue(":key", QString::fromStdString(key));
    q.bindValue(":value", QString::fromStdString(value));
    q.bindValue(":type", QLatin1StringView(valueType));
    if (!q.exec()) {
        qWarning("SqliteSettingsRepo::setRaw: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return false;
    }
    return true;
}

bool SqliteSettingsRepo::setString(const std::string& key, const std::string& value) {
    return setRaw(key, value, "string");
}

bool SqliteSettingsRepo::setInt(const std::string& key, int value) {
    return setRaw(key, std::to_string(value), "int");
}

bool SqliteSettingsRepo::setDouble(const std::string& key, double value) {
    // std::to_string truncates to 6 decimals; QString::number with 'g' is
    // round-trip safe for double.
    return setRaw(key, QString::number(value, 'g', 17).toStdString(), "double");
}

bool SqliteSettingsRepo::setBool(const std::string& key, bool value) {
    return setRaw(key, value ? "1" : "0", "bool");
}

std::optional<std::string> SqliteSettingsRepo::getString(const std::string& key) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSettingsRepo::getString: connection '%s' is not open",
                 connectionName_.c_str());
        return std::nullopt;
    }
    QSqlQuery q(db);
    q.prepare(QLatin1StringView("SELECT value FROM settings WHERE key = :key"));
    q.bindValue(":key", QString::fromStdString(key));
    if (!q.exec()) {
        qWarning("SqliteSettingsRepo::getString: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return std::nullopt;
    }
    if (!q.next())
        return std::nullopt;
    return q.value(0).toString().toStdString();
}

std::optional<int> SqliteSettingsRepo::getInt(const std::string& key) {
    auto s = getString(key);
    if (!s)
        return std::nullopt;
    int out = 0;
    auto [ptr, ec] = std::from_chars(s->data(), s->data() + s->size(), out);
    if (ec != std::errc{})
        return std::nullopt;
    return out;
}

std::optional<double> SqliteSettingsRepo::getDouble(const std::string& key) {
    auto s = getString(key);
    if (!s)
        return std::nullopt;
    bool ok = false;
    const double v = QString::fromStdString(*s).toDouble(&ok);
    if (!ok)
        return std::nullopt;
    return v;
}

std::optional<bool> SqliteSettingsRepo::getBool(const std::string& key) {
    auto s = getString(key);
    if (!s)
        return std::nullopt;
    if (*s == "1" || *s == "true")
        return true;
    if (*s == "0" || *s == "false")
        return false;
    return std::nullopt;
}

bool SqliteSettingsRepo::remove(const std::string& key) {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSettingsRepo::remove: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    QSqlQuery q(db);
    q.prepare(QLatin1StringView("DELETE FROM settings WHERE key = :key"));
    q.bindValue(":key", QString::fromStdString(key));
    if (!q.exec()) {
        qWarning("SqliteSettingsRepo::remove: failed: %s",
                 q.lastError().text().toUtf8().constData());
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool SqliteSettingsRepo::seedDefaults() {
    QSqlDatabase db = conn(connectionName_);
    if (!db.isOpen()) {
        qWarning("SqliteSettingsRepo::seedDefaults: connection '%s' is not open",
                 connectionName_.c_str());
        return false;
    }
    if (!db.transaction()) {
        qWarning("SqliteSettingsRepo::seedDefaults: BEGIN failed: %s",
                 db.lastError().text().toUtf8().constData());
        return false;
    }
    QSqlQuery q(db);
    q.prepare(
        QLatin1StringView("INSERT OR IGNORE INTO settings(key, value, value_type) "
                          "VALUES (:key, :value, :type)"));
    for (const auto& d : kDefaults) {
        q.bindValue(":key", QLatin1StringView(d.key));
        q.bindValue(":value", QLatin1StringView(d.value));
        q.bindValue(":type", QLatin1StringView(d.valueType));
        if (!q.exec()) {
            qWarning("SqliteSettingsRepo::seedDefaults: failed at '%s': %s", d.key,
                     q.lastError().text().toUtf8().constData());
            db.rollback();
            return false;
        }
    }
    if (!db.commit()) {
        qWarning("SqliteSettingsRepo::seedDefaults: COMMIT failed: %s",
                 db.lastError().text().toUtf8().constData());
        db.rollback();
        return false;
    }
    return true;
}

}  // namespace musicplayer
