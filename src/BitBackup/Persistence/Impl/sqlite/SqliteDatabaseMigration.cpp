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

#include <iostream>
#include <string>

#include "BitBackup/Persistence/Impl/Sqlite/MigrationResult.h"
#include "BitBackup/Persistence/Impl/Sqlite/SqliteDatabaseMigration.h"
#include <SQLiteCpp/SQLiteCpp.h>

#include "BitBackup/Core/ProgressTracker.h"
#include "BitBackup/Core/Utils.h"
#include "BitBackup/Persistence/Impl/Sqlite/DBMigrationSchemaHistory.h"
#include "BitBackup/Persistence/Impl/Sqlite/Migrations.h"
#include "BitBackup/Persistence/Impl/Sqlite/DBMigrationSchemaHistoryTable.h"
#include <openssl/sha.h>
#include <iomanip>

namespace BitBackup::Persistence::Impl::Sqlite {
    SqliteDatabaseMigration::SqliteDatabaseMigration() {
        //Not meant to be instantiated
    };

    static SqliteDatabaseMigration *INSTANCE;

    SqliteDatabaseMigration *SqliteDatabaseMigration::getInstance() {
        if (INSTANCE == nullptr) {
            INSTANCE = new SqliteDatabaseMigration();
        }
        return INSTANCE;
    }

    class DBMigration {
    private:
        std::string jdbcUrl;
        std::string directoryWhereSqliteFileIs;
        std::string installedBy;
        std::string name;
        std::string sqlDialect;
        std::string sqlDialectImplClass;

    public:
        DBMigration(
            std::string jdbcUrlIn,
            std::string directoryWhereSqliteFileIsIn,
            std::string installedByIn,
            std::string nameIn,
            std::string sqlDialectIn,
            std::string sqlDialectImplClassIn
        ) {
            this->jdbcUrl = jdbcUrlIn;
            this->directoryWhereSqliteFileIs = directoryWhereSqliteFileIsIn;
            this->installedBy = installedByIn;
            this->name = nameIn;
            this->sqlDialect = sqlDialectIn;
            this->sqlDialectImplClass = sqlDialectImplClassIn;
        }

        bool executeSQL(SQLite::Database& db, std::string& sql, int number) {

            try {
                SQLite::Statement query(db, sql);
                query.exec();
                return true;
            } catch (SQLite::Exception &e) {
                std::cerr << "Exception happened during SQLite migration # " << number << ": " << e.what() << " " << std::endl;
                return false;
            }
        }

        bool createTable(SQLite::Database& db) {
            std::string SQL_CREATE_TABLE_DB_MIGRATION_SCHEMA_HISTORY =
                    R"(
CREATE TABLE "DB_MIGRATION_SCHEMA_HISTORY" (
                "ID" TEXT NOT NULL,
                "MIGRATION_GROUP" TEXT NOT NULL,
                "INSTALLED_RANK" INTEGER NOT NULL,
                "VERSION" TEXT NOT NULL,
                "DESCRIPTION" TEXT NOT NULL,
                "TYPE" TEXT NOT NULL,
                "SCRIPT" TEXT NOT NULL,
                "HASH" TEXT NOT NULL UNIQUE,
                "INSTALLED_BY" TEXT NOT NULL,
                "INSTALLED_ON" TEXT NOT NULL,
                "EXECUTION_TIME" INTEGER NOT NULL,
                "SUCCESS" INTEGER NOT NULL,
                PRIMARY KEY("ID"),
                CONSTRAINT "DB_MIGRATION_SCHEMA_HISTORY_UNIQUE_CONSTRAINT_MIGRATION_GROUP_AND_INSTALLED_RANK" UNIQUE("MIGRATION_GROUP","INSTALLED_RANK"),
                CONSTRAINT "DB_MIGRATION_SCHEMA_HISTORY_UNIQUE_CONSTRAINT_MIGRATION_GROUP_AND_VERSION" UNIQUE("MIGRATION_GROUP","VERSION")
            );
)";
            return executeSQL(db, SQL_CREATE_TABLE_DB_MIGRATION_SCHEMA_HISTORY, 0);
        }

