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


// import java.io.File;
// import java.io.IOException;
// import java.util.ArrayList;
// import java.util.Date;
// import java.util.List;
// import java.util.UUID;
// import java.util.stream.Collectors;
// import org.apache.logging.log4j.LogManager;
// import org.apache.logging.log4j.Logger;
// import com.robertvokac.bitbackup.core.BitBackupContext;
// import com.robertvokac.bitbackup.core.Command;
// import com.robertvokac.bitbackup.core.BitBackupArgs;
// import com.robertvokac.bitbackup.core.BitBackupException;
// import com.robertvokac.bitbackup.core.BitBackupFiles;
// import com.robertvokac.bitbackup.core.ListSet;
// import com.robertvokac.bitbackup.core.Utils;
// import com.robertvokac.bitbackup.entity.FsFile;
// import com.robertvokac.bitbackup.entity.SystemItem;
// import com.robertvokac.bitbackup.files.FileEntry;
// import com.robertvokac.bitbackup.persistence.api.FileRepository;
// import com.robertvokac.bitbackup.persistence.impl.sqlite.SqliteDatabaseMigration;
// import com.robertvokac.dbmigration.core.main.MigrationResult;
// import com.robertvokac.powerframework.time.duration.Duration;
// import com.robertvokac.powerframework.time.moment.LocalDateTime;
// import com.robertvokac.powerframework.time.utils.ProgressTracker;
// import com.robertvokac.powerframework.time.utils.TimeUnit;

#include "BitBackup/Core/Command.h"
#include "BitBackup/Commands/CheckCommand.h"
#include <string>

#include "BitBackup/Core/BitBackupContext.h"
#include "BitBackup/Core/BitBackupFiles.h"
#include "BitBackup/Core/ListSet.h"
#include <ctime>
#include <random>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>
#include <unistd.h>

#include "BitBackup/Core/ProgressTracker.h"
#include "BitBackup/Files/FileEntry.h"
#include "BitBackup/Persistence/Impl/Sqlite/MigrationResult.h"
#include "BitBackup/Persistence/Impl/Sqlite/SqliteDatabaseMigration.h"

namespace BitBackup::Core {
    class BitBackupException;
}

/**
 *
 * @author r
 */
namespace BitBackup::Commands {
    using std::string;

    namespace {
        // Run fn(i) for i in [0, count) across `threads` worker threads.
        // fn MUST be thread-safe and only touch per-index state. threads<=1 runs
        // sequentially (and reproduces the old single-threaded behavior exactly).
        template <typename Fn>
        void parallelFor(std::size_t count, unsigned threads, Fn&& fn) {
            if (count == 0) return;
            if (threads <= 1) {
                for (std::size_t i = 0; i < count; ++i) fn(i);
                return;
            }
            std::atomic<std::size_t> next{0};
            auto worker = [&] {
                std::size_t i;
                while ((i = next.fetch_add(1, std::memory_order_relaxed)) < count) {
                    fn(i);
                }
            };
            const unsigned n = static_cast<unsigned>(std::min<std::size_t>(threads, count));
            std::vector<std::thread> pool;
            pool.reserve(n);
            for (unsigned t = 0; t < n; ++t) pool.emplace_back(worker);
            for (auto& th : pool) th.join();
        }

        unsigned resolveThreadCount(const Core::BitBackupArgs& args) {
            if (args.hasArgument("threads")) {
                try {
                    const int t = std::stoi(args.getArgument("threads"));
                    if (t >= 1) return static_cast<unsigned>(std::min(t, 256));
                } catch (...) { /* fall through to default */ }
            }
            const unsigned hc = std::thread::hardware_concurrency();
            return hc == 0 ? 4u : hc;
        }
    }

    CheckCommand::CheckCommand() = default;

    string CheckCommand::getName() const {
        return NAME;
    }

    static int iStatic = 0;

