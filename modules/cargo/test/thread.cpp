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

#include "cargo/thread.h"

#include <gtest/gtest.h>

#include <atomic>

TEST(thread, set_name) {
  std::atomic_bool wait{true};
  cargo::thread thread{[&]() {
    while (wait) {
      ;
    }
  }};
  auto result = thread.set_name("name set");
  if (cargo::unsupported != result) {
    ASSERT_EQ(cargo::success, result);
  }
  wait = false;
  thread.join();
}

TEST(thread, get_name) {
  std::atomic_bool wait{true};
  cargo::thread thread{[&]() {
    while (wait) {
      ;
    }
  }};
  const std::string name{"name set"};
  auto result = thread.set_name(name);
  if (cargo::unsupported != result) {
    ASSERT_EQ(cargo::success, result);
    auto error_or_name = thread.get_name();
    ASSERT_EQ(cargo::success, error_or_name.error());
    ASSERT_STREQ(name.c_str(), error_or_name->c_str());
  }
  wait = false;
  thread.join();
}
