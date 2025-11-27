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
#include "BitBackup/Core/ListSet.h"

using namespace BitBackup::Core;

TEST(ListSetTests, DoesContainInsertedItems) {
    std::vector<int> numbers = {1, 2, 3, 4};
    ListSet<int> set((std::move(numbers)), [](const int& n) { return std::to_string(n); });

    EXPECT_TRUE(set.doesSetContain("1"));
    EXPECT_TRUE(set.doesSetContain("2"));
    EXPECT_TRUE(set.doesSetContain("3"));
    EXPECT_TRUE(set.doesSetContain("4"));
    EXPECT_FALSE(set.doesSetContain("5"));
}

TEST(ListSetTests, CorrectSizeReported) {
    std::vector<std::string> words = {"apple", "banana", "cherry"};
    ListSet<std::string> set((std::move(words)), [](const std::string& s) { return s; });

    EXPECT_EQ(set.size(), 3);
}

TEST(ListSetTests, GetListAndSetCorrect) {
    std::vector<std::string> fruits = {"apple", "banana"};
    ListSet<std::string> set((std::move(fruits)), [](const std::string& s) { return s; });

    const auto& list = set.getList();
    const auto& internalSet = set.getSet();

    EXPECT_EQ(list.size(), 2);
    EXPECT_TRUE(internalSet.find("apple") != internalSet.end());
    EXPECT_TRUE(internalSet.find("banana") != internalSet.end());
}

TEST(ListSetTests, IteratorFunctionality) {
    std::vector<int> nums = {10, 20};
    ListSet<int> set((std::move(nums)), [](const int& n) { return std::to_string(n); });

    auto it = set.begin();
    EXPECT_EQ(*it, 10);
    ++it;
    EXPECT_EQ(*it, 20);
}
