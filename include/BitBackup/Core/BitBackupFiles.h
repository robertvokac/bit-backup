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

#ifndef BITBACKUPFILES_H
#define BITBACKUPFILES_H

#include <filesystem>
#include <memory>
#include "BitBackupIgnoreRegex.h"
#include "BitBackupArgs.h"

namespace BitBackup::Core {
    struct BitBackupFiles {

        std::filesystem::path workingDir;
        std::string workingDirAbsolutePath;
        std::filesystem::path bitBackupSQLite3File;
        std::filesystem::path bitBackupSQLite3FileSha512;
        std::filesystem::path bitBackupIgnore;
        std::shared_ptr<BitBackupIgnoreRegex> bitBackupIgnoreRegex;
        std::filesystem::path bitBackupReportCsv;
        std::filesystem::path bitbackupindex;

        explicit BitBackupFiles(const BitBackupArgs& args) {
            namespace fs = std::filesystem;

            workingDir = args.hasArgument("dir") ? fs::path(args.getArgument("dir")) : fs::current_path();
            workingDirAbsolutePath = fs::absolute(workingDir).string();

            bitBackupSQLite3File = workingDir / ".bitbackup.sqlite3";
            bitBackupSQLite3FileSha512 = workingDir / ".bitbackup.sqlite3.sha512";

            bitBackupIgnore = workingDir / ".bitbackupignore";
            bitBackupReportCsv = workingDir / ".bitbackupreport.csv";
            bitbackupindex = workingDir / ".bitbackupindex.csv";

            bitBackupIgnoreRegex = std::make_shared<BitBackupIgnoreRegex>(bitBackupIgnore);
        }
    };
}
#endif // BITBACKUPFILES_H
