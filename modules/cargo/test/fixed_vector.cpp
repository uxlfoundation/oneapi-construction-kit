// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <cargo/fixed_vector.h>
#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

TEST(fixed_vector, assign_range) {
  cargo::fixed_vector<int, 2> v;
  std::array<int, 2> a = {{42, 23}};
  ASSERT_EQ(cargo::success, v.assign(a.begin(), a.end()));
  std::array<int, 3> b = {{42, 23, 3}};
  ASSERT_EQ(cargo::bad_alloc, v.assign(b.begin(), b.end()));
}

TEST(fixed_vector, assign_size) {
  cargo::fixed_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.assign(2, 42));
  ASSERT_EQ(cargo::bad_alloc, v.assign(3, 42));
}

TEST(fixed_vector, assign_initalizer_list) {
  cargo::fixed_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.assign({42, 23}));
  ASSERT_EQ(cargo::bad_alloc, v.assign({42, 23, 3}));
}

TEST(fixed_vector, insert_range) {
  cargo::fixed_vector<int, 2> v;
  std::array<int, 2> a = {{42, 23}};
  ASSERT_EQ(cargo::success, v.insert(v.end(), a.begin(), a.end()).error());
  ASSERT_EQ(cargo::bad_alloc, v.insert(v.end(), a.begin(), a.end()).error());
}

TEST(fixed_vector, insert_size) {
  cargo::fixed_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.insert(v.end(), 2, 42).error());
  ASSERT_EQ(cargo::bad_alloc, v.insert(v.end(), 2, 42).error());
}

TEST(fixed_vector, insert_iterator_list) {
  cargo::fixed_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.insert(v.end(), {42, 23}).error());
  ASSERT_EQ(cargo::bad_alloc, v.insert(v.end(), {42, 23}).error());
}

TEST(fixed_vector, emplace) {
  cargo::fixed_vector<int, 1> v;
  ASSERT_EQ(cargo::success, v.emplace(v.end(), 42).error());
  ASSERT_EQ(cargo::bad_alloc, v.emplace(v.end(), 42).error());
}

TEST(fixed_vector, push_back) {
  cargo::fixed_vector<int, 1> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(cargo::bad_alloc, v.push_back(32));
}

TEST(fixed_vector, emplace_back) {
  cargo::fixed_vector<int, 1> v;
  ASSERT_EQ(cargo::success, v.emplace_back(42));
  ASSERT_EQ(cargo::bad_alloc, v.emplace_back(32));
}

TEST(fixed_vector, resize) {
  cargo::fixed_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.resize(2));
  ASSERT_EQ(cargo::bad_alloc, v.resize(3));
}

TEST(fixed_vector, as_std_vector) {
  cargo::fixed_vector<int, 16> f;
  for (int index = 0; index < 16; index++) {
    ASSERT_EQ(cargo::success, f.push_back(index));
  }
  auto v(cargo::as<std::vector<int>>(f));
  ASSERT_EQ(16u, v.size());
  for (int index = 0; index < 16; index++) {
    ASSERT_EQ(index, v[index]);
  }
  // v is not backed by f
  f[0] = 13;
  ASSERT_EQ(v[0], 0);
}

TEST(fixed_vector, as_std_string) {
  cargo::fixed_vector<char, 16> f;
  for (int index = 0; index < 16; index++) {
    ASSERT_EQ(cargo::success, f.push_back(static_cast<char>(index + 65)));
  }
  auto s(cargo::as<std::string>(f));
  ASSERT_EQ(16u, s.size());
  ASSERT_EQ("ABCDEFGHIJKLMNOP", s);
  // s is not backed by f
  f[0] = 13;
  ASSERT_EQ(s[0], 65);
}
