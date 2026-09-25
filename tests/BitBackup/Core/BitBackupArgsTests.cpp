#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "BitBackup/Core/BitBackupArgs.h"

using BitBackup::Core::BitBackupArgs;

TEST(BitBackupArgsTest, RejectsUnknownAndMalformedCheckOptions) {
    for (const std::string& option : {
             "repotr=true", "quick=treu", "scrub=oops", "scrub=25junk",
             "scrub=101", "threads=abc", "threads=0", "threads=17",
             "report", "dir=", "confirm=oops"}) {
        EXPECT_THROW(BitBackupArgs({"check", option}), std::invalid_argument) << option;
    }
}

TEST(BitBackupArgsTest, AcceptsDocumentedCheckOptions) {
    EXPECT_NO_THROW(BitBackupArgs({"check", "dir=/tmp/a=b", "report=false",
                                   "verbose=true", "bitbackupindex=true",
                                   "threads=16", "quick=false", "scrub=25",
                                   "confirm=delete"}));
    EXPECT_EQ(BitBackupArgs({"check", "dir=/tmp/a=b"}).getArgument("dir"), "/tmp/a=b");
    EXPECT_NO_THROW(BitBackupArgs({}));
}
