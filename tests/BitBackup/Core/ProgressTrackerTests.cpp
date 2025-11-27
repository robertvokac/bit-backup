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
#include <gtest/gtest.h>
#include "BitBackup/Core/ProgressTracker.h"
#include <thread>
#include <chrono>

using namespace BitBackup::Core;

TEST(ProgressTrackerTest, InitialProgressIsZero) {
    ProgressTracker tracker(5);
    EXPECT_DOUBLE_EQ(tracker.getProgress(), 0.0);
    EXPECT_EQ(tracker.getProgressAsPrettyString(), "0.00%");
    EXPECT_EQ(tracker.getProgressBar(), "[          ]");
}

TEST(ProgressTrackerTest, ProgressAfterNextDone) {
    ProgressTracker tracker(5);
    tracker.nextDone();
    EXPECT_DOUBLE_EQ(tracker.getProgress(), 0.2);
    EXPECT_EQ(tracker.getProgressBar(), "[##        ]");
}

TEST(ProgressTrackerTest, CurrentStatusFormat) {
    ProgressTracker tracker(3);
    tracker.nextDone();
    std::string status = tracker.currentStatus();
    EXPECT_TRUE(status.find("Done 1/3") != std::string::npos);
    EXPECT_TRUE(status.find("%") != std::string::npos);
    EXPECT_TRUE(status.find("[") != std::string::npos);
}

TEST(ProgressTrackerTest, ElapsedTimeIncreases) {
    ProgressTracker tracker(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    long millis = tracker.getElapsedMillisecondsSinceStart();
    EXPECT_GE(millis, 100);
}

TEST(ProgressTrackerTest, RemainingSecondsEstimate) {
    ProgressTracker tracker(2);
    tracker.nextDone(); // 1 of 2 done
    std::this_thread::sleep_for(std::chrono::seconds(1));
    long remaining = tracker.getRemainingSecondsUntilEnd();
    EXPECT_GE(remaining, 1);
}
