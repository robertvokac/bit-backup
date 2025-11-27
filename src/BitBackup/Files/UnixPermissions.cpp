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

#include "BitBackup/Files/UnixPermissions.h"
#include "BitBackup/Core/BitBackupException.h"

/**
 *
 * @author robertvokac
 */
namespace BitBackup::Files {

    UnixPermissions::UnixPermissions()
            : user(UnixPermissionType::USER), group(UnixPermissionType::GROUP), others(UnixPermissionType::OTHERS) {}

    UnixPermissions::UnixPermissions(const std::string& s)
    : user(UnixPermissionType::USER), group(UnixPermissionType::GROUP), others(UnixPermissionType::OTHERS) {

        if (s.length() != 9) {
            throw Core::BitBackupException("Cannot parse UnixPermissions, because the expected length is 9, but given is " + s.length());
        }
        user.setFromString(s.substr(0, 3));
        group.setFromString(s.substr(3, 3));
        others.setFromString(s.substr(6, 3));
    }

    void UnixPermissions::setSUID(bool b) {
        user.setSpecial(b);
    }

    void UnixPermissions::setSGID(bool b) {
        group.setSpecial(b);
    }

    void UnixPermissions::setStickyBit(bool b) {
        others.setSpecial(b);
    }

    bool UnixPermissions::isSUID() const {
        return user.isSpecial();
    }

    bool UnixPermissions::isSGID() const {
        return group.isSpecial();
    }

    bool UnixPermissions::isStickyBit() const {
        return others.isSpecial();
    }

    std::string UnixPermissions::toString() const {
        return user.toString() + group.toString() + others.toString();
    }

    UnixPermission& UnixPermissions::getUser() {
        return user;
    }
    UnixPermission& UnixPermissions::getGroup() {
        return group;
    }
    UnixPermission& UnixPermissions::getOthers() {
        return others;
    }
}
