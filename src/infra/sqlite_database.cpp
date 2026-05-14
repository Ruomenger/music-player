#include "sqlite_database.h"

#include <QLatin1StringView>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QtLogging>

#include <utility>

namespace musicplayer {

namespace {

constexpr const char* kDriver = "QSQLITE";

// PRAGMAs are per-connection in SQLite — these run every time we open().
constexpr const char* kPragmas[] = {
    "PRAGMA journal_mode = WAL", "PRAGMA foreign_keys = ON",   "PRAGMA synchronous = NORMAL",
    "PRAGMA cache_size = -8000", "PRAGMA temp_store = MEMORY", "PRAGMA busy_timeout = 5000",
};

// Phase 2 baseline schema. Idempotent: CREATE TABLE IF NOT EXISTS / CREATE
// INDEX IF NOT EXISTS / CREATE TRIGGER IF NOT EXISTS. No version table yet —
// the next migration (e.g. FTS5 in Phase 9) will introduce schema_version.
constexpr const char* kSchemaStatements[] = {
    // ── songs ──────────────────────────────────────────────────────────────
    "CREATE TABLE IF NOT EXISTS songs ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    file_path TEXT NOT NULL UNIQUE,"
    "    title TEXT NOT NULL,"
    "    artist TEXT NOT NULL DEFAULT '',"
    "    album TEXT NOT NULL DEFAULT '',"
    "    album_artist TEXT NOT NULL DEFAULT '',"
    "    composer TEXT NOT NULL DEFAULT '',"
    "    genre TEXT NOT NULL DEFAULT '',"
    "    year INTEGER NOT NULL DEFAULT 0,"
    "    track_number INTEGER NOT NULL DEFAULT 0,"
    "    disc_number INTEGER NOT NULL DEFAULT 0,"
    "    duration REAL NOT NULL DEFAULT 0,"
    "    file_size INTEGER NOT NULL DEFAULT 0,"
    "    file_mtime INTEGER NOT NULL DEFAULT 0,"
    "    fingerprint TEXT NOT NULL DEFAULT '',"
    "    format TEXT NOT NULL DEFAULT '',"
    "    bitrate INTEGER NOT NULL DEFAULT 0,"
    "    sample_rate INTEGER NOT NULL DEFAULT 0,"
    "    channels INTEGER NOT NULL DEFAULT 2,"
    "    cover_path TEXT NOT NULL DEFAULT '',"
    "    lyric_path TEXT NOT NULL DEFAULT '',"
    "    lyric_source TEXT NOT NULL DEFAULT 'none'"
    "        CHECK (lyric_source IN ('none','embedded','lrc_file','manual')),"
    "    added_at TEXT NOT NULL DEFAULT (datetime('now')),"
    "    updated_at TEXT NOT NULL DEFAULT (datetime('now'))"
    ")",
    "CREATE INDEX IF NOT EXISTS idx_songs_artist ON songs(artist)",
    "CREATE INDEX IF NOT EXISTS idx_songs_album ON songs(album, disc_number, track_number)",
    "CREATE INDEX IF NOT EXISTS idx_songs_year ON songs(year)",
    "CREATE INDEX IF NOT EXISTS idx_songs_fingerprint ON songs(fingerprint) "
    "    WHERE fingerprint != ''",
    "CREATE TRIGGER IF NOT EXISTS trg_songs_updated_at "
    "AFTER UPDATE ON songs FOR EACH ROW "
    "WHEN NEW.updated_at = OLD.updated_at "
    "BEGIN "
    "    UPDATE songs SET updated_at = datetime('now') WHERE id = NEW.id; "
    "END",
    // ── playlists ──────────────────────────────────────────────────────────
    "CREATE TABLE IF NOT EXISTS playlists ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    name TEXT NOT NULL,"
    "    description TEXT NOT NULL DEFAULT '',"
    "    cover_path TEXT NOT NULL DEFAULT '',"
    "    is_system INTEGER NOT NULL DEFAULT 0,"
    "    sort_order INTEGER NOT NULL DEFAULT 0,"
    "    created_at TEXT NOT NULL DEFAULT (datetime('now')),"
    "    updated_at TEXT NOT NULL DEFAULT (datetime('now'))"
    ")",
    // ── playlist_songs ─────────────────────────────────────────────────────
    "CREATE TABLE IF NOT EXISTS playlist_songs ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    playlist_id INTEGER NOT NULL,"
    "    song_id INTEGER NOT NULL,"
    "    sort_index INTEGER NOT NULL DEFAULT 0,"
    "    added_at TEXT NOT NULL DEFAULT (datetime('now')),"
    "    FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,"
    "    FOREIGN KEY (song_id) REFERENCES songs(id) ON DELETE CASCADE,"
    "    UNIQUE(playlist_id, song_id)"
    ")",
    "CREATE INDEX IF NOT EXISTS idx_ps_playlist ON playlist_songs(playlist_id, sort_index)",
    // ── play_history ───────────────────────────────────────────────────────
    "CREATE TABLE IF NOT EXISTS play_history ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    song_id INTEGER NOT NULL,"
    "    played_at TEXT NOT NULL DEFAULT (datetime('now')),"
    "    progress REAL NOT NULL DEFAULT 0,"
    "    FOREIGN KEY (song_id) REFERENCES songs(id) ON DELETE CASCADE"
    ")",
    "CREATE INDEX IF NOT EXISTS idx_history_song ON play_history(song_id)",
    "CREATE INDEX IF NOT EXISTS idx_history_time ON play_history(played_at DESC)",
    // ── settings ───────────────────────────────────────────────────────────
    "CREATE TABLE IF NOT EXISTS settings ("
    "    key TEXT PRIMARY KEY,"
    "    value TEXT NOT NULL,"
    "    value_type TEXT NOT NULL DEFAULT 'string',"
    "    display_name TEXT NOT NULL DEFAULT '',"
    "    updated_at TEXT NOT NULL DEFAULT (datetime('now'))"
    ")",
    // ── player_state (single row, id=1) ────────────────────────────────────
    "CREATE TABLE IF NOT EXISTS player_state ("
    "    id INTEGER PRIMARY KEY CHECK (id = 1),"
    "    current_song_id INTEGER,"
    "    playlist_id INTEGER,"
    "    position REAL NOT NULL DEFAULT 0,"
    "    FOREIGN KEY (current_song_id) REFERENCES songs(id) ON DELETE SET NULL,"
    "    FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE SET NULL"
    ")",
};

}  // namespace

SqliteDatabase::SqliteDatabase(std::string connectionName)
    : connectionName_(std::move(connectionName)) {}

SqliteDatabase::~SqliteDatabase() {
    close();
}

bool SqliteDatabase::open(const std::string& dbPath) {
    if (opened_) {
        qWarning("SqliteDatabase::open: connection '%s' already open", connectionName_.c_str());
        return false;
    }
    const QString connName = QString::fromStdString(connectionName_);
    if (QSqlDatabase::contains(connName)) {
        // Stale connection from a previous instance; remove before recreating.
        QSqlDatabase::removeDatabase(connName);
    }
    QSqlDatabase db = QSqlDatabase::addDatabase(QLatin1StringView(kDriver), connName);
    db.setDatabaseName(QString::fromStdString(dbPath));
    if (!db.open()) {
        qWarning("SqliteDatabase::open: failed to open '%s': %s", dbPath.c_str(),
                 db.lastError().text().toUtf8().constData());
        QSqlDatabase::removeDatabase(connName);
        return false;
    }
    opened_ = true;
    if (!applyPragmas() || !applySchema()) {
        close();
        return false;
    }
    return true;
}

void SqliteDatabase::close() {
    if (!opened_)
        return;
    const QString connName = QString::fromStdString(connectionName_);
    {
        QSqlDatabase db = QSqlDatabase::database(connName, /*open=*/false);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(connName);
    opened_ = false;
}

bool SqliteDatabase::applyPragmas() {
    QSqlDatabase db = QSqlDatabase::database(QString::fromStdString(connectionName_));
    QSqlQuery q(db);
    for (const char* sql : kPragmas) {
        if (!q.exec(QLatin1StringView(sql))) {
            qWarning("SqliteDatabase::applyPragmas: '%s' failed: %s", sql,
                     q.lastError().text().toUtf8().constData());
            return false;
        }
    }
    return true;
}

bool SqliteDatabase::applySchema() {
    QSqlDatabase db = QSqlDatabase::database(QString::fromStdString(connectionName_));
    if (!db.transaction()) {
        qWarning("SqliteDatabase::applySchema: BEGIN failed: %s",
                 db.lastError().text().toUtf8().constData());
        return false;
    }
    QSqlQuery q(db);
    for (const char* sql : kSchemaStatements) {
        if (!q.exec(QLatin1StringView(sql))) {
            qWarning("SqliteDatabase::applySchema: failed: %s\nSQL: %s",
                     q.lastError().text().toUtf8().constData(), sql);
            db.rollback();
            return false;
        }
    }
    if (!db.commit()) {
        qWarning("SqliteDatabase::applySchema: COMMIT failed: %s",
                 db.lastError().text().toUtf8().constData());
        db.rollback();
        return false;
    }
    return true;
}

}  // namespace musicplayer