    string CheckCommand::run(const Core::BitBackupArgs &bitBackupArgs) {
        Core::BitBackupFiles bitBackupFiles(bitBackupArgs);
        Core::BitBackupContext bitBackupContext(bitBackupFiles.workingDirAbsolutePath);

        // Resolve performance/scan options once.
        numThreads = resolveThreadCount(bitBackupArgs);
        quickMode = bitBackupArgs.hasArgument("quick") && bitBackupArgs.getArgument("quick") == "true";
        scrubPercent = 100;
        if (bitBackupArgs.hasArgument("scrub")) {
            try {
                scrubPercent = std::clamp(std::stoi(bitBackupArgs.getArgument("scrub")), 0, 100);
            } catch (...) { scrubPercent = 100; }
        }
        if (quickMode) scrubPercent = 0; // quick == scrub 0%
        cout << "Options: threads=" << numThreads
             << " quick=" << (quickMode ? "true" : "false")
             << " scrub=" << scrubPercent << "%" << std::endl;
        //
        //part 1:
        part1CheckDbHasExpectedHashSum(bitBackupFiles);
        //part 2:

        bool part2Result = part2MigrateDbSchemaIfNeeded(bitBackupFiles);
        if (!part2Result) {
            return "part 2 failed";
        }
        //part 3:
        part3UpdateVersionInDbIfNeeded(bitBackupContext);

        BitBackup::Core::ListSet<File> filesInFileSystem = part4FoundFilesInFileSystem(bitBackupFiles, bitBackupArgs);

        // } catch (BitBackup::Core::BitBackupException ex) {
        //     return "Part 4 failed: " + ex.getMessage();
        // }
        Core::ListSet<FsFile> filesInDb = part5FoundFilesInDb(*bitBackupContext.getFileRepository(), bitBackupArgs);

        tp now = part6AddNewFilesToDb(filesInFileSystem, bitBackupFiles, filesInDb, bitBackupContext);

        vector<FsFile> filesToBeRemovedFromDb = part7RemoveDeletedFilesFromDb(filesInDb, filesInFileSystem, bitBackupContext);

        vector<FsFile> filesWithBitRot = part8CompareContentAndLastModificationDate(filesInDb, filesToBeRemovedFromDb, bitBackupContext, now);

        part9CreateReportCsvIfNeeded(bitBackupArgs, bitBackupFiles, filesWithBitRot);
        part10CalculateCurrentHashSumOfDbFile(bitBackupFiles);

        cout << "==========" << endl;
        cout << "Summary" << endl;

        const bool problems = !filesWithBitRot.empty() || !lockViolations.empty();
        const bool tty = isatty(fileno(stderr)) != 0;
        const std::string RED = tty ? "\033[31m" : "";
        const std::string RST = tty ? "\033[0m" : "";

        if (!problems) {
            cout << "Summary: OK : No files with bit rot and no lock violations were found." << std::endl;
        } else {
            if (!filesWithBitRot.empty()) {
                std::cerr << RED
                << "Summary: KO : " << filesWithBitRot.size()
                << " file(s) with bit rot were found." << RST << std::endl;
                for (const FsFile& f : filesWithBitRot) {
                    std::cerr << RED
                    << "Bit rot detected: \"" << f.absolutePath << "\""
                    << " expected_sha512=" << f.hashSumValue
                    << " returned_sha512="
                    << Core::Utils::calculateSHA512Hash(File("./" + f.absolutePath))
                    << RST << std::endl;
                }
            }
            if (!lockViolations.empty()) {
                std::cerr << RED
                << "Summary: KO : " << lockViolations.size()
                << " lock violation(s) in .bitbackuplock'd directories:" << RST << std::endl;
                for (const std::string& v : lockViolations) {
                    std::cerr << RED << "Lock violation: " << v << RST << std::endl;
                }
            }
        }

        cout << "foundFiles=" << foundFiles << std::endl;
        cout << "foundDirs=" << foundDirs << endl;

        // A non-empty result signals problems (drives a non-zero exit code):
        // it aggregates both bit-rot paths and lock violations.
        if (!problems) {
            return "";
        }
        std::ostringstream oss;
        for (const FsFile& f : filesWithBitRot) {
            oss << "bitrot: " << f.absolutePath << "\n";
        }
        for (const std::string& v : lockViolations) {
            oss << "lock: " << v << "\n";
        }
        return oss.str();
    }

    bool CheckCommand::isPathLocked(const std::string& relPath) const {
        if (lockRoots.empty()) return false;
        if (lockRoots.count("")) return true;            // whole working dir locked
        // Check every ancestor directory of relPath.
        std::size_t pos = 0;
        while ((pos = relPath.find('/', pos)) != std::string::npos) {
            if (lockRoots.count(relPath.substr(0, pos))) return true;
            ++pos;
        }
        return false;
    }

