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


#include "BitBackup/Core/BitBackupArgs.h"
using std::string;

#include "BitBackup/Core/BitBackupCommand.h"

namespace BitBackup::Core {


    const std::string BitBackupArgs::CHECK = "check";
    const std::string BitBackupArgs::VERBOSE = "verbose";
    const std::string BitBackupArgs::TRUE = "true";
    const std::string BitBackupArgs::BITBACKUPINDEX = "bitbackupindex";

    BitBackupArgs::BitBackupArgs(const std::vector<std::string> &args) {
        command = args.empty() ? CHECK : args[0];

        if (args.size() > 1) {
            for (const std::string &arg: args) {
                if (args[0] == (arg)) {
                    continue;
                }
                std::vector<std::string> keyValue = split(arg, '=');
                internalMap[keyValue[0]] = keyValue.size() > 1 ? keyValue[1] : "";
            }
            for (const auto &keyValue: internalMap) {
                std::cout << "Found argument: " << keyValue.first << "(=)" << keyValue.second << std::endl;
            }
        }
    }

    std::vector<std::string> BitBackupArgs::split(const std::string &s, char delim) {
        std::vector<std::string> result;
        std::stringstream ss(s);
        std::string item;

        while (getline(ss, item, delim)) {
            result.push_back(item);
        }

        return result;
    }

    bool BitBackupArgs::hasArgument(const std::string &arg) const {
        return internalMap.find(arg) != internalMap.end();
    }

    void BitBackupArgs::addArgument(const string &arg, const string &value) {
        internalMap[arg] = value;
    }

    std::string BitBackupArgs::getArgument(const std::string &arg) const {
        auto it = internalMap.find(arg);
        return it != internalMap.end() ? it->second : "";
    }


    bool BitBackupArgs::isVerboseLoggingEnabled() const {
        return hasArgument(VERBOSE) && getArgument(VERBOSE) == TRUE;
    }


    bool BitBackupArgs::isBitBackupIndexEnabled() const {
        return hasArgument(BITBACKUPINDEX) && getArgument(BITBACKUPINDEX) == TRUE;
    }
    string BitBackupArgs::getCommand() const {
        return command;
    }
};
