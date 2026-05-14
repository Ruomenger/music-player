#pragma once

#include <string>

namespace musicplayer {

// Owns a named QSqlDatabase connection: opens the file, applies per-connection
// PRAGMAs (foreign_keys, journal_mode, ...) and runs the Phase 2 baseline
// schema migration. Repos reference the same connection by name.
//
// One instance per logical database; not copyable/movable. The destructor
// closes and removes the connection.
class SqliteDatabase {
public:
    explicit SqliteDatabase(std::string connectionName = "musicplayer");
    ~SqliteDatabase();

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;
    SqliteDatabase(SqliteDatabase&&) = delete;
    SqliteDatabase& operator=(SqliteDatabase&&) = delete;

    // dbPath: filesystem path or the literal ":memory:" for an in-process DB.
    // Returns false on driver/open/migration failure (details logged via qWarning).
    bool open(const std::string& dbPath);
    void close();

    [[nodiscard]] bool isOpen() const { return opened_; }
    [[nodiscard]] const std::string& connectionName() const { return connectionName_; }

private:
    bool applyPragmas();
    bool applySchema();

    std::string connectionName_;
    bool opened_ = false;
};

}  // namespace musicplayer