        bool validateTableExists(SQLite::Database& db) {
            bool doesTableExist = false;

            try {
                SQLite::Statement query(
                db, std::string("SELECT * FROM ") + DBMigrationSchemaHistoryTable::TABLE_NAME);

                query.executeStep();
                doesTableExist = true;
            } catch (SQLite::Exception &e) {
                doesTableExist = false;
            }
            if (!doesTableExist) {
                std::cerr << "Table " << DBMigrationSchemaHistoryTable::TABLE_NAME << " does not exist." << std::endl;
                return false;
            }
            return true;
        }
        bool validateThatThereAreNoMigrationFailures(SQLite::Database& db) {
            bool noFailures = false;
            SQLite::Statement query(
                db, std::string(
                    "SELECT COUNT(*) AS C FROM ") +
                    DBMigrationSchemaHistoryTable::TABLE_NAME +
                    " WHERE " +
                    DBMigrationSchemaHistoryTable::SUCCESS +
                    " = 0"
                    );
            try {
                query.executeStep();
                int count = query.getColumn(0);

                noFailures = count == 0;
            } catch (SQLite::Exception &e) {
                std::cerr << "Exception happened during SQLite migration validateThatThereAreNoMigrationFailures(): " << e.what() << std::endl;
                return false;
            }
            if (!noFailures) {
                std::cerr << "Table " << DBMigrationSchemaHistoryTable::TABLE_NAME << " has some migration failures." << std::endl;
                return false;
            }
            return true;
        }

        bool validateMigrationHashes(SQLite::Database& db) {
            SQLite::Statement q(db,
                "SELECT INSTALLED_RANK, HASH FROM DB_MIGRATION_SCHEMA_HISTORY ORDER BY INSTALLED_RANK");

            int i = 0;
            while (q.executeStep()) {
                int rank = q.getColumn(0).getInt();
                std::string storedHash = q.getColumn(1).getString();
                std::string expectedHash = sha1(migrations[rank - 1]);

                if (storedHash != expectedHash) {
                    std::cerr << "Migration " << rank << " has modified SQL!" << std::endl;
                    return false;
                }
            }
            return true;
        }


        bool validate(SQLite::Database& db) {
            if (!validateTableExists(db)) {
                bool create = createTable(db);
                if (!create) return false;
            }
            if (!validateTableExists(db)) return false;
            if (!validateThatThereAreNoMigrationFailures(db)) return false;
            if (!validateMigrationHashes(db)) return false;

            return true;
        }

        int getNewestMigrationNumber(SQLite::Database& db) {
            SQLite::Statement query(
                db, std::string(
                    std::string("SELECT MAX(") +
                    DBMigrationSchemaHistoryTable::INSTALLED_RANK +
                    std::string(") AS M FROM ") +
                    DBMigrationSchemaHistoryTable::TABLE_NAME
                    ));
            try {
                query.executeStep();
                int max = query.getColumn(0);

                return max;
            } catch (SQLite::Exception &e) {
                std::cerr << "Exception happened during SQLite migration getNewestMigrationNumber(): " << e.what() << std::endl;
                return -1;
            }
        }

