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
//
// Created by robertvokac on 3/11/25.
//

#ifndef MIGRATIONS_H
#define MIGRATIONS_H
#include <string>


namespace BitBackup::Persistence::Impl::Sqlite {
    constexpr int MIGRATION_COUNT = 4;
    inline std::string migrations[MIGRATION_COUNT] = {
        R"(CREATE TABLE "FILE" (
	"ID" TEXT,
	"NAME" TEXT NOT NULL,
        "ABSOLUTE_PATH" TEXT NOT NULL,
        "LAST_MODIFICATION_DATE" TEXT NOT NULL,
        "LAST_CHECK_DATE" TEXT NOT NULL,
        "HASH_SUM_VALUE" TEXT NOT NULL,
        "HASH_SUM_ALGORITHM" TEXT NOT NULL,
        UNIQUE(ABSOLUTE_PATH),
	PRIMARY KEY("ID")
);

CREATE UNIQUE INDEX IDX_FILE_ABSOLUTE_PATH
ON FILE (ABSOLUTE_PATH);

)",

    	R"(CREATE TABLE "SYSTEM_ITEM" (
	"KEY" TEXT,
	"VALUE" TEXT NOT NULL,
	PRIMARY KEY("KEY")
)

)",

        R"(ALTER TABLE "FILE" ADD COLUMN SIZE NUMBER

)",

        R"(ALTER TABLE "FILE" ADD COLUMN LAST_CHECK_RESULT NUMBER

)",
    };
}


#endif //MIGRATIONS_H
