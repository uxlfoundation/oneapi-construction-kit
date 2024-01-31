// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <cargo/mutex.h>
#include <cargo/string_algorithm.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <sstream>
#include <thread>

TEST(lock_guard, thread_safety) {
  // Compile time test with clang and -Wthread-safety
  cargo::mutex mutex;
  const cargo::lock_guard<cargo::mutex> lock{mutex};
}

TEST(unique_lock, thread_safety) {
  // Compile time test with clang and -Wthread-safety
  cargo::mutex mutex;
  const cargo::unique_lock<cargo::mutex> lock{mutex};
}

TEST(ostream_lock_guard, construct_value) {
  std::stringstream stream;
  std::mutex mutex;
  cargo::ostream_lock_guard<std::stringstream> guard(stream, mutex);
  guard << "construct_default";
  ASSERT_STREQ("construct_default", stream.str().c_str());
}

TEST(ostream_lock_guard, construct_move) {
  std::stringstream stream;
  std::mutex mutex;
  cargo::ostream_lock_guard<std::stringstream> guard(stream, mutex);
  guard << "construct_move";
  ASSERT_STREQ("construct_move", stream.str().c_str());
}

TEST(ostream_lock_guard, assign_move) {
  // These mutex's have to be declared before everything else, as they need to
  // be destructed last
  std::mutex mutex0, mutex1;

  std::stringstream stream0;
  cargo::ostream_lock_guard<std::stringstream> guard0(stream0, mutex0);
  guard0 << "guard0";
  std::stringstream stream1;
  cargo::ostream_lock_guard<std::stringstream> guard1(stream1, mutex1);
  guard1 << "guard1";
  guard0 = std::move(guard1);
  guard0 << "_move";
  ASSERT_STREQ("guard0", stream0.str().c_str());
  ASSERT_STREQ("guard1_move", stream1.str().c_str());
}

struct holder {
  holder(std::ostream &stream) : stream(stream) {}

  cargo::ostream_lock_guard<std::ostream> out() { return {stream, mutex}; }

 private:
  std::ostream &stream;
  std::mutex mutex;
};

TEST(ostream_lock_guard, operator_output_single) {
  std::stringstream stream;
  holder h(stream);
  h.out() << "operator_output_single";
  ASSERT_STREQ("operator_output_single", stream.str().c_str());
}

TEST(ostream_lock_guard, operator_output_multiple) {
  std::stringstream stream;
  holder h(stream);
  {
    auto out = h.out();
    out << "operator";
    out << "_output";
    out << "_multiple";
  }
  ASSERT_STREQ("operator_output_multiple", stream.str().c_str());
}

TEST(ostream_lock_guard, operator_output_threads) {
  std::stringstream stream;
  holder h(stream);
  std::vector<std::thread> threads;
  threads.reserve(4);
  std::array<cargo::string_view, 4> lookup{{"one", "two", "three", "four"}};
  for (auto index = 0; index < 4; index++) {
    threads.push_back(std::thread(
        [&h, &lookup, index]() { h.out() << lookup[index] << "\n"; }));
  }
  for (auto &thread : threads) {
    thread.join();
  }
  auto str = stream.str();
  auto words = cargo::split(str.c_str(), "\n");
  for (auto &word : words) {
    ASSERT_TRUE(std::any_of(
        lookup.begin(), lookup.end(),
        [&word](const cargo::string_view &a) { return a == word; }));
  }
}
