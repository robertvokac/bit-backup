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

#include "BitBackup/Persistence/Impl/Sqlite/SqliteConnectionFactory.h"
#include "BitBackup/Persistence/Api/SystemItemRepository.h"
#include "BitBackup/Persistence/Impl/Sqlite/SystemItemRepositoryImplSqlite.h"

#include <iostream>

#include "BitBackup/Persistence/Impl/Sqlite/SystemItemTable.h"
#include <SQLiteCpp/SQLiteCpp.h>

#include "BitBackup/Core/BitBackupException.h"


namespace BitBackup::Impl::Sqlite
{
    using std::vector;
    using std::string;


    SystemItemRepositoryImplSqlite::SystemItemRepositoryImplSqlite(
        Persistence::Impl::Sqlite::SqliteConnectionFactory* sqliteConnectionFactoryIn) :
        sqliteConnectionFactory(sqliteConnectionFactoryIn)
    {
    }

    SystemItemRepositoryImplSqlite::~SystemItemRepositoryImplSqlite()
    {
    }


    string SystemItemRepositoryImplSqlite::create(const Entity::SystemItem& systemItem)
    {
        std::string sql = "INSERT INTO " +
            std::string(Persistence::Impl::Sqlite::SystemItemTable::TABLE_NAME) +
            "(" +
            std::string(Persistence::Impl::Sqlite::SystemItemTable::KEY) +
            "," +
            std::string(Persistence::Impl::Sqlite::SystemItemTable::VALUE);

        sql += ")";
        sql += " VALUES (?,?)";

        SQLite::Database db(sqliteConnectionFactory->createConnection()->getName(),
                            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        SQLite::Statement query(db, sql);
        std::cerr << sql << std::endl;
        try
        {
            int i = 0;
            query.bind(++i, systemItem.key);
            query.bind(++i, systemItem.value);

            //
            query.exec();

            return systemItem.key;
        }
        catch (SQLite::Exception& e)
        {
            std::cerr << "Exception happened during of execution of SQLite SQL statement  " << sql << ": " << e.what()
                << " " << std::endl;
            std::cerr << "Error.";
            return "";
        }
    }


    vector<Entity::SystemItem> SystemItemRepositoryImplSqlite::list()
    {
        vector<Entity::SystemItem> out;

        SQLite::Database db(
            sqliteConnectionFactory->createConnection()->getName(),
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );

        const char* sql =
            "SELECT KEY, VALUE FROM SYSTEM_ITEM";

        SQLite::Statement stmt(db, sql);
        while (stmt.executeStep())
        {
            out.push_back(Entity::SystemItem{
                stmt.getColumn(0).getString(),
                stmt.getColumn(1).getString()
            });
        }
        return out;
    }


    Entity::SystemItem extractSystemItemFromResultSet(const SQLite::Statement& query)
    {
        return Entity::SystemItem(
            query.getColumn(0),
            query.getColumn(1)
        );
    }

    Entity::SystemItem SystemItemRepositoryImplSqlite::read(const string& key)
    {
        std::string sql =
            "SELECT * FROM " +
            std::string(Persistence::Impl::Sqlite::SystemItemTable::TABLE_NAME) +
            " WHERE " +
            std::string(Persistence::Impl::Sqlite::SystemItemTable::KEY) +
            "=?";
        SQLite::Database db(sqliteConnectionFactory->createConnection()->getName(),
                            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        SQLite::Statement query(db, sql);

        int i = 0;

        try
        {
            query.bind(++i, key);

            while (query.executeStep())
            {
                return extractSystemItemFromResultSet(query);
            }
            return Entity::SystemItem(key, "");
        }
        catch (SQLite::Exception& e)
        {
            std::cout << e.what() << std::endl;
            throw std::runtime_error(e.what());
        }
    }


    void SystemItemRepositoryImplSqlite::remove(const string& key)
    {
        if (key.empty()) throw BitBackup::Core::BitBackupException("key is null/empty");
        SQLite::Database db(
            sqliteConnectionFactory->createConnection()->getName(),
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );
        SQLite::Statement stmt(db, "DELETE FROM SYSTEM_ITEM WHERE KEY=?");
        stmt.bind(1, key);
        stmt.exec();
    }

    void SystemItemRepositoryImplSqlite::update(Entity::SystemItem& systemItem)
    {
        SQLite::Database db(
            sqliteConnectionFactory->createConnection()->getName(),
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );
        SQLite::Statement stmt(db, "UPDATE SYSTEM_ITEM SET VALUE=? WHERE KEY=?");
        stmt.bind(1, systemItem.value);
        stmt.bind(2, systemItem.key);
        stmt.exec();
    }
}
