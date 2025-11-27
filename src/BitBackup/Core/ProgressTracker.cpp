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
// Created by robertvokac on 4/26/25.
//

#include "BitBackup/Core/ProgressTracker.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

namespace BitBackup::Core {
    std::string ProgressTracker::getProgressBar() {
        std::string progressBar = "[";
        int percentProgress = static_cast<int>(getProgress() * 10);

        for (int i = 1; i <= 10; ++i) {
            progressBar += (i <= percentProgress) ? "#" : " ";
        }

        progressBar += "]";
        return progressBar;
    }

    long ProgressTracker::getElapsedSecondsSinceStart() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    }

    long ProgressTracker::getElapsedMillisecondsSinceStart() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    }

    long ProgressTracker::getRemainingSecondsUntilEnd() {
        if (done == 0) return 0;
        double secondsPerTask = static_cast<double>(getElapsedSecondsSinceStart()) / done;

        long remainsCount = total - done;
        return static_cast<long>(secondsPerTask * remainsCount);
    }

    ProgressTracker::ProgressTracker(int total) : total(total), done(0) {
        start();
    }

    void ProgressTracker::start() {
        startTime = std::chrono::high_resolution_clock::now();
    }

    void ProgressTracker::nextDone() {
        if (done < total) {
            ++done;
        } else {
            std::cerr << "done is greater than total: done=" << done << ", total=" << total << std::endl;
        }

    }

    double ProgressTracker::getProgress() const {
        return static_cast<double>(done) / total;
    }

    std::string ProgressTracker::getProgressAsPrettyString() {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << (getProgress() * 100) << "%";
        return oss.str();
    }

    std::string ProgressTracker::currentStatus() {
        std::ostringstream oss;
        oss << "Done " << done << "/" << total << " " << getProgressAsPrettyString()
                << " " << getProgressBar();
        return oss.str();
    }
    ;
}