    /**
     * Checks, if SQLite DB file has the expected SHA-512 hash sum
     *
     * @param bitBackupSQLite3File
     * @param bitBackupSQLite3FileSha512
     * @throws BitBackupException - if this check fails.
     */
    void CheckCommand::part1CheckDbHasExpectedHashSum(const Core::BitBackupFiles& bitBackupFiles) noexcept(false) {
        cout << "** Part "\
        << CheckCommandPart::CHECK_OLD_DB_CHECKSUM
        << ": Checking DB, if has expected check sum." << std::endl;
        File bitBackupSQLite3FileSha512 = bitBackupFiles.bitBackupSQLite3FileSha512;
        
        bool dbExists = std::filesystem::exists(bitBackupFiles.bitBackupSQLite3File);
        bool checkSumExists = std::filesystem::exists(bitBackupFiles.bitBackupSQLite3FileSha512);

        if (dbExists && checkSumExists) {

            string expectedHash = Core::Utils::readTextFromFile(bitBackupSQLite3FileSha512);
            string returnedHash = Core::Utils::calculateSHA512Hash(bitBackupFiles.bitBackupSQLite3File);
            if (returnedHash != expectedHash) {
                std::ostringstream oss;
                oss <<
                         "Part " << CheckCommandPart::CHECK_OLD_DB_CHECKSUM <<": KO. "
                        << "Unexpected hash "
                        << returnedHash
                        << ". Expected SHA-512 hash sum was: "
                        << expectedHash
                        << " for file "
                        << string(absolute(bitBackupFiles.bitBackupSQLite3File)) << std::endl;
                std::cerr << oss.str();

                cout << "Exiting because of the previous error." << std::endl;
                throw Core::BitBackupException(oss.str());
            }
        } else {
            cout << "Part "
            << CheckCommandPart::CHECK_OLD_DB_CHECKSUM
            << ": OK. Nothing to do: {}"
            << (!dbExists ? "DB does not yet exist." : "Check sum file does not exist.") << std::endl;


        }
    }

    bool CheckCommand::part2MigrateDbSchemaIfNeeded(Core::BitBackupFiles& bitBackupFiles) {
        cout << "** Part "
        << CheckCommandPart::MIGRATE_DB_SCHEMA_IF_NEEDED
        << ": Migrating schema, if needed." << std::endl;


            Persistence::Impl::Sqlite::MigrationResult migrationResult = Persistence::Impl::Sqlite::SqliteDatabaseMigration::getInstance()->migrate(bitBackupFiles.workingDirAbsolutePath);
            if (migrationResult == Persistence::Impl::Sqlite::MigrationResult::SUCCESS) {
                cout << "Part " << CheckCommandPart::MIGRATE_DB_SCHEMA_IF_NEEDED << ": OK. Success." << std::endl;
                return true;
            }
        std::cerr << "Part " << CheckCommandPart::MIGRATE_DB_SCHEMA_IF_NEEDED << ": KO. Failed." << CheckCommandPart::MIGRATE_DB_SCHEMA_IF_NEEDED << std::endl;
        return false;
    }

    void CheckCommand::part3UpdateVersionInDbIfNeeded(Core::BitBackupContext& bitBackupContext) {
        cout << "** Part "
        << CheckCommandPart::UPDATE_VERSION
        << ": Updating version, if needed." << std::endl;

        string bitBackupVersion = bitBackupContext.getSystemItemRepository()->read(BIBVERSION).value;
        cout << "Before: bib.version=" << bitBackupVersion << std::endl;
        if (bitBackupVersion == "") {
            bitBackupContext.getSystemItemRepository()->create(Entity::SystemItem("bib.version", "0.0.0-SNAPSHOT"));
        }
        cout << "Updating version in DB." << std::endl;
        bitBackupVersion = bitBackupContext.getSystemItemRepository()->read("bib.version").value;
        cout << "After: bib.version=" + bitBackupVersion << std::endl;
        cout << "Part "
        << CheckCommandPart::UPDATE_VERSION
        << ": OK." << std::endl;
    }

