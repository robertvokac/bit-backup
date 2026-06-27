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

// Integration tests for the SQLite FileRepository against a real (temporary)
// .bitbackup.sqlite3 database created via the production schema migration.
// These guard the batched single-connection implementation: create / list /
// updateAll / removeAll / updateLastCheckDate must keep the same row semantics
// as the original per-row implementation.

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "BitBackup/Entity/FsFile.h"
#include "BitBackup/Persistence/Impl/Sqlite/FileRepositoryImplSqlite.h"
#include "BitBackup/Persistence/Impl/Sqlite/SqliteConnectionFactory.h"
#include "BitBackup/Persistence/Impl/Sqlite/SqliteDatabaseMigration.h"

namespace fs = std::filesystem;
using BitBackup::Entity::FsFile;

namespace {

FsFile makeFile(const std::string& id,
                const std::string& relPath,
                const std::string& hash,
                unsigned long size,
                const std::string& result = "OK") {
    return FsFile{
        id,
        fs::path(relPath).filename().string(),
        relPath,
        "2024-01-01 00:00:00.000",
        "2024-01-01 00:00:00.000",
        hash,
        "SHA-512",
        size,
        result
    };
}

// Index the listed rows by ABSOLUTE_PATH for easy assertions.
std::map<std::string, FsFile> byPath(const std::vector<FsFile>& files) {
    std::map<std::string, FsFile> m;
    for (const auto& f : files) m[f.absolutePath] = f;
    return m;
}

class FileRepositoryTest : public ::testing::Test {
protected:
    fs::path dir;
    std::string dirStr;
    std::unique_ptr<BitBackup::Persistence::Impl::Sqlite::SqliteConnectionFactory> factory;
    std::unique_ptr<BitBackup::Impl::Sqlite::FileRepositoryImplSqlite> repo;

    void SetUp() override {
        dir = fs::temp_directory_path() /
              (std::string("bitbackup_repo_test_") + ::testing::UnitTest::GetInstance()
                   ->current_test_info()->name());
        fs::remove_all(dir);
        fs::create_directories(dir);
        dirStr = dir.string();

        auto result = BitBackup::Persistence::Impl::Sqlite::SqliteDatabaseMigration
                          ::getInstance()->migrate(dirStr);
        ASSERT_EQ(result, BitBackup::Persistence::Impl::Sqlite::MigrationResult::SUCCESS);

        factory = std::make_unique<
            BitBackup::Persistence::Impl::Sqlite::SqliteConnectionFactory>(dirStr);
        repo = std::make_unique<
            BitBackup::Impl::Sqlite::FileRepositoryImplSqlite>(factory.get());
    }

