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

#ifndef SQLITEDATABASEMIGRATION_H
#define SQLITEDATABASEMIGRATION_H

#include <string>

#include "BitBackup/Persistence/Impl/Sqlite/MigrationResult.h"

#include "BitBackup/Core/Utils.h"

namespace BitBackup::Persistence::Impl::Sqlite {

/**
 *
* @author <a href="mailto:mail@robertvokac.com">Robert Vokac</a>
 */
class SqliteDatabaseMigration {
private:
    //Not meant to be instantiated
    SqliteDatabaseMigration();

public:
    SqliteDatabaseMigration(const SqliteDatabaseMigration&) = delete;
    SqliteDatabaseMigration& operator=(const SqliteDatabaseMigration&) = delete;

    static SqliteDatabaseMigration* getInstance();
    MigrationResult migrate(std::string directoryWhereSqliteFileIs);

};

}
#endif // SQLITEDATABASEMIGRATION_H
