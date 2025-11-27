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

#ifndef FILETYPE_H
#define FILETYPE_H

#include <string>
#include <filesystem>

namespace BitBackup::Files {
    /**
     *
     * @author robertvokac
     */
    enum class FileType {
        DIR, REGULAR, LINK, OTHER
    };

    struct FileTypeHelper {
        static constexpr const char *DIR = "d";
        static constexpr const char *REGULAR = "-";
        static constexpr const char *LINK = "l";
        static constexpr const char *OTHER = "z";

        static FileType forFile(const std::filesystem::file_status &status) {
            if (std::filesystem::is_directory(status)) {
                return FileType::DIR;
            }
            if (std::filesystem::is_regular_file(status)) {
                return FileType::REGULAR;
            }
            if (std::filesystem::is_symlink(status)) {
                return FileType::LINK;
            }
            return FileType::OTHER;
        }

        static constexpr const char *toChar(FileType type) {
            switch (type) {
                case FileType::DIR: return DIR;
                case FileType::REGULAR: return REGULAR;
                case FileType::LINK: return LINK;
                case FileType::OTHER: return OTHER;
                default: return "?";
            }
        }
    };
} // namespace BitBackup::Files

#endif // FILETYPE_H
