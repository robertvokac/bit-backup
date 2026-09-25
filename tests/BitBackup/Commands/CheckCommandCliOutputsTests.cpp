/*
 * MIT License
 * Copyright (c) 2023-2026 Robert Vokac
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
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "BitBackup/Commands/CheckCommand.h"
#include "BitBackup/Core/BitBackupArgs.h"
#include "BitBackup/Core/Utils.h"

namespace fs = std::filesystem;
using BitBackup::Commands::CheckCommand;
using BitBackup::Core::BitBackupArgs;
using BitBackup::Core::Utils;

namespace {

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << content;
}

std::string readFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

struct CheckOutput {
    std::string result;
    std::string errors;
};

} // namespace

class CheckCommandCliOutputsTest : public ::testing::Test {
protected:
    fs::path dir;

    void SetUp() override {
        dir = fs::temp_directory_path() /
              (std::string("bitbackup_cli_outputs_") +
               ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(dir);
        fs::create_directories(dir);
    }

    void TearDown() override {
        std::error_code error;
        fs::remove_all(dir, error);
    }

    CheckOutput runCheck(const std::vector<std::string>& options, const fs::path& target = {}) {
        const fs::path scanned = target.empty() ? dir : target;
        std::vector<std::string> args = {"check", "dir=" + scanned.string()};
        args.insert(args.end(), options.begin(), options.end());

        std::stringstream output;
        std::stringstream errors;
        std::streambuf* oldOut = std::cout.rdbuf(output.rdbuf());
        std::streambuf* oldErr = std::cerr.rdbuf(errors.rdbuf());
        std::string result;
        try {
            CheckCommand command;
            result = command.run(BitBackupArgs(args));
        } catch (...) {
            std::cout.rdbuf(oldOut);
            std::cerr.rdbuf(oldErr);
            throw;
        }
        std::cout.rdbuf(oldOut);
        std::cerr.rdbuf(oldErr);
        return {result, errors.str()};
    }
};

TEST_F(CheckCommandCliOutputsTest, IndexListsScannedFilesAndSkipsIgnoredMetadata) {
    writeFile(dir / "keep.txt", "hello");
    writeFile(dir / "nested/data.bin", "data");
    writeFile(dir / "semi;quote\".txt", "x");
    writeFile(dir / "skip.tmp", "ignored");
    writeFile(dir / ".bitbackupreport.csv", "previous report");
    writeFile(dir / ".bitbackupignore",
              "*.tmp\n!.bitbackupindex.csv\n!.bitbackupignore\n!.bitbackupreport.csv\n");

    EXPECT_EQ(runCheck({"bitbackupindex=true"}).result, "");

    const fs::path indexPath = dir / ".bitbackupindex.csv";
    ASSERT_TRUE(fs::exists(indexPath));
    const std::string index = readFile(indexPath);
    EXPECT_EQ(index,
              "path;size;sha512\n"
              "keep.txt;5;" + Utils::calculateSHA512Hash(dir / "keep.txt") + "\n" +
              "nested/data.bin;4;" + Utils::calculateSHA512Hash(dir / "nested/data.bin") + "\n" +
              "\"semi;quote\"\".txt\";1;" + Utils::calculateSHA512Hash(dir / "semi;quote\".txt") + "\n");
    EXPECT_EQ(runCheck({"bitbackupindex=true"}).result, "");
    EXPECT_EQ(readFile(indexPath), index);
}

TEST_F(CheckCommandCliOutputsTest, ForeignCwdBitRotSummaryAndReportUseScannedDirectory) {
    writeFile(dir / "data.bin", "original");
    const std::string expectedHash = Utils::calculateSHA512Hash(dir / "data.bin");
    ASSERT_EQ(runCheck({"report=true"}).result, "");

    const auto savedMtime = fs::last_write_time(dir / "data.bin");
    writeFile(dir / "data.bin", "corrupted");
    fs::last_write_time(dir / "data.bin", savedMtime);
    const std::string actualHash = Utils::calculateSHA512Hash(dir / "data.bin");

    const CheckOutput check = runCheck({"report=true"});
    EXPECT_NE(check.result.find("bitrot: data.bin"), std::string::npos);
    EXPECT_NE(check.errors.find("expected_sha512=" + expectedHash), std::string::npos);
    EXPECT_NE(check.errors.find("returned_sha512=" + actualHash), std::string::npos);
    EXPECT_EQ(readFile(dir / ".bitbackupreport.csv"),
              "file;expected;calculated\n"
              "data.bin;" + expectedHash + ";" + actualHash + "\n");
}

TEST_F(CheckCommandCliOutputsTest, RelativeDirArgScansAndIndexesTheSelectedDirectory) {
    writeFile(dir / "keep.txt", "hello");
    writeFile(dir / "skip.tmp", "ignored");
    writeFile(dir / ".bitbackupignore", "*.tmp\n");

    const fs::path relativeDir = fs::relative(dir, fs::current_path());
    ASSERT_TRUE(relativeDir.is_relative());
    EXPECT_EQ(runCheck({"bitbackupindex=true"}, relativeDir).result, "");
    EXPECT_EQ(readFile(dir / ".bitbackupindex.csv"),
              "path;size;sha512\n"
              "keep.txt;5;" + Utils::calculateSHA512Hash(dir / "keep.txt") + "\n");

    const auto savedMtime = fs::last_write_time(dir / "keep.txt");
    writeFile(dir / "keep.txt", "rot!!");
    fs::last_write_time(dir / "keep.txt", savedMtime);
    const CheckOutput check = runCheck({"report=true"}, relativeDir);
    EXPECT_NE(check.result.find("bitrot: keep.txt"), std::string::npos);
    EXPECT_NE(readFile(dir / ".bitbackupreport.csv").find("keep.txt;"), std::string::npos);
}
