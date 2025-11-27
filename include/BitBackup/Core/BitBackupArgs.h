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


#ifndef BITBACKUPARGS_H
#define BITBACKUPARGS_H

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
using std::string;

#include "BitBackup/Core/BitBackupCommand.h"

namespace BitBackup::Core {

    class BitBackupArgs {
    private:
        std::string command;
        std::unordered_map<std::string, std::string> internalMap;
        static const std::string CHECK;
        static const std::string VERBOSE;
        static const std::string TRUE;
        static const std::string BITBACKUPINDEX;

    public:
        explicit BitBackupArgs(const std::vector<std::string> &args);

    private:
        std::vector<std::string> split(const std::string &s, char delim);

    public:
        bool hasArgument(const std::string &arg) const;
        void addArgument(const string &arg, const string &value);

        std::string getArgument(const std::string &arg) const;
        bool isVerboseLoggingEnabled() const;
        bool isBitBackupIndexEnabled() const;
        string getCommand() const;

    };
};

#endif