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

#include "BitBackup/Core/BitBackupIgnoreRegex.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "BitBackup/Core/Utils.h"
#include <string>

namespace BitBackup::Core {

    namespace {
        // Strip a trailing CR (so CRLF-edited ignore files work) and trailing
        // horizontal whitespace, mirroring .gitignore's handling of trailing
        // spaces. Without this, "*.log\r" never matches anything.
        std::string rstripPattern(std::string s) {
            while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) {
                s.pop_back();
            }
            return s;
        }
    }

    BitBackupIgnoreRegex::BitBackupIgnoreRegex(const std::filesystem::path& bitBackupIgnoreFile) {
        // Always ignore bit-backup's own metadata files so they never get
        // tracked as regular content (the .sqlite3 / .sha512 files are skipped
        // separately in the scan).
        addPattern("*.bitbackupreport.csv");
        addPattern("*.bitbackupindex.csv");
        addPattern("*.bitbackupignore");
        addBitBackupIgnoreFile(bitBackupIgnoreFile);
    }

    void BitBackupIgnoreRegex::addBitBackupIgnoreFile(const std::filesystem::path& bitBackupIgnoreFile) {
        addBitBackupIgnoreFile(bitBackupIgnoreFile, std::filesystem::path());
    }

    void BitBackupIgnoreRegex::addBitBackupIgnoreFile(const std::filesystem::path& bitBackupIgnoreFile, const std::filesystem::path& workingDir) {
        std::vector<std::string> lines;
        if (std::filesystem::exists(bitBackupIgnoreFile)) {
            const std::string txt = Utils::readTextFromFile(bitBackupIgnoreFile);

            std::istringstream stream(txt);
            std::string line;

            while (std::getline(stream, line)) {
                lines.push_back(line);
            }
        }

        std::string addPrefix = workingDir.empty() ? "" : bitBackupIgnoreFile.parent_path().string();

        if (!workingDir.empty()) {
            std::string workingDirAbs = std::filesystem::absolute(workingDir).string() + "/";
            if (bitBackupIgnoreFile.parent_path().string().rfind(workingDirAbs, 0) == 0) {
                addPrefix.replace(0, workingDirAbs.length(), "");
            }
        }

        for (const auto& raw : lines) {
            std::string line = rstripPattern(raw);
            if (line.empty() || line[0] == '#') {
                continue;  // Skip comments and empty lines
            }
            // A leading '/' anchors the pattern to the ignore file's directory.
            // The scan tests paths relative to the working dir WITHOUT a leading
            // slash (e.g. "logs/a.log"), so drop it - otherwise the documented
            // "/logs/*" style patterns could never match anything.
            if (line.front() == '/') {
                line.erase(0, 1);
            }
            addPattern(addPrefix + line);
        }
    }

    void BitBackupIgnoreRegex::addPattern(const std::string& unixWildcard) {
        patterns.emplace_back(convertUnixRegexToCppRegex(unixWildcard),
                              std::regex_constants::ECMAScript);
    }

    bool BitBackupIgnoreRegex::test(const std::string& text) const {
        for (const auto& re : patterns) {
            if (std::regex_match(text, re)) {
                if (verbose) {
                    std::cout << "ignoring file: " << text << std::endl;
                }
                return true;  // Match found, ignore the file
            }
        }
        return false;
    }

    std::string BitBackupIgnoreRegex::convertUnixRegexToCppRegex(const std::string& wildcard) {
        std::string result = "^";
        for (char c : wildcard) {
            switch (c) {
                case '*':
                    result += ".*";
                    break;
                case '?':
                    result += ".";
                    break;
                // Escape special regexp characters
                case '(':
                case ')':
                case '[':
                case ']':
                case '$':
                case '^':
                case '.':
                case '{':
                case '}':
                case '|':
                case '+':
                case '\\':
                    result += "\\";
                    result += c;
                    break;
                default:
                    result += c;
                    break;
            }
        }
        result += "$";
        return result;
    }
};
