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
//
// Created by robertvokac on 3/11/25.
//

#ifndef UTILS_H
#define UTILS_H
#include <vector>
#include <filesystem>

namespace BitBackup::Core {

    /**
 *
 *
 */
    class Utils {

    private:
        Utils() = delete;

        Utils(const Utils&) = delete;
        Utils& operator=(const Utils&) = delete;
    public:
        static std::vector<std::filesystem::path> listAllFilesInDir(const std::filesystem::path& dir);
        static std::vector<std::filesystem::path> listAllFilesInDir(const std::filesystem::path& dir, std::vector<std::filesystem::path>&);
        static void writeTextToFile(const std::string &text, const std::filesystem::path& file);
        static std::string readTextFromFile(const std::filesystem::path& file);
        static std::string calculateSHA512Hash(const std::filesystem::path& file);
        static std::string calculateSHA256Hash(const std::filesystem::path& file);
        static std::string createJdbcUrl(const std::string& directoryWhereSqliteFileIs);
        static std::string encodeBase64(const std::string& s);
        static std::string decodeBase64(const std::string& s);
        static std::string generateUUIDv4();
    };

}

#endif //UTILS_H
