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

#ifndef UNIXPERMISSION_H
#define UNIXPERMISSION_H

#include "BitBackup/Files/UnixPermissionType.h"
#include <string>

namespace BitBackup::Files {

/**
 *
 * @author robertvokac
 */

    class UnixPermission {
    public:
        [[nodiscard]] UnixPermissionType unix_permission_type() const {
            return unixPermissionType;
        }

        [[nodiscard]] bool isSpecial() const {
            return special;
        }
        void setSpecial(bool& specialIn) {
            this->special = specialIn;;
        }

    private:
        static constexpr char R = 'r';
        static constexpr char W = 'w';
        static constexpr char X = 'x';
        static constexpr char LOWER_S = 's';
        static constexpr char UPPER_S = 'S';
        static constexpr char LOWER_T = 't';
        static constexpr char UPPER_T = 'T';
        static constexpr char DASH = '-';

        UnixPermissionType unixPermissionType;
        bool read;
        bool write;
        bool execute;
        bool special;

    public:
        explicit UnixPermission(UnixPermissionType type);
        void setFromString(const std::string& s);
        [[nodiscard]] std::string toString() const;
        void setRead(bool& readIn);
        void setWrite(bool& writeIn);
        void setExecute(bool& executeIn);
        [[nodiscard]] bool isRead() const;
        [[nodiscard]] bool isWrite() const;
        [[nodiscard]] bool isExecute() const;
        void setAll(bool& readIn, bool& writeIn, bool& executeIn);
        void setAll(bool& readIn, bool& writeIn, bool& executeIn, bool& specialIn);
        void setAll(UnixPermissionType& type);
        void setAll(UnixPermissionType& type, bool& specialIn);

    };

}
#endif // UNIXPERMISSION_H