    Core::ListSet<File> CheckCommand::part4FoundFilesInFileSystem(
        Core::BitBackupFiles& bitBackupFiles,
        const Core::BitBackupArgs& bitBackupArgs
    ) {
        cout << "** Part " << CheckCommandPart::FOUND_FILES_IN_FILESYSTEM
             << ": Loading files in filesystem" << endl;

        const auto rootAbs = std::filesystem::absolute(bitBackupFiles.workingDir).string();
        // Hoist these out of the loop: they are constant across all entries, but
        // std::filesystem::absolute() does path work + allocates on every call,
        // so recomputing them per file was ~2 wasted absolute() calls per file.
        const auto dbAbs = std::filesystem::absolute(bitBackupFiles.bitBackupSQLite3File).string();
        const auto dbShaAbs = std::filesystem::absolute(bitBackupFiles.bitBackupSQLite3FileSha512).string();

        std::vector<File> found;

        found.reserve(200000);

        std::filesystem::recursive_directory_iterator it(bitBackupFiles.workingDir);
        const std::filesystem::recursive_directory_iterator end;
        for (; it != end; ++it) {
            const auto& e = *it;
            const auto abs = e.path().string();
            const auto rel = abs.substr(rootAbs.size() + 1);

            if (std::filesystem::is_directory(e)) {
                // Prune directories whose entire contents are ignored so we never
                // walk big ignored trees (.git, node_modules, ...). matchesDirectoryContents
                // only returns true when every child would be ignored anyway, so
                // this never drops a file that would otherwise be tracked.
                if (bitBackupFiles.bitBackupIgnoreRegex->matchesDirectoryContents(rel)) {
                    it.disable_recursion_pending();
                }
                continue;
            }

            if (!std::filesystem::is_regular_file(e))
                continue;

            if (e.path().filename().string() == BITBACKUPLOCK) {
                // This directory (and its subtree) is locked. Record it and do
                // not track the marker file itself.
                const auto slash = rel.find_last_of('/');
                lockRoots.insert(slash == std::string::npos
                                     ? std::string()
                                     : rel.substr(0, slash));
                continue;
            }

            if (abs == dbAbs)
                continue;
            if (abs == dbShaAbs)
                continue;

            if (bitBackupFiles.bitBackupIgnoreRegex->test(rel))
                continue;

            found.push_back(e.path());
        }

        cout << "Part " << CheckCommandPart::FOUND_FILES_IN_FILESYSTEM
             << ": Found " << found.size() << " files." << endl;

        return Core::ListSet<File>(
            std::move(found),
            [rootAbs](const File& f) {
                return std::filesystem::absolute(f).string().substr(rootAbs.size() + 1);
            }
        );
    }


    string CheckCommand::loadPathButOnlyTheNeededPart(const std::filesystem::path &currentDir, const std::filesystem::path &file) {
        string currentDirAbsolutePath = absolute(currentDir).string();
        string fileAbsolutePath = absolute(file).string();
        return fileAbsolutePath.substr(currentDirAbsolutePath.size() + 1);
    }
    std::vector<std::filesystem::path> CheckCommand::foundFilesInCurrentDir(
        const File& currentDir,
        std::vector<File>& filesAlreadyFound,
        Core::BitBackupFiles& bitBackupFiles,
        const Core::BitBackupArgs& bitBackupArgs,
        string& workingDir) {

        for (const auto& f : std::filesystem::directory_iterator(currentDir)) {
            bool isBitBackupIgnore = f.path().filename().string() == bitBackupFiles.bitBackupIgnore.filename().string();

            string fAbs = absolute(f).string();
            string bbAbs = std::filesystem::absolute(bitBackupFiles.bitBackupIgnore).string();
            if (isBitBackupIgnore && fAbs != bbAbs) {
                bitBackupFiles.bitBackupIgnoreRegex.get()->addBitBackupIgnoreFile(f, bitBackupFiles.workingDir);
            }
            if (is_directory(f)) {
                ++foundDirs;
                if (bitBackupArgs.isBitBackupIndexEnabled()) {
                    bitbackupindexSB << Files::FileEntry(f.path()).toCsvLine() << endl;
                }
                foundFilesInCurrentDir(
                    f,
                    filesAlreadyFound,
                    bitBackupFiles,
                    bitBackupArgs,
                    workingDir);
            } else {
                ++foundFiles;
                if (fAbs == absolute(bitBackupFiles.bitBackupSQLite3File)) {
                    continue;
                }
                if (fAbs == absolute(bitBackupFiles.bitBackupSQLite3FileSha512)) {
                    continue;
                }

                ++iii;
                //System.out.println("Testing file: " + iii + "#" + " " + loadPathButOnlyTheNeededPart(currentDirRoot, f));
                if (bitBackupFiles.bitBackupIgnoreRegex.get()->test(
                    loadPathButOnlyTheNeededPart(bitBackupFiles.workingDir, f))
                    ) {
                    continue;
                }
                if (bitBackupArgs.isBitBackupIndexEnabled()) {
                    auto csv = Files::FileEntry(f.path()).toCsvLine();

                    bitbackupindexSB << csv << endl;
                }
                filesAlreadyFound.push_back(f);

                if (bitBackupArgs.isVerboseLoggingEnabled() || iStatic % 100 == 0) {
                    cout << "Found file in file system: #" << (++iStatic) << " "
                    << absolute(f).string().substr(workingDir.length() + 1) << std::endl;
                }
            }
        }
        return filesAlreadyFound;
    }

