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



#include <iostream>
#include <string>
#include <set>
#include <memory>
#include <vector>
#include "BitBackup/Core/Command.h"
#include "BitBackup/Core/BitBackupProgram.h"
#include "BitBackup/Commands/VersionCommand.h"
#include "BitBackup/Commands/HelpCommand.h"
#include "BitBackup/Commands/CheckCommand.h"


namespace BitBackup::Core {
    BitBackupProgram::BitBackupProgram() {
        commandImplementations.push_back(std::make_unique<Commands::CheckCommand>());
        commandImplementations.push_back(std::make_unique<Commands::HelpCommand>());
        commandImplementations.push_back(std::make_unique<Commands::VersionCommand>());
    }

    void BitBackupProgram::run(std::vector<std::string> &args) {
        const BitBackupArgs bba(args);
        run(bba);
    }

    void BitBackupProgram::run(const BitBackupArgs &command) {
        Command *foundCommand = nullptr;
        for (const auto &cmd: commandImplementations) {
            if (cmd->getName() == command.getCommand()) {
                foundCommand = cmd.get();
                break;
            }
        }

        if (!foundCommand) {
            std::cerr << "Command \"" << command.getCommand() << "\" is not supported.\n";
            Commands::HelpCommand helpCmd;
            helpCmd.run(command);
            throw std::runtime_error("Invalid command!");
        }

        foundCommand->run(command);
    }

}
