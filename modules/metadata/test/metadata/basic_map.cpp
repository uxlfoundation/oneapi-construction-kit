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
#include <gtest/gtest.h>
#include <metadata/detail/basic_map.h>

#include "fixtures.h"

struct BasicMapTest : MDAllocatorTest {
  std::vector<std::pair<int, int>> data = {
      {1, 3},
      {2, 4},
      {3, 9},
      {4, 1},
  };
};

TEST_F(BasicMapTest, Insert) {
  md::AllocatorHelper<> helper(&hooks, &userdata);
  md::basic_map<int, int> map(helper);

  auto ins_1 = map.insert({1, 4});
  EXPECT_TRUE(ins_1.second);
  auto ins_2 = map.insert({7, 12});
  EXPECT_TRUE(ins_2.second);

  // attempt to insert a key that already exists
  auto ins_fail = map.insert({7, 14});
  EXPECT_FALSE(ins_fail.second);
  // returns the conflicting key/value pair
  EXPECT_EQ(ins_fail.first->first, 7);
  EXPECT_EQ(ins_fail.first->second, 12);
}

TEST_F(BasicMapTest, Erase) {
  md::AllocatorHelper<> helper(&hooks, &userdata);
  md::basic_map<int, int> map(helper);

  for (const auto &el : data) {
    auto ins = map.insert(el);
    EXPECT_TRUE(ins.second);
  }

  for (const auto &el : data) {
    const size_t erased = map.erase(el.first);
    EXPECT_EQ(erased, 1);
  }

  // attempt to erase a non-existant value
  EXPECT_EQ(map.erase(101), 0);
}

TEST_F(BasicMapTest, Find) {
  md::AllocatorHelper<> helper(&hooks, &userdata);
  md::basic_map<int, int> map(helper);

  for (const auto &el : data) {
    auto ins = map.insert(el);
    EXPECT_TRUE(ins.second);
  }

  for (const auto &el : data) {
    auto it = map.find(el.first);
    EXPECT_NE(it, map.end());
  }

  // attempt to find a non-existant value
  auto it_not_found = map.find(101);
  EXPECT_EQ(it_not_found, map.end());
}
