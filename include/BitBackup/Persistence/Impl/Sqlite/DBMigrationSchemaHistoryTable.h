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


/**
 *
*
 */
#ifndef DBMIGRATIONSCHEMAHISTORYTABLE_H
#define DBMIGRATIONSCHEMAHISTORYTABLE_H

namespace BitBackup::Persistence::Impl::Sqlite {
    struct DBMigrationSchemaHistoryTable {
        DBMigrationSchemaHistoryTable() = delete;

        DBMigrationSchemaHistoryTable(const DBMigrationSchemaHistoryTable &) = delete;

        DBMigrationSchemaHistoryTable &operator=(const DBMigrationSchemaHistoryTable &) = delete;

        static constexpr const char *TABLE_NAME = "DB_MIGRATION_SCHEMA_HISTORY";

        static constexpr const char *ID = "ID";
        static constexpr const char *MIGRATION_GROUP = "MIGRATION_GROUP";
        static constexpr const char *INSTALLED_RANK = "INSTALLED_RANK";
        static constexpr const char *VERSION = "VERSION";
        static constexpr const char *DESCRIPTION = "DESCRIPTION";

        static constexpr const char *TYPE = "TYPE";
        static constexpr const char *SCRIPT = "SCRIPT";
        static constexpr const char *HASH = "HASH";
        static constexpr const char *INSTALLED_BY = "INSTALLED_BY";
        static constexpr const char *INSTALLED_ON = "INSTALLED_ON";

        static constexpr const char *EXECUTION_TIME = "EXECUTION_TIME";
        static constexpr const char *SUCCESS = "SUCCESS";
    };
}
#endif // DBMIGRATIONSCHEMAHISTORYTABLE_H
