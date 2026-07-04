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

// End-to-end tests for directory locking via a .bitbackuplock marker. A locked
// subtree is frozen: any modification, new file, or deletion is reported as a
// violation and the stored mtime/hash are never overwritten.

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

#include "BitBackup/Commands/CheckCommand.h"
#include "BitBackup/Core/BitBackupArgs.h"

namespace fs = std::filesystem;
using BitBackup::Commands::CheckCommand;
using BitBackup::Core::BitBackupArgs;

namespace {

void writeFile(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream(p, std::ios::binary) << content;
}

struct Row {
    bool exists = false;
    std::string hash;
    std::string mtime;
    std::string result;
    int locked = 0;
};

} // namespace

class CheckCommandLockTest : public ::testing::Test {
protected:
    fs::path dir;
    fs::path oldCwd;

    void SetUp() override {
        oldCwd = fs::current_path();
        dir = fs::temp_directory_path() /
              (std::string("bitbackup_lock_") +
               ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(dir);
        fs::create_directories(dir);
        fs::current_path(dir);
    }

    void TearDown() override {
        fs::current_path(oldCwd);
        std::error_code ec;
        fs::remove_all(dir, ec);
    }

    std::string runCheck(const std::vector<std::string>& args = {"check"}) {
        std::stringstream sink;
        std::streambuf* oldOut = std::cout.rdbuf(sink.rdbuf());
        std::streambuf* oldErr = std::cerr.rdbuf(sink.rdbuf());
        std::string result;
        try {
            CheckCommand cmd;
            result = cmd.run(BitBackupArgs(args));
        } catch (...) {
            std::cout.rdbuf(oldOut);
            std::cerr.rdbuf(oldErr);
            throw;
        }
        std::cout.rdbuf(oldOut);
        std::cerr.rdbuf(oldErr);
        return result;
    }

    Row queryRow(const std::string& relPath) {
        SQLite::Database db((dir / ".bitbackup.sqlite3").string(), SQLite::OPEN_READONLY);
        SQLite::Statement q(db,
            "SELECT HASH_SUM_VALUE, LAST_MODIFICATION_DATE, LAST_CHECK_RESULT, LOCKED "
            "FROM FILE WHERE ABSOLUTE_PATH = ?");
        q.bind(1, relPath);
        Row r;
        if (q.executeStep()) {
            r.exists = true;
            r.hash = q.getColumn(0).getString();
            r.mtime = q.getColumn(1).getString();
            r.result = q.getColumn(2).getString();
            r.locked = q.getColumn(3).getInt();
        }
        return r;
    }
};

TEST_F(CheckCommandLockTest, MarkingLockedSetsLockedFlag) {
    writeFile(dir / "d/a.txt", "a");
    runCheck();                                   // index (no marker)
    EXPECT_EQ(queryRow("d/a.txt").locked, 0);

    writeFile(dir / "d/.bitbackuplock", "");
    EXPECT_EQ(runCheck(), "");                    // nothing changed -> no violation
    EXPECT_EQ(queryRow("d/a.txt").locked, 1);     // now frozen
}

TEST_F(CheckCommandLockTest, ModifyingLockedFileIsReportedAndDbFrozen) {
    writeFile(dir / "d/a.txt", "original");
    runCheck();
    writeFile(dir / "d/.bitbackuplock", "");
    runCheck();                                   // mark locked
    const Row before = queryRow("d/a.txt");
    ASSERT_EQ(before.locked, 1);

    // Real edit: change content AND advance the modification time.
    const auto t = fs::last_write_time(dir / "d/a.txt");
    writeFile(dir / "d/a.txt", "TAMPERED");
    fs::last_write_time(dir / "d/a.txt", t + std::chrono::hours(1));

    const std::string res = runCheck();
    EXPECT_NE(res.find("locked file modified"), std::string::npos);
    EXPECT_NE(res.find("d/a.txt"), std::string::npos);

    const Row after = queryRow("d/a.txt");
    EXPECT_EQ(after.hash, before.hash);           // hash NOT overwritten
    EXPECT_EQ(after.mtime, before.mtime);         // mtime NOT overwritten
    EXPECT_EQ(after.result, "KO");
    EXPECT_EQ(after.locked, 1);
}

TEST_F(CheckCommandLockTest, SilentBitRotInLockedDirIsReportedAndFrozen) {
    writeFile(dir / "d/a.txt", "original");
    runCheck();                                   // index without marker
    const Row before = queryRow("d/a.txt");

    writeFile(dir / "d/.bitbackuplock", "");
    // Silent rot: content changes but the modification time is restored.
    const auto t = fs::last_write_time(dir / "d/a.txt");
    writeFile(dir / "d/a.txt", "CORRUPTED");
    fs::last_write_time(dir / "d/a.txt", t);

    const std::string res = runCheck();
    EXPECT_NE(res.find("locked file modified"), std::string::npos);

    const Row after = queryRow("d/a.txt");
    EXPECT_EQ(after.hash, before.hash);           // frozen
    EXPECT_EQ(after.result, "KO");
    EXPECT_EQ(after.locked, 1);
}

TEST_F(CheckCommandLockTest, NewFileInLockedDirIsRejected) {
    writeFile(dir / "d/keep.txt", "keep");
    runCheck();                                   // index keep.txt
    writeFile(dir / "d/.bitbackuplock", "");
    writeFile(dir / "d/intruder.txt", "new");

    const std::string res = runCheck();
    EXPECT_NE(res.find("new file in locked directory"), std::string::npos);
    EXPECT_FALSE(queryRow("d/intruder.txt").exists);   // not tracked
    EXPECT_TRUE(queryRow("d/keep.txt").exists);
}

TEST_F(CheckCommandLockTest, DeletedLockedFileIsKept) {
    writeFile(dir / "d/a.txt", "a");
    runCheck();
    writeFile(dir / "d/.bitbackuplock", "");
    runCheck();                                   // mark locked
    const Row before = queryRow("d/a.txt");
    ASSERT_EQ(before.locked, 1);

    fs::remove(dir / "d/a.txt");
    const std::string res = runCheck();
    EXPECT_NE(res.find("locked file deleted"), std::string::npos);

    const Row after = queryRow("d/a.txt");
    EXPECT_TRUE(after.exists);                     // row preserved
    EXPECT_EQ(after.result, "KO");                 // flagged
    EXPECT_EQ(after.hash, before.hash);            // mtime/hash stay frozen
    EXPECT_EQ(after.mtime, before.mtime);
}

TEST_F(CheckCommandLockTest, DeletedLockedFileWithMarkerGoneStillReported) {
    // Even if the whole locked dir (and its marker) vanishes, the stored
    // LOCKED=1 flag still makes the deletion a violation.
    writeFile(dir / "d/a.txt", "a");
    runCheck();
    writeFile(dir / "d/.bitbackuplock", "");
    runCheck();                                   // LOCKED=1 persisted
    ASSERT_EQ(queryRow("d/a.txt").locked, 1);

    fs::remove_all(dir / "d");                     // dir + marker + file all gone
    const std::string res = runCheck();
    EXPECT_NE(res.find("locked file deleted"), std::string::npos);
    const Row after = queryRow("d/a.txt");
    EXPECT_TRUE(after.exists);
    EXPECT_EQ(after.result, "KO");
}

TEST_F(CheckCommandLockTest, UnlockingResolvesPreviouslyReportedDeletion) {
    // Removing only the marker (the directory itself stays) must let a
    // deletion that was reported while locked finally resolve, per the
    // README's promise that removing .bitbackuplock resumes normal updates.
    writeFile(dir / "d/a.txt", "a");
    runCheck();
    writeFile(dir / "d/.bitbackuplock", "");
    runCheck();                                    // mark locked

    fs::remove(dir / "d/a.txt");
    const std::string res1 = runCheck();
    EXPECT_NE(res1.find("locked file deleted"), std::string::npos);
    ASSERT_TRUE(queryRow("d/a.txt").exists);

    fs::remove(dir / "d/.bitbackuplock");          // unlock only; "d" stays
    const std::string res2 = runCheck();
    EXPECT_EQ(res2, "");                            // no more violation
    EXPECT_FALSE(queryRow("d/a.txt").exists);       // row finally removed
}

TEST_F(CheckCommandLockTest, UnlockingResumesNormalUpdates) {
    writeFile(dir / "d/a.txt", "v1");
    runCheck();
    writeFile(dir / "d/.bitbackuplock", "");
    runCheck();
    ASSERT_EQ(queryRow("d/a.txt").locked, 1);

    fs::remove(dir / "d/.bitbackuplock");          // unlock
    const auto t = fs::last_write_time(dir / "d/a.txt");
    writeFile(dir / "d/a.txt", "v2 edited");
    fs::last_write_time(dir / "d/a.txt", t + std::chrono::hours(1));

    EXPECT_EQ(runCheck(), "");                      // normal edit, no violation
    const Row after = queryRow("d/a.txt");
    EXPECT_EQ(after.locked, 0);                     // cleared
    EXPECT_EQ(after.result, "OK");
}

TEST_F(CheckCommandLockTest, FilesOutsideLockedDirAreUnaffected) {
    writeFile(dir / "locked/a.txt", "a");
    writeFile(dir / "free/b.txt", "b");
    runCheck();
    writeFile(dir / "locked/.bitbackuplock", "");
    runCheck();

    EXPECT_EQ(queryRow("locked/a.txt").locked, 1);
    EXPECT_EQ(queryRow("free/b.txt").locked, 0);
}