    Core::ListSet<Entity::FsFile> CheckCommand::part5FoundFilesInDb(Persistence::Api::FileRepository& fileRepository, const Core::BitBackupArgs& bitBackupArgs) {
        cout << "** Part "<< CheckCommandPart::FOUND_FILES_IN_DB << ": Loading files in DB" << std::endl;
        vector<Entity::FsFile> filesInDb = fileRepository.list();

        Core::ListSet<Entity::FsFile> listSet((std::move(filesInDb)), []( const Entity::FsFile& f) {return f.absolutePath;});
        cout << "Part " << CheckCommandPart::FOUND_FILES_IN_DB << ": Found " << listSet.size() << " files." << std::endl;
        iStatic = 0;
        if (bitBackupArgs.isVerboseLoggingEnabled()) {

            for (const auto& f : listSet.getSet()) {
                std::cout << "#" << (++iStatic) << " " << f << std::endl;
            }

        }
        return listSet;
    }

    string print_clock(const tp& time) {
        auto duration = time.time_since_epoch();
        std::time_t timet = std::chrono::system_clock::to_time_t(time);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;

        std::stringstream ss;
        std::tm tmBuf{};
        ss << std::put_time(localtime_r(&timet, &tmBuf), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
        return ss.str();
    }
    string print_short_clock(const tp& time) {
        auto duration = time.time_since_epoch();
        std::time_t timet = std::chrono::system_clock::to_time_t(time);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;

        std::stringstream ss;
        std::tm tmBuf{};
        ss << std::put_time(localtime_r(&timet, &tmBuf), "%Y%m%d%H%M%S");
        ss << std::setfill('0') << std::setw(3) << milliseconds.count();
        return ss.str();
    }

    // tp lastModified(const std::string& filePath) {
    //     auto ftime = std::filesystem::last_write_time(filePath);
    //     auto sctp = std::chrono::system_clock::now() + (ftime - std::filesystem::file_time_type::clock::now());
    //     return sctp;
    // }
    std::string last_modified_string(const std::filesystem::path& p)
    {
        try
        {
            auto ftime = std::filesystem::last_write_time(p);

            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now()
                + std::chrono::system_clock::now()
            );

            auto tt = std::chrono::system_clock::to_time_t(sctp);

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                sctp.time_since_epoch()
            ) % 1000;

            std::tm tm{};
            localtime_r(&tt, &tm);

            std::stringstream ss;
            ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S.");
            ss << std::setfill('0') << std::setw(3) << ms.count();
            return ss.str();
        }
        catch (...)
        {
            return "1970-01-01 00:00:00.000";
        }
    }



    tp CheckCommand::part6AddNewFilesToDb(
        Core::ListSet<File>& filesInFileSystem,
        Core::BitBackupFiles& bitBackupFiles,
        Core::ListSet<Entity::FsFile>& filesInDb,
        Core::BitBackupContext& bitBackupContext) {

        cout
        << "** Part "
        << CheckCommandPart::ADD_NEW_FILES_TO_DB
        << ": Adding new files to DB"
        << std::endl;

        tp now = std::chrono::system_clock::now();
        const string nowStr = print_clock(now);

        // 1. Find files missing in the DB (cheap set lookups, single-threaded).
        struct NewFile { File path; string rel; };
        vector<NewFile> missing;
        for (const File& fileInDir : filesInFileSystem.getList()) {
            string rel = loadPathButOnlyTheNeededPart(bitBackupFiles.workingDir, fileInDir);
            if (filesInDb.doesSetContain(rel)) {
                continue;
            }
            if (isPathLocked(rel)) {
                // Frozen subtree: a new file is a violation and is NOT added.
                lockViolations.push_back("new file in locked directory: " + rel);
                continue;
            }
            missing.push_back({fileInDir, std::move(rel)});
        }

        // 2. Hash the new files in parallel (the expensive part).
        vector<Entity::FsFile> filesMissingInDb(missing.size());
        std::mutex errMtx;
        string firstError;
        parallelFor(missing.size(), numThreads, [&](std::size_t i) {
            const NewFile& m = missing[i];
            try {
                filesMissingInDb[i] = Entity::FsFile{
                    Core::Utils::generateUUIDv4(),
                    m.path.filename().string(),
                    m.rel,
                    last_modified_string(m.path),
                    nowStr,
                    Core::Utils::calculateSHA512Hash(m.path),
                    "SHA-512",
                    std::filesystem::file_size(m.path),
                    "OK"
                };
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lk(errMtx);
                if (firstError.empty()) firstError = e.what();
            }
        });
        if (!firstError.empty()) {
            throw Core::BitBackupException("Part 6 hashing failed: " + firstError);
        }

        cout << "Adding new files: " << filesMissingInDb.size() << std::endl;
        bitBackupContext.getFileRepository()->create(filesMissingInDb);
        return now;
    }

    vector<Entity::FsFile> CheckCommand::part7RemoveDeletedFilesFromDb(
        Core::ListSet<Entity::FsFile>& filesInDb,
        Core::ListSet<File>& filesInFileSystem,
        Core::BitBackupContext& bitBackupContext) {
        cout
        << "** Part "
        << CheckCommandPart::REMOVE_DELETED_FILES_FROM_DB
        << ": Removing deleted files from DB"<< std::endl;
        vector<Entity::FsFile> filesToBeRemovedFromDb;
        int processedCount = 0;

        for (const Entity::FsFile& fileInDb : filesInDb.getList()) {
            processedCount = processedCount + 1;
            if (processedCount % 100 == 0) {
                double progress = ((double) processedCount) / filesInDb.getList().size() * 100;
                cout
                << "Part "
                << CheckCommandPart::REMOVE_DELETED_FILES_FROM_DB
                <<": Remove - Progress: " << processedCount
                <<"/" << filesInDb.getList().size()
                << std::fixed << std::setprecision(2)
                <<" " << progress
                << "%" << std::endl;
            }

            string absolutePathOfFileInDb = fileInDb.absolutePath;
            if (!filesInFileSystem.doesSetContain(absolutePathOfFileInDb)) {
                // Gone from disk: part8 must skip it (cannot be hashed).
                missingFromDiskIds.insert(fileInDb.id);
                // A file that was (or still is) inside a locked subtree must not
                // be silently dropped from the DB - report it and keep the row.
                if (fileInDb.locked == 1 || isPathLocked(absolutePathOfFileInDb)) {
                    lockViolations.push_back("locked file deleted: " + absolutePathOfFileInDb);
                    continue;
                }
                filesToBeRemovedFromDb.push_back(fileInDb);
            }

        }
        cout
        << "Part " << CheckCommandPart::REMOVE_DELETED_FILES_FROM_DB
        <<": Removing files: " << filesToBeRemovedFromDb.size() << std::endl;


        // One transaction for the whole batch instead of a fresh DB connection
        // + auto-commit (fsync) per deleted row.
        bitBackupContext.getFileRepository()->removeAll(filesToBeRemovedFromDb);
        return filesToBeRemovedFromDb;
    }

    std::string seconds_to_duration_string(long total_seconds) {
        const long seconds_per_day = 86400;
        const long seconds_per_hour = 3600;
        const long seconds_per_minute = 60;

        long days = total_seconds / seconds_per_day;
        total_seconds %= seconds_per_day;

        long hours = total_seconds / seconds_per_hour;
        total_seconds %= seconds_per_hour;

        long minutes = total_seconds / seconds_per_minute;
        long seconds = total_seconds % seconds_per_minute;

        std::ostringstream oss;

        if (days > 0) {
            oss << days << " day" << (days > 1 ? "s " : " ");
        }

        if (hours > 0) {
            oss << hours << " hour" << (hours > 1 ? "s " : " ");
        }

        if (minutes > 0) {
            oss << minutes << " minute" << (minutes > 1 ? "s " : " ");
        }

        oss << seconds << " second" << (seconds > 1 ? "s" : "");

        return oss.str();
    }

    vector<FsFile> CheckCommand::part8CompareContentAndLastModificationDate(
        Core::ListSet<FsFile>& filesInDb, const vector<FsFile>& filesToBeRemovedFromDb,
        Core::BitBackupContext& bitBackupContext,
        tp& now) {
        cout << "** Part "
        << CheckCommandPart::COMPARE_CONTENT_AND_LAST_MODTIME
        << ": Comparing Content and last modification date" << std::endl;
        //// Update modified files with same last modification date
        vector<FsFile> filesWithBitRot;
        vector<FsFile> filesToUpdateLastCheckDate;
        vector<FsFile> filesToUpdate;
        int contentAndModTimeWereChanged = 0;

        // Build the work list of files still present on disk. missingFromDiskIds
        // (filled by part7) covers both normal deletions and locked deletions
        // that are kept in the DB - neither can be hashed here. Pointers stay
        // valid: dbList is the ListSet's backing vector.
        const auto& dbList = filesInDb.getList();
        vector<const FsFile*> work;
        work.reserve(dbList.size());
        for (const auto& f : dbList) {
            if (!missingFromDiskIds.count(f.id)) work.push_back(&f);
        }

        // Decide which unchanged-modtime files to re-hash this run:
        //   scrubPercent==100 -> all (full bit-rot check, the default)
        //   scrubPercent==0   -> none (quick mode)
        //   else              -> the oldest scrubPercent% by stored last-check date
        std::unordered_set<const FsFile*> forceHash;
        if (scrubPercent > 0 && scrubPercent < 100) {
            vector<const FsFile*> byAge = work;
            std::sort(byAge.begin(), byAge.end(),
                      [](const FsFile* a, const FsFile* b) {
                          return a->lastCheckDate < b->lastCheckDate;
                      });
            const std::size_t scrubCount =
                static_cast<std::size_t>(byAge.size() * (scrubPercent / 100.0));
            for (std::size_t i = 0; i < scrubCount; ++i) forceHash.insert(byAge[i]);
        }

        const string workingDir = bitBackupContext.getWorkingDirectory();
        const string nowStr = print_clock(now);

        struct ScanResult {
            bool ok = true;
            string err;
            string lastModified;
            bool modtimeChanged = false;
            bool hashed = false;
            bool locked = false;
            string hash;
            unsigned long size = 0;
        };
        vector<ScanResult> results(work.size());

        // Parallel phase: the stat + (conditional) SHA-512 of every file. This is
        // the dominant cost of a check and is independent per file.
        std::atomic<std::size_t> hashedSoFar{0};
        std::mutex progressMtx;
        parallelFor(work.size(), numThreads, [&](std::size_t i) {
            const FsFile& f = *work[i];
            ScanResult r;
            try {
                File file(workingDir + "/" + f.absolutePath);
                r.lastModified = last_modified_string(file);
                r.modtimeChanged = r.lastModified != f.lastModificationDate;
                r.size = static_cast<unsigned long>(std::filesystem::file_size(file));
                r.locked = isPathLocked(f.absolutePath);

                bool needsHash;
                if (r.locked)                 needsHash = true;   // frozen -> always verify
                else if (scrubPercent >= 100) needsHash = true;   // full (default)
                else if (r.modtimeChanged)    needsHash = true;   // changed -> must rehash
                else if (scrubPercent <= 0)   needsHash = false;  // quick
                else                          needsHash = forceHash.count(work[i]) > 0;

                if (needsHash) {
                    r.hash = Core::Utils::calculateSHA512Hash(file);
                    r.hashed = true;
                    const std::size_t d = hashedSoFar.fetch_add(1) + 1;
                    if (d % 2000 == 0) {
                        std::lock_guard<std::mutex> lk(progressMtx);
                        cout << "Update - hashed " << d << " files" << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                r.ok = false;
                r.err = e.what();
            }
            results[i] = std::move(r);
        });

        // Single-threaded reduce: identical classification and ordering as the old
        // sequential loop, only the hashing was moved off the critical path. When
        // scrubPercent==100 (the default) every file is hashed, so this reproduces
        // the original behavior exactly.
        for (std::size_t i = 0; i < work.size(); ++i) {
            const ScanResult& r = results[i];
            if (!r.ok) {
                throw Core::BitBackupException("Part 8 hashing failed: " + r.err);
            }
            FsFile fileInDb = *work[i];

            if (r.locked) {
                // Frozen subtree: never overwrite the stored mtime/hash/size -
                // they are the source of truth. Report any drift as a violation.
                const bool drift = r.modtimeChanged ||
                                   (r.hashed && r.hash != fileInDb.hashSumValue);
                fileInDb.lastCheckDate = nowStr;
                fileInDb.locked = 1;
                if (drift) {
                    lockViolations.push_back("locked file modified: " + fileInDb.absolutePath);
                    fileInDb.lastCheckResult = "KO";
                } else {
                    fileInDb.lastCheckResult = "OK";
                }
                filesToUpdate.push_back(fileInDb);  // writes back unchanged mtime/hash + LOCKED=1
                continue;
            }

            if (r.modtimeChanged) {
                fileInDb.lastCheckDate = nowStr;
                fileInDb.lastModificationDate = r.lastModified;
                fileInDb.hashSumValue = r.hash;
                fileInDb.hashSumAlgorithm = "SHA-512";
                fileInDb.size = r.size;
                fileInDb.lastCheckResult = "OK";
                fileInDb.locked = 0;
                filesToUpdate.push_back(fileInDb);
                contentAndModTimeWereChanged++;
            } else if (r.hashed) {
                if (r.hash != fileInDb.hashSumValue) {
                    filesWithBitRot.push_back(fileInDb);
                    fileInDb.lastCheckDate = nowStr;
                    fileInDb.lastCheckResult = "KO";
                    fileInDb.locked = 0;
                    filesToUpdate.push_back(fileInDb);
                } else {
                    fileInDb.lastCheckResult = "OK";
                    if (fileInDb.size == 0) {
                        fileInDb.size = r.size;
                        fileInDb.locked = 0;
                        filesToUpdate.push_back(fileInDb);
                    } else {
                        filesToUpdateLastCheckDate.push_back(fileInDb);
                    }
                }
            } else {
                // Hash skipped (quick mode / not in this run's scrub subset):
                // modtime unchanged, so assume OK without a bit-rot check.
                fileInDb.lastCheckResult = "OK";
                if (fileInDb.size == 0) {
                    fileInDb.size = r.size;
                    fileInDb.locked = 0;
                    filesToUpdate.push_back(fileInDb);
                } else {
                    filesToUpdateLastCheckDate.push_back(fileInDb);
                }
            }
        }

        // Flush all per-file mutations (bit rot, modified, zero-size fixups) in a
        // single transaction instead of one fsync per updated row.
        bitBackupContext.getFileRepository()->updateAll(filesToUpdate);
        cout << "Part 8: Updating files - "
             << (filesWithBitRot.empty() ? "no" : "some")
             << " files with bit rots - content was changed and last modification is the same: "
             << filesWithBitRot.size()
             << std::endl;

        cout << "Part 8: Updating files - content and last modification date were changed: "
        << contentAndModTimeWereChanged << std::endl;
        cout << "Part 8: Updating files - content and last modification date were not changed: "
        <<filesToUpdateLastCheckDate.size() << std::endl;
        string nowS = print_clock(now);
        bitBackupContext.getFileRepository()->updateLastCheckDate(nowS, filesToUpdateLastCheckDate);

        return filesWithBitRot;
    }

    void CheckCommand::part9CreateReportCsvIfNeeded(
        const Core::BitBackupArgs& bitBackupArgs,
        Core::BitBackupFiles& bitBackupFiles,
        vector<FsFile>& filesWithBitRot) {
        cout << "** Part 9: Creating csv report, if needed" << std::endl;
        if (!bitBackupArgs.hasArgument("report")) {
            cout << " Part 9: OK. Nothing to do. No option report was passed." << std::endl;
            return;
        }
        if (!(bitBackupArgs.getArgument("report") == "true")) {
            cout
            << "Part 9: Nothing to do. Option report={}"
            << bitBackupArgs.getArgument("report") << std::endl;

            return;
        }

        File bibReportCsv = bitBackupFiles.bitBackupReportCsv;
        if (std::filesystem::exists(bibReportCsv)) {

            auto now = std::chrono::system_clock::now();

            File backup(bibReportCsv.parent_path().string() + "/" + print_short_clock(now) + "." + bibReportCsv.filename().string());
            std::rename(absolute(bibReportCsv).c_str(),absolute(backup).c_str());
        }

        std::stringstream sb;

        if (!filesWithBitRot.empty()) {
            sb <<"file;expected;calculated" << std::endl;;
        }
        for (FsFile const &f: filesWithBitRot) {
            File file("./" + f.absolutePath);
            sb << f.absolutePath
                    << ";"
                    << f.hashSumValue
                    << ";"
                    << Core::Utils::calculateSHA512Hash(file)
                    << std::endl;
        }


        Core::Utils::writeTextToFile(sb.str(), bibReportCsv);
        cout << "Part 9: OK." << std::endl;
    }

    void CheckCommand::part10CalculateCurrentHashSumOfDbFile(Core::BitBackupFiles& bitBackupFiles) {
        cout << "** Part 10: Calculating current hash sum of DB file" << std::endl;
        Core::Utils::writeTextToFile(
            Core::Utils::calculateSHA512Hash(bitBackupFiles.bitBackupSQLite3File),
            bitBackupFiles.bitBackupSQLite3FileSha512
            );
    }




}
