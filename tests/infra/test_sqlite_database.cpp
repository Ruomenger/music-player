#include <gtest/gtest.h>
#include "sqlite_database.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

#include <atomic>
#include <set>
#include <string>

using musicplayer::SqliteDatabase;

namespace {

std::string uniqueConn(const char* base) {
    static std::atomic<int> counter{0};
    return std::string(base) + "_" + std::to_string(counter.fetch_add(1));
}

}  // namespace

TEST(SqliteDatabaseTest, OpenInMemorySucceedsAndIsOpen) {
    SqliteDatabase db(uniqueConn("open"));
    EXPECT_FALSE(db.isOpen());
    ASSERT_TRUE(db.open(":memory:"));
    EXPECT_TRUE(db.isOpen());
}

TEST(SqliteDatabaseTest, MigrationCreatesAllBaselineTables) {
    const std::string name = uniqueConn("schema");
    SqliteDatabase db(name);
    ASSERT_TRUE(db.open(":memory:"));

    QSqlDatabase conn = QSqlDatabase::database(QString::fromStdString(name));
    QSqlQuery q(conn);
    ASSERT_TRUE(
        q.exec("SELECT name FROM sqlite_master WHERE type='table' "
               "AND name NOT LIKE 'sqlite_%'"));
    std::set<std::string> tables;
    while (q.next())
        tables.insert(q.value(0).toString().toStdString());

    EXPECT_TRUE(tables.contains("songs"));
    EXPECT_TRUE(tables.contains("playlists"));
    EXPECT_TRUE(tables.contains("playlist_songs"));
    EXPECT_TRUE(tables.contains("play_history"));
    EXPECT_TRUE(tables.contains("settings"));
    EXPECT_TRUE(tables.contains("player_state"));
}

TEST(SqliteDatabaseTest, ForeignKeysPragmaIsActive) {
    const std::string name = uniqueConn("fk");
    SqliteDatabase db(name);
    ASSERT_TRUE(db.open(":memory:"));
    QSqlDatabase conn = QSqlDatabase::database(QString::fromStdString(name));
    QSqlQuery q(conn);
    ASSERT_TRUE(q.exec("PRAGMA foreign_keys"));
    ASSERT_TRUE(q.next());
    EXPECT_EQ(q.value(0).toInt(), 1);
}

TEST(SqliteDatabaseTest, ReopenAfterCloseSucceeds) {
    SqliteDatabase db(uniqueConn("reopen"));
    ASSERT_TRUE(db.open(":memory:"));
    db.close();
    EXPECT_FALSE(db.isOpen());
    EXPECT_TRUE(db.open(":memory:"));
    EXPECT_TRUE(db.isOpen());
}

TEST(SqliteDatabaseTest, DoubleOpenReturnsFalse) {
    SqliteDatabase db(uniqueConn("double"));
    ASSERT_TRUE(db.open(":memory:"));
    EXPECT_FALSE(db.open(":memory:"));
}
