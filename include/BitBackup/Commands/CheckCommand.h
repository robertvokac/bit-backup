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
#ifndef CHECKCOMMAND_H
#define CHECKCOMMAND_H

#include "BitBackup/Core/Command.h"
#include <string>

#include "BitBackup/Core/BitBackupContext.h"
#include "BitBackup/Core/BitBackupException.h"
#include "BitBackup/Core/BitBackupFiles.h"
#include "BitBackup/Core/ListSet.h"
#include <iostream>
#include <unordered_set>
#include <vector>

namespace BitBackup::Commands {
    using std::string;
    using std::cout;
    using std::endl;
    using Entity::FsFile;
    using std::vector;

    typedef std::filesystem::path File;

    typedef std::chrono::system_clock::time_point tp;

    /**
     *
    *
     */
    class CheckCommand : public Core::Command {
    private:
        enum CheckCommandPart {
            CHECK_OLD_DB_CHECKSUM = 1,
            MIGRATE_DB_SCHEMA_IF_NEEDED = 2,
            UPDATE_VERSION = 3,
            FOUND_FILES_IN_FILESYSTEM = 4,
            FOUND_FILES_IN_DB = 5,
            ADD_NEW_FILES_TO_DB = 6,
            REMOVE_DELETED_FILES_FROM_DB = 7,
            COMPARE_CONTENT_AND_LAST_MODTIME = 8,
            CREATE_REPORT_CSV_IF_NEEDED = 9,
            CHECK_NEW_DB_CHECKSUM = 10
        };

        int foundFiles = 0;
        int foundDirs = 0;
        std::stringstream bitbackupindexSB;

        // Resolved once at the start of run() from the CLI arguments:
        //   threads=N   number of hashing worker threads (default: hw concurrency)
        //   quick=true  skip re-hashing files whose modtime is unchanged
        //               (fast, but does NOT detect silent bit rot)
        //   scrub=N     re-hash only the oldest N% of unchanged-modtime files
        //               this run (rotating coverage); 100 = full, 0 = quick
        unsigned numThreads = 1;
        bool quickMode = false;
        int scrubPercent = 100;

        // Directories (relative to the working dir) that contain a .bitbackuplock
        // marker; "" means the working dir root itself is locked. A file is locked
        // when any of its ancestor directories is one of these.
        std::unordered_set<std::string> lockRoots;
        // Human-readable lock violations collected during a run (modified, added,
        // or deleted files inside a locked subtree).
        std::vector<std::string> lockViolations;
        // IDs of DB rows whose file is gone from disk (normal deletions AND
        // locked deletions that are kept). part8 must not try to hash these.
        std::unordered_set<std::string> missingFromDiskIds;
        // Locked rows whose file was deleted: kept in the DB but flagged KO.
        std::vector<Entity::FsFile> lockedDeletedFiles;

        static constexpr const char* BITBACKUPLOCK = ".bitbackuplock";

        [[nodiscard]] bool isPathLocked(const std::string& relPath) const;

        void part1CheckDbHasExpectedHashSum(const Core::BitBackupFiles &bitBackupFiles) noexcept(false);

        bool part2MigrateDbSchemaIfNeeded(Core::BitBackupFiles &bitBackupFiles);

        void part3UpdateVersionInDbIfNeeded(
            Core::BitBackupContext& bitBackupContext);

        Core::ListSet<std::filesystem::path> part4FoundFilesInFileSystem(
            Core::BitBackupFiles& bitBackupFiles, const Core::BitBackupArgs &bitBackupArgs);

        string loadPathButOnlyTheNeededPart(const std::filesystem::path &currentDir, const std::filesystem::path &file);

        std::vector<std::filesystem::path> foundFilesInCurrentDir(const File &currentDir,
                                                                  std::vector<File> &filesAlreadyFound,
                                                                  Core::BitBackupFiles &bitBackupFiles,
                                                                  const Core::BitBackupArgs &bitBackupArgs,
                                                                  string &workingDir);

        Core::ListSet<Entity::FsFile> part5FoundFilesInDb(
            Persistence::Api::FileRepository &fileRepository, const Core::BitBackupArgs &bitBackupArgs);

        tp part6AddNewFilesToDb(
            Core::ListSet<File> &filesInFileSystem,
            Core::BitBackupFiles &bitBackupFiles,
            Core::ListSet<Entity::FsFile> &filesInDb,
            Core::BitBackupContext &bitBackupContext);

        vector<Entity::FsFile> part7RemoveDeletedFilesFromDb(
            Core::ListSet<Entity::FsFile> &filesInDb,
            Core::ListSet<File> &filesInFileSystem,
            Core::BitBackupContext &bitBackupContext);

        vector<FsFile> part8CompareContentAndLastModificationDate(
            Core::ListSet<FsFile> &filesInDb, const vector<FsFile>& filesToBeRemovedFromDb,
            Core::BitBackupContext &bitBackupContext,
            tp &now);

        void part9CreateReportCsvIfNeeded(
            const Core::BitBackupArgs& bitBackupArgs,
            Core::BitBackupFiles &bitBackupFiles,
            vector<FsFile> &filesWithBitRot);

        void part10CalculateCurrentHashSumOfDbFile(Core::BitBackupFiles& bitBackupFiles);

    public:


        static constexpr std::string NAME = "check";
        static constexpr std::string BIBVERSION = "bib.version";

        CheckCommand();

        [[nodiscard]] string getName() const override;

        string run(const Core::BitBackupArgs &bitBackupArgs) override;
    };
    static int iii = 0;
}
#endif // CHECKCOMMAND_H
