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

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "BitBackup/Core/BitBackupIgnoreRegex.h"

namespace fs = std::filesystem;
using BitBackup::Core::BitBackupIgnoreRegex;

namespace {

// Write a .bitbackupignore with the given content to a unique temp path and
// return that path. The file is overwritten fresh each call.
fs::path writeIgnore(const std::string& name, const std::string& content) {
    fs::path dir = fs::temp_directory_path() / ("bitbackup_ignore_test_" + name);
    fs::remove_all(dir);
    fs::create_directories(dir);
    fs::path file = dir / ".bitbackupignore";
    std::ofstream(file) << content;
    return file;
}

} // namespace

TEST(BitBackupIgnoreRegexTest, BuiltInMetadataFilesAreIgnored) {
    // No file on disk -> only the built-in patterns are active.
    BitBackupIgnoreRegex re(fs::path("/nonexistent/.bitbackupignore"));
    EXPECT_TRUE(re.test(".bitbackupignore"));
    EXPECT_TRUE(re.test(".bitbackupindex.csv"));
    EXPECT_TRUE(re.test("20240101.bitbackupreport.csv"));
    EXPECT_TRUE(re.test("sub/.bitbackupignore"));
    EXPECT_FALSE(re.test("keep.txt"));
}

TEST(BitBackupIgnoreRegexTest, ExtensionGlobMatchesAtAnyDepth) {
    BitBackupIgnoreRegex re(writeIgnore("ext", "*.tmp\n"));
    EXPECT_TRUE(re.test("a.tmp"));
    EXPECT_TRUE(re.test("sub/deep/b.tmp"));
    EXPECT_FALSE(re.test("a.txt"));
}

TEST(BitBackupIgnoreRegexTest, LeadingSlashPatternIsAnchoredToRoot) {
    // This is the README "/logs/*" style that used to never match because the
    // tested path has no leading slash.
    BitBackupIgnoreRegex re(writeIgnore("anchor", "/logs/*\n/node_modules/*\n"));
    EXPECT_TRUE(re.test("logs/a.log"));
    EXPECT_TRUE(re.test("node_modules/deep/x.js"));
    EXPECT_FALSE(re.test("keep.txt"));
    // anchored: only at the root, not nested under another dir
    EXPECT_FALSE(re.test("src/logs/a.log"));
}

TEST(BitBackupIgnoreRegexTest, CarriageReturnAndTrailingSpacesAreStripped) {
    // Simulate a CRLF-edited ignore file plus trailing spaces.
    BitBackupIgnoreRegex re(writeIgnore("crlf", "*.log  \r\n/build/*\t\r\n"));
    EXPECT_TRUE(re.test("server.log"));
    EXPECT_TRUE(re.test("build/out.o"));
}

TEST(BitBackupIgnoreRegexTest, CommentsAndBlankLinesAreSkipped) {
    BitBackupIgnoreRegex re(writeIgnore("comments", "# a comment\n\n*.bak\n"));
    EXPECT_TRUE(re.test("x.bak"));
    EXPECT_FALSE(re.test("a comment"));
    EXPECT_FALSE(re.test(""));
}

TEST(BitBackupIgnoreRegexTest, QuestionMarkMatchesSingleChar) {
    BitBackupIgnoreRegex re(writeIgnore("qmark", "file?.txt\n"));
    EXPECT_TRUE(re.test("file1.txt"));
    EXPECT_TRUE(re.test("fileA.txt"));
    EXPECT_FALSE(re.test("file.txt"));
    EXPECT_FALSE(re.test("file10.txt"));
}

TEST(BitBackupIgnoreRegexTest, UnrelatedFilesAreKept) {
    BitBackupIgnoreRegex re(writeIgnore("keep", "*.tmp\n/logs/*\n"));
    EXPECT_FALSE(re.test("src/main.cpp"));
    EXPECT_FALSE(re.test("README.md"));
    EXPECT_FALSE(re.test("data/important.bin"));
}

TEST(BitBackupIgnoreRegexTest, DirectoryPruneOnlyWhenAllChildrenIgnored) {
    BitBackupIgnoreRegex re(writeIgnore("prune", "/logs/*\n/node_modules/*\n"));
    // whole subtree ignored -> safe to prune
    EXPECT_TRUE(re.matchesDirectoryContents("logs"));
    EXPECT_TRUE(re.matchesDirectoryContents("node_modules"));
    // unrelated dir -> must keep descending
    EXPECT_FALSE(re.matchesDirectoryContents("src"));
}

TEST(BitBackupIgnoreRegexTest, DoNotPruneWhenOnlyDirNameMatches) {
    // Bare "build" matches a file/dir named build, but NOT build/out.o, so the
    // directory must NOT be pruned (build/out.o would still be tracked).
    BitBackupIgnoreRegex re(writeIgnore("bare", "build\n"));
    EXPECT_TRUE(re.test("build"));
    EXPECT_FALSE(re.matchesDirectoryContents("build"));
}

TEST(BitBackupIgnoreRegexTest, DoNotPruneExtensionDirWithKeptChildren) {
    // "*.tmp" matches foo.tmp itself, but foo.tmp/bar.txt does not end in .tmp,
    // so pruning foo.tmp would wrongly drop a tracked file.
    BitBackupIgnoreRegex re(writeIgnore("exttmp", "*.tmp\n"));
    EXPECT_FALSE(re.matchesDirectoryContents("foo.tmp"));
}

TEST(BitBackupIgnoreRegexTest, NestedDirectoryPrune) {
    BitBackupIgnoreRegex re(writeIgnore("nested", "/logs/keep/*\n"));
    EXPECT_FALSE(re.matchesDirectoryContents("logs"));      // logs/other must be kept
    EXPECT_TRUE(re.matchesDirectoryContents("logs/keep"));  // whole keep subtree ignored
}
