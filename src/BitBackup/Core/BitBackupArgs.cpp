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
#include <charconv>
#include <stdexcept>
using std::string;

#include "BitBackup/Core/BitBackupCommand.h"

namespace BitBackup::Core {

    namespace {
        void validateCheckOption(const string& key, const string& value) {
            if (key == "dir") {
                if (!value.empty()) return;
            } else if (key == "report" || key == "verbose" ||
                       key == "bitbackupindex" || key == "quick") {
                if (value == "true" || value == "false") return;
            } else if (key == "confirm") {
                if (value == "delete") return;
            } else if (key == "threads" || key == "scrub") {
                int parsed = 0;
                const auto [end, error] = std::from_chars(
                    value.data(), value.data() + value.size(), parsed);
                const int min = key == "threads" ? 1 : 0;
                const int max = key == "threads" ? 16 : 100;
                if (error == std::errc{} && end == value.data() + value.size() &&
                    parsed >= min && parsed <= max) return;
            } else {
                throw std::invalid_argument("Unknown check option: " + key);
            }
            throw std::invalid_argument("Invalid value for check option " + key + ": " + value);
        }
    }


    const std::string BitBackupArgs::CHECK = "check";
    const std::string BitBackupArgs::VERBOSE = "verbose";
    const std::string BitBackupArgs::TRUE = "true";
    const std::string BitBackupArgs::BITBACKUPINDEX = "bitbackupindex";

    BitBackupArgs::BitBackupArgs(const std::vector<std::string> &args) {
        command = args.empty() ? CHECK : args[0];

        for (std::size_t i = 1; i < args.size(); ++i) {
            if (command != CHECK) {
                throw std::invalid_argument("Command " + command + " does not accept options");
            }
            const string& arg = args[i];
            const auto separator = arg.find('=');
            if (separator == string::npos || separator == 0) {
                throw std::invalid_argument("Expected check option key=value: " + arg);
            }
            const string key = arg.substr(0, separator);
            const string value = arg.substr(separator + 1);
            validateCheckOption(key, value);
            internalMap[key] = value;
        }
        for (const auto &keyValue: internalMap) {
            std::cout << "Found argument: " << keyValue.first << "(=)" << keyValue.second << std::endl;
        }
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
