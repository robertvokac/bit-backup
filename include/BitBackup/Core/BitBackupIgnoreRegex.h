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

#ifndef BITBACKUPIGNOREREGEX_H
#define BITBACKUPIGNOREREGEX_H

#include <vector>
#include <string>
#include <regex>
#include <filesystem>

namespace BitBackup::Core {
    class BitBackupIgnoreRegex {
    public:
        explicit BitBackupIgnoreRegex(const std::filesystem::path &bitBackupIgnoreFile);

        void addBitBackupIgnoreFile(const std::filesystem::path &bitBackupIgnoreFile);

        void addBitBackupIgnoreFile(const std::filesystem::path &bitBackupIgnoreFile,
                                    const std::filesystem::path &workingDir);

        [[nodiscard]] bool test(const std::string &text) const;

    private:
        std::vector<std::string> patterns;

        static std::string convertUnixRegexToCppRegex(const std::string &wildcard);
    };
}
#endif
