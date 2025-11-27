/*
 * MIT License
 * Copyright (c) 2023-2025 Robert Vokac
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "BitBackup/Persistence/Impl/Sqlite/FileRepositoryImplSqlite.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace BitBackup::Impl::Sqlite {

FileRepositoryImplSqlite::FileRepositoryImplSqlite(Persistence::Impl::Sqlite::SqliteConnectionFactory* sqliteConnectionFactoryIn)
: sqliteConnectionFactory(sqliteConnectionFactoryIn) {}

FileRepositoryImplSqlite::~FileRepositoryImplSqlite() = default;

// INSERT batch (stejně jako Java)
void FileRepositoryImplSqlite::create(const vector<Entity::FsFile>& files) {
    if (files.empty()) return;

    SQLite::Database db(
        sqliteConnectionFactory->createConnection()->getName(),
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );
    SQLite::Transaction txn(db);

    const char* sql =
        "INSERT INTO FILE (ID, NAME, ABSOLUTE_PATH, LAST_MODIFICATION_DATE, LAST_CHECK_DATE, "
        "HASH_SUM_VALUE, HASH_SUM_ALGORITHM, SIZE, LAST_CHECK_RESULT) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    SQLite::Statement stmt(db, sql);

    for (const auto& f : files) {
        int i = 0;
        stmt.bind(++i, f.id);
        stmt.bind(++i, f.name);
        stmt.bind(++i, f.absolutePath);
        stmt.bind(++i, f.lastModificationDate);
        stmt.bind(++i, f.lastCheckDate);
        stmt.bind(++i, f.hashSumValue);
        stmt.bind(++i, f.hashSumAlgorithm);
        stmt.bind(++i, static_cast<int64_t>(f.size));
        stmt.bind(++i, f.lastCheckResult);
        stmt.exec();
        stmt.reset();
        stmt.clearBindings();
    }

    txn.commit();
}

// SELECT * (column order matches 1:1 with Java)
vector<Entity::FsFile> FileRepositoryImplSqlite::list() {
    vector<Entity::FsFile> result;

    SQLite::Database db(
        sqliteConnectionFactory->createConnection()->getName(),
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );

    const char* sql =
        "SELECT ID, NAME, ABSOLUTE_PATH, LAST_MODIFICATION_DATE, LAST_CHECK_DATE, "
        "HASH_SUM_VALUE, HASH_SUM_ALGORITHM, SIZE, LAST_CHECK_RESULT "
        "FROM FILE";

    SQLite::Statement stmt(db, sql);
    while (stmt.executeStep()) {
        Entity::FsFile f{
            stmt.getColumn(0).getString(),
            stmt.getColumn(1).getString(),
            stmt.getColumn(2).getString(),
            stmt.getColumn(3).getString(),
            stmt.getColumn(4).getString(),
            stmt.getColumn(5).getString(),
            stmt.getColumn(6).getString(),
            static_cast<unsigned long>(stmt.getColumn(7).getInt64()),
            stmt.getColumn(8).getString()
        };
        result.push_back(std::move(f));
    }
    return result;
}

// DELETE BY ID (Java used ID deletion; CheckCommand in C++ calls remove(FsFile) - let's stick with ID)
void FileRepositoryImplSqlite::remove(const Entity::FsFile& file) {
    SQLite::Database db(
        sqliteConnectionFactory->createConnection()->getName(),
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );
    const char* sql = "DELETE FROM FILE WHERE ID=?";
    SQLite::Statement stmt(db, sql);
    stmt.bind(1, file.id);
    stmt.exec();
}

// UPDATE all mutable fields according to Java version (WHERE ID=?)
void FileRepositoryImplSqlite::updateFile(Entity::FsFile& file) {
    SQLite::Database db(
        sqliteConnectionFactory->createConnection()->getName(),
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );

    const char* sql =
        "UPDATE FILE SET "
        "LAST_MODIFICATION_DATE=?, "
        "LAST_CHECK_DATE=?, "
        "HASH_SUM_VALUE=?, "
        "HASH_SUM_ALGORITHM=?, "
        "SIZE=?, "
        "LAST_CHECK_RESULT=? "
        "WHERE ID=?";

    SQLite::Statement stmt(db, sql);
    int i = 0;
    stmt.bind(++i, file.lastModificationDate);
    stmt.bind(++i, file.lastCheckDate);
    stmt.bind(++i, file.hashSumValue);
    stmt.bind(++i, file.hashSumAlgorithm);
    stmt.bind(++i, static_cast<int64_t>(file.size));
    stmt.bind(++i, file.lastCheckResult);
    stmt.bind(++i, file.id);
    stmt.exec();
}

// SELECT * (column order matches 1:1 with Java)
void FileRepositoryImplSqlite::updateLastCheckDate(std::string& lastCheckDate, vector<Entity::FsFile>& files) {
    if (files.empty()) return;

    SQLite::Database db(
        sqliteConnectionFactory->createConnection()->getName(),
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );
    SQLite::Transaction txn(db);

    // Simple iterative approach (SQLiteCpp doesn't support dynamic IN without string building)
    const char* sql = "UPDATE FILE SET LAST_CHECK_DATE=?, LAST_CHECK_RESULT='OK' WHERE ID=?";
    SQLite::Statement stmt(db, sql);

    for (auto& f : files) {
        stmt.bind(1, lastCheckDate);
        stmt.bind(2, f.id);
        stmt.exec();
        stmt.reset();
        stmt.clearBindings();
    }

    txn.commit();
}

} // namespace BitBackup::Impl::Sqlite
