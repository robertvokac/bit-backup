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

// End-to-end tests for the `check` command, driving CheckCommand::run() against
// a temporary directory. The headline case is silent bit rot: change a file's
// CONTENT but restore its ORIGINAL modification time, and verify bit-backup
// still flags it (this is the whole point of the tool).

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

} // namespace

class CheckCommandBitRotTest : public ::testing::Test {
protected:
    fs::path dir;
    fs::path oldCwd;

    void SetUp() override {
        oldCwd = fs::current_path();
        dir = fs::temp_directory_path() /
              (std::string("bitbackup_bitrot_") +
               ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(dir);
        fs::create_directories(dir);
        // run() uses CWD-relative paths for the bit-rot summary, so run from dir.
        fs::current_path(dir);
    }

    void TearDown() override {
        fs::current_path(oldCwd);
        std::error_code ec;
        fs::remove_all(dir, ec);
    }

    // Run `check` with stdout/stderr captured so the test output stays clean.
    // Returns CheckCommand::run()'s value: newline-joined paths of files with
    // detected bit rot, or "" when none.
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

    // Read the stored verdict (LAST_CHECK_RESULT) for a file straight from the
    // SQLite DB the command just wrote.
    std::string queryResult(const std::string& relPath) {
        SQLite::Database db((dir / ".bitbackup.sqlite3").string(), SQLite::OPEN_READONLY);
        SQLite::Statement q(db, "SELECT LAST_CHECK_RESULT FROM FILE WHERE ABSOLUTE_PATH = ?");
        q.bind(1, relPath);
        if (q.executeStep()) {
            return q.getColumn(0).getString();
        }
        return "<missing>";
    }
};

TEST_F(CheckCommandBitRotTest, SilentBitRotIsDetectedWhenModtimeUnchanged) {
    writeFile(dir / "data.bin", "original content");

    // First check: indexes the file, no bit rot.
    EXPECT_EQ(runCheck(), "");
    EXPECT_EQ(queryResult("data.bin"), "OK");

    // Capture the current mtime, corrupt the content, then RESTORE the old mtime
    // -> the file looks untouched by timestamp but its bytes changed.
    const auto savedMtime = fs::last_write_time(dir / "data.bin");
    writeFile(dir / "data.bin", "CORRUPTED bytes!");
    fs::last_write_time(dir / "data.bin", savedMtime);

    // Second check: must flag the file as bit-rotten.
    const std::string result = runCheck();
    EXPECT_NE(result.find("data.bin"), std::string::npos)
        << "bit rot was not reported; run() returned: '" << result << "'";
    EXPECT_EQ(queryResult("data.bin"), "KO");
}

TEST_F(CheckCommandBitRotTest, NormalEditWithNewModtimeIsNotBitRot) {
    writeFile(dir / "doc.txt", "v1");
    EXPECT_EQ(runCheck(), "");

    // Edit content AND let the modtime advance (a normal edit).
    const auto t = fs::last_write_time(dir / "doc.txt");
    writeFile(dir / "doc.txt", "v2 updated");
    fs::last_write_time(dir / "doc.txt", t + std::chrono::hours(1));

    EXPECT_EQ(runCheck(), "");                       // not bit rot
    EXPECT_EQ(queryResult("doc.txt"), "OK");
}

TEST_F(CheckCommandBitRotTest, QuickModeSkipsBitRotDetection) {
    writeFile(dir / "data.bin", "original");
    EXPECT_EQ(runCheck(), "");

    const auto savedMtime = fs::last_write_time(dir / "data.bin");
    writeFile(dir / "data.bin", "CORRUPTED");
    fs::last_write_time(dir / "data.bin", savedMtime);

    // quick=true skips re-hashing unchanged-modtime files, so the silent rot is
    // intentionally NOT detected.
    EXPECT_EQ(runCheck({"check", "quick=true"}), "");
    EXPECT_EQ(queryResult("data.bin"), "OK");

    // A subsequent full check still catches it.
    EXPECT_NE(runCheck().find("data.bin"), std::string::npos);
    EXPECT_EQ(queryResult("data.bin"), "KO");
}

TEST_F(CheckCommandBitRotTest, UnchangedFileStaysOk) {
    writeFile(dir / "stable.dat", "constant");
    EXPECT_EQ(runCheck(), "");
    EXPECT_EQ(runCheck(), "");                       // second run, still clean
    EXPECT_EQ(queryResult("stable.dat"), "OK");
}
