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


#include "BitBackup/Files/UnixPermission.h"
#include "BitBackup/Core/BitBackupException.h"

namespace BitBackup::Files {

/**
 *
 * @author robertvokac
 */


    UnixPermission::UnixPermission(UnixPermissionType type) :
    unixPermissionType(type), read(false), write(false), execute(false), special(false) {

    }



    void UnixPermission::setFromString(const std::string& s) {
        if (s.length() != 3) {
            throw new Core::BitBackupException("Cannot parse UnixPermission, because the expected lengthis 9, but given is " + s.length());
        }

        switch(s[0]) {
            case R : read = true; break;
            case DASH : read = false; break;
            default: throw new Core::BitBackupException("Cannot parse read UnixPermission from character: " + std::string(1, s[0]));
        }

        switch(s[1]) {
            case W : write = true; break;
            case DASH : write = false; break;
            default: throw new Core::BitBackupException("Cannot parse write UnixPermission from character: " + std::string(1, s[1]));
        }

        switch(s[2]){
            case X: execute = true; special = false; break;
            case LOWER_S:
            case UPPER_S:
                if (unixPermissionType == UnixPermissionType::OTHERS) throw std::runtime_error("Invalid letter " + std::string(1, s[2]));
                execute = (s[2] == LOWER_S);
                special = true;
                break;
            case LOWER_T:
            case UPPER_T:
                if (unixPermissionType != UnixPermissionType::OTHERS) throw std::runtime_error("Invalid letter " + std::string(1, s[2]));
                execute = (s[2] == LOWER_T);
                special = true;
                break;
            case DASH: execute = false; special = false; break;
            default: throw new Core::BitBackupException("Cannot parse execute UnixPermission from character: " + std::string(1, s[2]));
        }

    }

    std::string UnixPermission::toString() const {
        std::string result;
        result+=(read ? R : DASH);
        result+=(write ? W : DASH);
        if (execute) {
            result += special ? (unixPermissionType == UnixPermissionType::OTHERS ? LOWER_T : LOWER_S) : X;
        } else {
            result += special ? (unixPermissionType == UnixPermissionType::OTHERS ? UPPER_T : UPPER_S) : DASH;
        }

        return result;
    }

    void UnixPermission::setRead(bool& readIn) {
        read = readIn;
    }

    void UnixPermission::setWrite(bool& writeIn) {
        write = writeIn;
    }

    void UnixPermission::setExecute(bool& executeIn) {
        execute = executeIn;
    }

    bool UnixPermission::isRead() const {
        return read;
    }

    bool UnixPermission::isWrite() const {
        return write;
    }

    bool UnixPermission::isExecute() const {
        return execute;
    }

    void UnixPermission::setAll(bool& readIn, bool& writeIn, bool &executeIn) {
        read = readIn;
        write = writeIn;
        execute = executeIn;
    }

    void UnixPermission::setAll(bool& readIn, bool& writeIn, bool& executeIn, bool& specialIn) {
        read = readIn;
        write = writeIn;
        execute = executeIn;
        special = specialIn;
    }

    void UnixPermission::setAll(UnixPermissionType& type) {
        read = true;
        write = true;
        execute = true;
    }

    void UnixPermission::setAll(UnixPermissionType& type, bool& specialIn) {
        read = true;
        write = true;
        execute = true;
        special = specialIn;
    }
    ;

}
