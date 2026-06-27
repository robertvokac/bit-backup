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


#ifndef FSFILE_H
#define FSFILE_H
#include <ostream>

namespace BitBackup::Entity {
    struct FsFile {
        std::string id;
        std::string name;
        std::string absolutePath;
        std::string lastModificationDate;
        std::string lastCheckDate;
        std::string hashSumValue;
        std::string hashSumAlgorithm;
        unsigned long size;
        std::string lastCheckResult;
        // 1 when this file lives under a .bitbackuplock'd directory (frozen).
        int locked = 0;

        friend std::ostream& operator<<(std::ostream& os, const FsFile& file) {
            os << "FsFile{id: " << file.id
               << ", name: " << file.name
               << ", absolutePath: " << file.absolutePath
               << ", lastModificationDate: " << file.lastModificationDate
               << ", lastCheckDate: " << file.lastCheckDate
               << ", hashSumValue: " << file.hashSumValue
               << ", hashSumAlgorithm: " << file.hashSumAlgorithm
               << ", size: " << file.size
               << ", lastCheckResult: " << file.lastCheckResult
               << ", locked: " << file.locked << "}";
            return os;
        }

        bool operator==(const FsFile &other) const {
            return
                    this->id == other.id &&
                    this->name == other.name &&
                    this->absolutePath == other.absolutePath &&
                    this->lastModificationDate == other.lastModificationDate &&
                    this->lastCheckDate == other.lastCheckDate &&
                    this->hashSumValue == other.hashSumValue &&
                    this->hashSumAlgorithm == other.hashSumAlgorithm &&
                    this->size == other.size &&
                    this->lastCheckResult == other.lastCheckResult &&
                    this->locked == other.locked;
        }
    };


}

#endif