        bool addMigration(SQLite::Database& db, Entity::DBMigrationSchemaHistory& migration) {
            SQLite::Statement query(
                db,
            "INSERT INTO " +
            std::string(DBMigrationSchemaHistoryTable::TABLE_NAME) +
            " VALUES (?,?,?,?,?,   ?,?,?,?,?,   ?,?)"
            );
            int i = 0;
            query.bind(++i, migration.id);
            query.bind(++i, migration.migrationGroup);
            query.bind(++i, migration.installedRank);
            query.bind(++i, migration.version);
            query.bind(++i, migration.description);
            ////
            query.bind(++i, migration.type);
            query.bind(++i, migration.script);
            query.bind(++i, migration.hash);
            query.bind(++i, migration.installedBy);
            query.bind(++i, migration.installedOn);
            ////
            query.bind(++i, migration.executionTime);
            query.bind(++i, migration.success);
            try {
                query.exec();
                return true;
            } catch (SQLite::Exception &e) {
                std::cerr << "Exception happened during adding # " << migration.installedRank << "migration to database: " << e.what() << std::endl;
                return false;
            }
        }

        std::string sha1(const std::string& input) {
            unsigned char hash[SHA_DIGEST_LENGTH]; // SHA1 digest size
            SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

            std::ostringstream oss;
            for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
            }

            return oss.str();
        }


        std::string getCurrentDateTime() {
            using namespace std::chrono;

            // Get current time point
            auto now = system_clock::now();
            auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
            std::time_t now_c = system_clock::to_time_t(now);
            std::tm now_tm = *std::localtime(&now_c);

            std::ostringstream oss;
            oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << ':'
                << std::setw(3) << std::setfill('0') << ms.count();

            return oss.str();
        }


        MigrationResult migrate() {
            std::string dbFile = directoryWhereSqliteFileIs + "/.bitbackup.sqlite3";
            try {
                SQLite::Database db(dbFile, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

                if (!validate(db)) return FAILURE;
                int maxMigrationNumber = getNewestMigrationNumber(db);
                if (maxMigrationNumber == -1) return FAILURE;
                for (int migrationNumber = 1; migrationNumber <= MIGRATION_COUNT; migrationNumber++) {
                    if (migrationNumber <= maxMigrationNumber) {
                        std::cout << "Skipping already finished migration " << migrationNumber << std::endl;
                        continue;
                    }
                    std::string sql = migrations[migrationNumber - 1];
                    Core::ProgressTracker pt = 1;
                    pt.start();
                    bool migrated = executeSQL(db, sql, migrationNumber);
                    pt.nextDone();
                    int elapsedMilliseconds = pt.getElapsedMillisecondsSinceStart();
                    if (migrated) {

                        Entity::DBMigrationSchemaHistory db_migration_schema_history = {
                            Core::Utils::generateUUIDv4(),
                            "bitbackup",
                            migrationNumber,
                            std::to_string(migrationNumber),
                            sql,
                            "SQL",
                            std::to_string(migrationNumber),
                            sha1(sql),
                            installedBy,
                            getCurrentDateTime(),
                            elapsedMilliseconds,
                            true
                        };

                        bool added = addMigration(db, db_migration_schema_history);
                        if (!added) {
                            std::cerr << "Migration " << migrationNumber << " failed, it could not be added to the database. " << std::endl;
                            return FAILURE;
                        }
                    } else {
                        std::cerr << "Migration " << migrationNumber << " failed." << std::endl;
                        return MigrationResult::FAILURE;
                    }
                }
            } catch (std::exception &e) {
                std::cout << "Exception happened during SQLite migration: " << e.what() << " " << std::endl;
                return MigrationResult::FAILURE;
            }

            return MigrationResult::SUCCESS;
        }
    };

    MigrationResult SqliteDatabaseMigration::migrate(std::string directoryWhereSqliteFileIs) {
        using std::string;
        string jdbcUrl = Core::Utils::createJdbcUrl(directoryWhereSqliteFileIs);
        std::cerr << "jdbcUrl=" << jdbcUrl << std::endl;

        DBMigration dbMigration = DBMigration(
            jdbcUrl,
            directoryWhereSqliteFileIs,
            "bitbackup-persistence-impl-sqlite",
            "bitbackup",
            "sqlite",
            "BitBackup::Persistence::Impl::Sqlite.SqliteDatabaseMigration");
        return dbMigration.migrate();
    }
}
