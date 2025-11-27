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
// Created by robertvokac on 4/20/25.
//

#ifndef UNIX_PERMISSIONS_H
#define UNIX_PERMISSIONS_H

#include <string>
#include "UnixPermission.h"

namespace BitBackup::Files {

    class UnixPermissions {
    private:
        UnixPermission user;
        UnixPermission group;
        UnixPermission others;

    public:
        UnixPermissions();
        explicit UnixPermissions(const std::string& s);

        void setSUID(bool b);
        void setSGID(bool b);
        void setStickyBit(bool b);

        bool isSUID() const;
        bool isSGID() const;
        bool isStickyBit() const;

        std::string toString() const;

        [[nodiscard]] UnixPermission& getUser();
        [[nodiscard]] UnixPermission& getGroup();
        [[nodiscard]] UnixPermission& getOthers();
    };

} // namespace BitBackup::Files

#endif // UNIX_PERMISSIONS_H
