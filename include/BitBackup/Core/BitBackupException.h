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



#ifndef BITBACKUP_EXCEPTION_H
#define BITBACKUP_EXCEPTION_H

#include <stdexcept>
#include <string>


namespace BitBackup::Core {
    /**
     * @brief Exception class for BitBackup tool.
     *
     * This exception is thrown in case of errors during the execution of the BitBackup tool.
     * @author  robertvokac
     * @since 2025-03-11
     */
    class BitBackupException : public std::runtime_error {
    public:
        // Constructor with an error message
        explicit BitBackupException(const std::string& msg)
            : std::runtime_error(msg) {}

        // Constructor with an error message and a nested exception
        BitBackupException(const std::string& msg, const std::exception& e)
            : std::runtime_error(msg + ": " + e.what()) {}

        // Constructor with a nested exception
        explicit BitBackupException(const std::exception& e)
            : std::runtime_error(e.what()) {}
    };

} // namespace BitBackup::Core

#endif // BITBACKUP_EXCEPTION_H