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


/**
 *
*
 */
#ifndef FILETABLE_H
#define FILETABLE_H

namespace BitBackup::Persistence::Impl::Sqlite {

struct FileTable {


    FileTable() = delete;

    FileTable(const FileTable&) = delete;
    FileTable& operator=(const FileTable&) = delete;

    static constexpr const char* TABLE_NAME = "FILE";
    
    static constexpr const char* ID = "ID";
    static constexpr const char* NAME = "NAME";
    static constexpr const char* ABSOLUTE_PATH = "ABSOLUTE_PATH";
    static constexpr const char* LAST_MODIFICATION_DATE = "LAST_MODIFICATION_DATE";
    static constexpr const char* LAST_CHECK_DATE = "LAST_CHECK_DATE";
    //
    static constexpr const char* HASH_SUM_VALUE = "HASH_SUM_VALUE";
    static constexpr const char* HASH_SUM_ALGORITHM = "HASH_SUM_ALGORITHM";
    static constexpr const char* SIZE = "SIZE";
    static constexpr const char* LAST_CHECK_RESULT = "LAST_CHECK_RESULT";
    

};
}
#endif // FILETABLE_H