    void TearDown() override {
        repo.reset();
        factory.reset();
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
};

TEST_F(FileRepositoryTest, CreateAndListRoundTrip) {
    std::vector<FsFile> input = {
        makeFile("id-1", "a.txt", "hashA", 11),
        makeFile("id-2", "sub/b.txt", "hashB", 22),
    };
    repo->create(input);

    auto all = repo->list();
    ASSERT_EQ(all.size(), 2u);

    auto m = byPath(all);
    ASSERT_TRUE(m.count("a.txt"));
    ASSERT_TRUE(m.count("sub/b.txt"));
    EXPECT_EQ(m["a.txt"].id, "id-1");
    EXPECT_EQ(m["a.txt"].hashSumValue, "hashA");
    EXPECT_EQ(m["a.txt"].size, 11u);
    EXPECT_EQ(m["sub/b.txt"].hashSumValue, "hashB");
    EXPECT_EQ(m["sub/b.txt"].size, 22u);
}

TEST_F(FileRepositoryTest, LockedFlagRoundTrips) {
    FsFile locked = makeFile("id-1", "a.txt", "h1", 1);
    locked.locked = 1;
    FsFile normal = makeFile("id-2", "b.txt", "h2", 2);   // locked defaults to 0
    repo->create({locked, normal});

    auto m = byPath(repo->list());
    EXPECT_EQ(m["a.txt"].locked, 1);
    EXPECT_EQ(m["b.txt"].locked, 0);

    // updateAll persists the locked flag too.
    FsFile unlock = m["a.txt"];
    unlock.locked = 0;
    repo->updateAll({unlock});
    EXPECT_EQ(byPath(repo->list())["a.txt"].locked, 0);
}

TEST_F(FileRepositoryTest, EmptyBatchesAreNoOps) {
    // Must not throw or create rows.
    repo->create({});
    repo->removeAll({});
    repo->updateAll({});
    std::string date = "2024-02-02 00:00:00.000";
    std::vector<FsFile> none;
    repo->updateLastCheckDate(date, none);
    EXPECT_TRUE(repo->list().empty());
}

TEST_F(FileRepositoryTest, UpdateAllUpdatesMutableFields) {
    repo->create({
        makeFile("id-1", "a.txt", "old", 1),
        makeFile("id-2", "b.txt", "keep", 2),
    });

    FsFile changed = makeFile("id-1", "a.txt", "newhash", 999, "KO");
    changed.lastModificationDate = "2024-09-09 09:09:09.999";
    changed.lastCheckDate = "2024-09-09 10:00:00.000";
    repo->updateAll({changed});

    auto m = byPath(repo->list());
    EXPECT_EQ(m["a.txt"].hashSumValue, "newhash");
    EXPECT_EQ(m["a.txt"].size, 999u);
    EXPECT_EQ(m["a.txt"].lastCheckResult, "KO");
    EXPECT_EQ(m["a.txt"].lastModificationDate, "2024-09-09 09:09:09.999");
    // unrelated row untouched
    EXPECT_EQ(m["b.txt"].hashSumValue, "keep");
}

TEST_F(FileRepositoryTest, UpdateAllMatchesSingleUpdateFile) {
    // updateAll([f]) must produce the exact same row state as updateFile(f).
    repo->create({makeFile("id-1", "a.txt", "old", 1)});

    FsFile viaBatch = makeFile("id-1", "a.txt", "batched", 42, "OK");
    viaBatch.lastCheckDate = "2024-05-05 05:05:05.000";
    repo->updateAll({viaBatch});
    auto afterBatch = byPath(repo->list())["a.txt"];

    FsFile viaSingle = afterBatch;          // re-apply identical values via single path
    repo->updateFile(viaSingle);
    auto afterSingle = byPath(repo->list())["a.txt"];

    EXPECT_EQ(afterBatch, afterSingle);
}

TEST_F(FileRepositoryTest, RemoveAllRemovesOnlySelectedRows) {
    repo->create({
        makeFile("id-1", "a.txt", "h1", 1),
        makeFile("id-2", "b.txt", "h2", 2),
        makeFile("id-3", "c.txt", "h3", 3),
    });

    repo->removeAll({
        makeFile("id-1", "a.txt", "h1", 1),
        makeFile("id-3", "c.txt", "h3", 3),
    });

    auto all = repo->list();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0].absolutePath, "b.txt");
}

TEST_F(FileRepositoryTest, UpdateLastCheckDateSetsDateAndOk) {
    repo->create({
        makeFile("id-1", "a.txt", "h1", 1, "KO"),
        makeFile("id-2", "b.txt", "h2", 2, "KO"),
    });

    std::string date = "2025-12-31 23:59:59.999";
    std::vector<FsFile> toTouch = {makeFile("id-1", "a.txt", "h1", 1, "KO")};
    repo->updateLastCheckDate(date, toTouch);

    auto m = byPath(repo->list());
    EXPECT_EQ(m["a.txt"].lastCheckDate, date);
    EXPECT_EQ(m["a.txt"].lastCheckResult, "OK");   // forced to OK by the query
    // the other row keeps its previous values
    EXPECT_EQ(m["b.txt"].lastCheckResult, "KO");
}

} // namespace
