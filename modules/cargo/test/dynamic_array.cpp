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

#include <cargo/dynamic_array.h>
#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

TEST(dynamic_array, construct_default) {
  const cargo::dynamic_array<int> d;
  ASSERT_EQ(0u, d.size());
}

TEST(dynamic_array, construct_move) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(16));
  ASSERT_EQ(16u, d.size());
  for (int index = 0; index < static_cast<int>(d.size()); index++) {
    d[index] = index;
  }
  cargo::dynamic_array<int> m(std::move(d));
  d.clear();
  ASSERT_EQ(0u, d.size());
  ASSERT_EQ(16u, m.size());
  for (int index = 0; index < static_cast<int>(m.size()); index++) {
    ASSERT_EQ(index, m[index]);
  }
}

TEST(dynamic_array, assign_move) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(16));
  ASSERT_EQ(16u, d.size());
  for (int index = 0; index < static_cast<int>(d.size()); index++) {
    d[index] = index;
  }
  cargo::dynamic_array<int> m;
  m = std::move(d);
  d.clear();
  ASSERT_EQ(0u, d.size());
  ASSERT_EQ(16u, m.size());
  for (int index = 0; index < static_cast<int>(m.size()); index++) {
    ASSERT_EQ(index, m[index]);
  }
}

TEST(dynamic_array, zero_sized) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(0));
  ASSERT_EQ(d.begin(), d.end());
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(0, d.size());
}

TEST(dynamic_array, access_at) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(16));
  int val = 42;
  for (auto &item : d) {
    item = val++;
  }
  for (size_t index = 0; index < d.size(); index++) {
    auto v = d.at(index);
    ASSERT_TRUE(bool(v));
    ASSERT_EQ(index + 42, *v);
  }
  auto v = d.at(d.size());
  ASSERT_FALSE(bool(v));
  ASSERT_EQ(cargo::out_of_bounds, v.error());
}

TEST(dynamic_array, access_at_const) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(16));
  int val = 42;
  for (auto &item : d) {
    item = val++;
  }
  const cargo::dynamic_array<int> &r = d;
  for (size_t index = 0; index < r.size(); index++) {
    auto v = r.at(index);
    ASSERT_TRUE(bool(v));
    ASSERT_EQ(index + 42, *v);
  }
  auto v = r.at(r.size());
  ASSERT_FALSE(bool(v));
  ASSERT_EQ(cargo::out_of_bounds, v.error());
}

TEST(dynamic_array, access_front) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  ASSERT_EQ(0u, d.front());
  cargo::dynamic_array<int> &r = d;
  ASSERT_EQ(0u, r.front());
  ASSERT_EQ(r[0], r.front());
}

TEST(dynamic_array, access_front_const) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  const cargo::dynamic_array<int> &r = d;
  ASSERT_EQ(0u, r.front());
  ASSERT_EQ(r[0], r.front());
}

TEST(dynamic_array, access_back) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  ASSERT_EQ(3u, d.back());
  ASSERT_EQ(d[3], d.back());
}

TEST(dynamic_array, access_back_const) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  const cargo::dynamic_array<int> &r = d;
  ASSERT_EQ(3u, r.back());
  ASSERT_EQ(r[3], r.back());
}

TEST(dynamic_array, access_data) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  ASSERT_NE(nullptr, d.data());
}

TEST(dynamic_array, iterator_begin) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  ASSERT_EQ(*d.data(), *d.begin());
}

TEST(dynamic_array, iterator_begin_const) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  const cargo::dynamic_array<int> &r = d;
  ASSERT_EQ(*r.data(), *r.begin());
}

TEST(dynamic_array, iterator_cbegin) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  ASSERT_EQ(*d.data(), *d.cbegin());
}

TEST(dynamic_array, iterator_rbegin) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  ASSERT_EQ(d.back(), *d.rbegin());
}

TEST(dynamic_array, iterator_rbegin_const) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  const cargo::dynamic_array<int> &r = d;
  ASSERT_EQ(r.back(), *r.rbegin());
}

TEST(dynamic_array, iterator_crbegin) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  ASSERT_EQ(d.back(), *d.crbegin());
}

TEST(dynamic_array, iterator_end) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  auto end = d.end();
  ASSERT_EQ(d.back(), *(--end));
}

TEST(dynamic_array, iterator_end_const) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  const cargo::dynamic_array<int> &r = d;
  auto end = r.end();
  ASSERT_EQ(r.back(), *(--end));
}

TEST(dynamic_array, iterator_cend) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  auto cend = d.cend();
  ASSERT_EQ(d.back(), *(--cend));
}

TEST(dynamic_array, iterator_rend) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  auto rend = d.rend();
  ASSERT_EQ(d.front(), *(--rend));
}

TEST(dynamic_array, iterator_rend_const) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  const cargo::dynamic_array<int> &r = d;
  auto rend = r.rend();
  ASSERT_EQ(r.front(), *(--rend));
}

TEST(dynamic_array, iterator_crend) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  for (size_t index = 0; index < d.size(); index++) {
    d[index] = index;
  }
  auto crend = d.crend();
  ASSERT_EQ(d.front(), *(--crend));
}

TEST(dynamic_array, capacity_empty) {
  cargo::dynamic_array<int> d;
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(cargo::success, d.alloc(4));
  ASSERT_FALSE(d.empty());
}

TEST(dynamic_array, capacity_size) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(0u, d.size());
  ASSERT_EQ(cargo::success, d.alloc(16));
  ASSERT_EQ(16u, d.size());
}

TEST(dynamic_array, modify_clear) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(16));
  ASSERT_EQ(16u, d.size());
  ASSERT_NE(nullptr, d.data());
  d.clear();
  ASSERT_EQ(0u, d.size());
}

TEST(dynamic_array, multiple_alloc) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(4));
  ASSERT_EQ(4u, d.size());
  ASSERT_EQ(cargo::success, d.alloc(16));
  ASSERT_EQ(16u, d.size());
}

TEST(dynamic_array, as_std_vector) {
  cargo::dynamic_array<int> d;
  ASSERT_EQ(cargo::success, d.alloc(16));
  ASSERT_EQ(16u, d.size());
  for (int index = 0; index < static_cast<int>(d.size()); index++) {
    d[index] = index;
  }
  auto v(cargo::as<std::vector<int>>(d));
  ASSERT_EQ(16u, v.size());
  for (int index = 0; index < static_cast<int>(v.size()); index++) {
    ASSERT_EQ(index, v[index]);
  }
  // v is not backed by d
  d[0] = 13;
  ASSERT_EQ(v[0], 0);
}

TEST(dynamic_array, as_std_string) {
  cargo::dynamic_array<char> d;
  ASSERT_EQ(cargo::success, d.alloc(16));
  ASSERT_EQ(16u, d.size());
  for (int index = 0; index < static_cast<int>(d.size()); index++) {
    d[index] = static_cast<char>(index + 65);
  }
  auto s(cargo::as<std::string>(d));
  ASSERT_EQ(16u, s.size());
  ASSERT_EQ("ABCDEFGHIJKLMNOP", s);
  // s is not backed by d
  d[0] = 13;
  ASSERT_EQ(s[0], 65);
}
