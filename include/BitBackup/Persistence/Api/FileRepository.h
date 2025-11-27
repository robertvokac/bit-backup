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

#ifndef FILEREPOSITORY_H
#define FILEREPOSITORY_H


#include "BitBackup/Entity/FsFile.h"
#include <string>
#include <vector>


namespace BitBackup::Persistence::Api {

    /**
     *
    *
     */

    class FileRepository {

    public:
        virtual ~FileRepository() = default;

        virtual void create(const std::vector<Entity::FsFile>& files) = 0;
        virtual std::vector<Entity::FsFile> list() = 0;
        virtual void remove(const Entity::FsFile& file) = 0;
        virtual void updateFile(Entity::FsFile& file) = 0;
        virtual void updateLastCheckDate(std::string& lastCheckDate, std::vector<Entity::FsFile>& files) = 0;

    };

}
#endif // FILEREPOSITORY_H
