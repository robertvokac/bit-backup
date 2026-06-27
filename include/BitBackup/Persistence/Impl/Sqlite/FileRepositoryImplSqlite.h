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
#ifndef FILEREPOSITORYIMPLSQLITE_H
#define FILEREPOSITORYIMPLSQLITE_H

#include "BitBackup/Persistence/Api/FileRepository.h"
#include "BitBackup/Persistence/Impl/Sqlite/SqliteConnectionFactory.h"
#include "BitBackup/Entity/FsFile.h"
#include <memory>
#include <string>
#include <vector>

namespace SQLite { class Database; }

namespace BitBackup::Impl::Sqlite {
    using std::vector;
    class FileRepositoryImplSqlite : public Persistence::Api::FileRepository {

    private:
        Persistence::Impl::Sqlite::SqliteConnectionFactory* sqliteConnectionFactory;
        // One connection reused for the whole repository lifetime. Opening a new
        // SQLite::Database per call (as the old code did) is expensive and forced
        // a fresh schema read on every single row operation.
        std::unique_ptr<SQLite::Database> db;
        SQLite::Database& database();

    public:
        FileRepositoryImplSqlite(Persistence::Impl::Sqlite::SqliteConnectionFactory* sqliteConnectionFactoryIn);
        ~FileRepositoryImplSqlite();

        void create(const vector<Entity::FsFile>& files) override;
        vector<Entity::FsFile> list() override;


        void remove(const Entity::FsFile& file) override;


        void updateFile(Entity::FsFile& file) override;

    public: void updateLastCheckDate(std::string& lastCheckDate, vector<Entity::FsFile>& files) override;

        void removeAll(const vector<Entity::FsFile>& files) override;
        void updateAll(const vector<Entity::FsFile>& files) override;
    };
}
#endif // FILEREPOSITORYIMPLSQLITE_H
