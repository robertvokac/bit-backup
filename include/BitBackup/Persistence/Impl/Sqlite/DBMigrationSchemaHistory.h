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


#ifndef DBMIGRATIONSCHEMAHISTORY_H
#define DBMIGRATIONSCHEMAHISTORY_H
#include <ostream>

namespace BitBackup::Entity {
    struct DBMigrationSchemaHistory {
        /**
 * UUID of migration.
 */
        std::string id;
        /**
         * Migration group.
         */
        std::string migrationGroup = "bitbackup";
        /**
         * Order number.
         */
        int installedRank;
        /**
         * Version. examples: 1, 1.0, 4.3.5
         */
        std::string version;
        /**
         * Description extracted from script name.
         */
        std::string description;
        /**
         * Migration type: SQL or Java
         */
        std::string type = "SQL";
        /**
         * Script name
         */
        std::string script;
        /**
         * Hash.
         */
        std::string hash;
        /**
         * Installation was done by.
         */
        std::string installedBy;
        /**
         * Installation date and time.
         */
        std::string installedOn;
        /**
         * Installation time.
         */
        int executionTime;
        /**
         * Result. If success, then true, otherwise false (failure).
         */
        //in milliseconds
        bool success;

    };
}

#endif //DBMIGRATIONSCHEMAHISTORY_H
