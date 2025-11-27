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


#include "BitBackup/Core/Command.h"
#include "BitBackup/Commands/HelpCommand.h"
#include <string>

namespace BitBackup::Commands {
    using std::string;


    HelpCommand::HelpCommand() {
    }

    string HelpCommand::getName() const {
        return NAME;
    }

    string HelpCommand::run(const Core::BitBackupArgs &bitBackupArgs) {
        std::string str = R"(NAME
    bitbackup - " Bit Backup"

    SYNOPSIS
        bitbackup [command] [options]
        If no command is provided, then the default command check is used. This means, if you run "bitbackup", it is the same, as to run "bitbackup check".

    DESCRIPTION
        Detects bit rotten files in the given directory to keep your files forever.

    COMMAND
        check       Generates the static website
                        OPTIONS
                            dir={working directory to be checked for bit rot}
                                Optional. Default=. (current working directory)
                            report=true or false
                                Optional. Default= false (nothing will be reported to file .bitbackupreport.csv).
        help        Display help information
        version     Display version information
)";

        std::cout << str << std::endl;
        return str;
    }
    ;
}

