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

#include <cargo/array_view.h>
#include <cargo/utility.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <type_traits>
#include <vector>

TEST(array_view, construct_default) {
  const cargo::array_view<int> av;
  ASSERT_EQ(0u, av.size());
  ASSERT_TRUE(av.empty());
}

TEST(array_view, construct_count) {
  static const int size = 4;
  int a[size] = {2, 9, 5, 1};
  cargo::array_view<int> av(a, size);
  ASSERT_EQ(a, av.data());
  ASSERT_EQ(size, av.size());
  ASSERT_FALSE(av.empty());
  for (int index = 0; index < size; index++) {
    ASSERT_EQ(a[index], av[index]);
  }
}

TEST(array_view, construct_iterator) {
  std::array<int, 4> a{{2, 9, 5, 1}};
  cargo::array_view<int> av(a.begin(), a.end());
  ASSERT_EQ(a.data(), av.data());
  ASSERT_EQ(a.size(), av.size());
  ASSERT_FALSE(av.empty());
  for (int index = 0; index < static_cast<int>(a.size()); index++) {
    ASSERT_EQ(a[index], av[index]);
  }
}

TEST(array_view, construct_iterator_empty) {
  std::array<int, 0> a;
  const cargo::array_view<int> av(a.begin(), a.end());
  ASSERT_EQ(a.size(), av.size());
  ASSERT_TRUE(av.empty());
}

TEST(array_view, constuct_container) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(v.data(), av.data());
  ASSERT_EQ(v.size(), av.size());
  ASSERT_FALSE(av.empty());
  for (int index = 0; index < static_cast<int>(v.size()); index++) {
    ASSERT_EQ(v[index], av[index]);
  }
}

TEST(array_view, constuct_container_empty) {
  std::vector<int> v;
  const cargo::array_view<int> av(v);
  ASSERT_EQ(v.size(), av.size());
  ASSERT_EQ(true, av.empty());
}

TEST(array_view, constuct_container_const) {
  const std::vector<int> v{2, 9, 5, 1};
  const auto &cv = v;
  cargo::array_view<const int> av(cv);
  ASSERT_EQ(cv.data(), av.data());
  ASSERT_EQ(cv.size(), av.size());
  ASSERT_FALSE(av.empty());
  for (int index = 0; index < static_cast<int>(cv.size()); index++) {
    ASSERT_EQ(cv[index], av[index]);
  }
}

TEST(array_view, access_at) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  for (size_t index = 0; index < v.size(); index++) {
    ASSERT_EQ(v.at(index), *av.at(index));
  }
  ASSERT_EQ(cargo::out_of_bounds, av.at(v.size()).error());
}

TEST(array_view, access_at_const) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  for (size_t index = 0; index < v.size(); index++) {
    ASSERT_EQ(v.at(index), *cav.at(index));
  }
  ASSERT_EQ(cargo::out_of_bounds, cav.at(v.size()).error());
}

TEST(array_view, access_operator_index) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  for (size_t index = 0; index < v.size(); index++) {
    ASSERT_EQ(v.at(index), av[index]);
  }
}

TEST(array_view, access_operator_index_const) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  for (size_t index = 0; index < v.size(); index++) {
    ASSERT_EQ(v.at(index), cav[index]);
  }
}

TEST(array_view, access_front) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(v.front(), av.front());
}

TEST(array_view, access_front_const) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  ASSERT_EQ(v.front(), cav.front());
}

TEST(array_view, access_back) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(v.back(), av.back());
}

TEST(array_view, access_back_const) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  ASSERT_EQ(v.back(), cav.back());
}

TEST(array_view, access_data) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(v.data(), av.data());
}

TEST(array_view, access_data_const) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  ASSERT_EQ(v.data(), cav.data());
}

TEST(array_view, iterator_begin) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(*v.begin(), *av.begin());
  ASSERT_TRUE(std::equal(av.begin(), av.end(), v.begin()));
}

TEST(array_view, iterator_begin_const) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  ASSERT_EQ(*v.begin(), *cav.begin());
  ASSERT_TRUE(std::equal(cav.begin(), cav.end(), v.cbegin()));
}

TEST(array_view, iterator_cbegin) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  ASSERT_EQ(*v.cbegin(), *cav.cbegin());
  ASSERT_TRUE(std::equal(cav.cbegin(), cav.cend(), v.cbegin()));
}

TEST(array_view, iterator_end) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(*(v.end() - 1), *(av.end() - 1));
  ASSERT_TRUE(std::equal(av.begin(), av.end(), v.begin()));
}

TEST(array_view, iterator_end_const) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  ASSERT_EQ(*(v.end() - 1), *(cav.end() - 1));
  ASSERT_TRUE(std::equal(cav.begin(), cav.end(), v.cbegin()));
}

TEST(array_view, iterator_cend) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  ASSERT_EQ(*(v.cend() - 1), *(cav.cend() - 1));
  ASSERT_TRUE(std::equal(cav.cbegin(), cav.cend(), v.cbegin()));
}

TEST(array_view, iterator_rbegin) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(v.back(), *v.rbegin());
  ASSERT_TRUE(std::equal(av.rbegin(), av.rend(), v.rbegin()));
}

TEST(array_view, iterator_rbegin_const) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  ASSERT_EQ(v.back(), *cav.rbegin());
  ASSERT_TRUE(std::equal(cav.rbegin(), cav.rend(), v.crbegin()));
}

TEST(array_view, iterator_crbegin) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  ASSERT_EQ(v.back(), *av.crbegin());
  ASSERT_TRUE(std::equal(av.crbegin(), av.crend(), v.crbegin()));
}

TEST(array_view, iterator_rend) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(v.front(), *(av.rend() - 1));
  ASSERT_TRUE(std::equal(av.rbegin(), av.rend(), v.rbegin()));
}

TEST(array_view, iterator_rend_const) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  const auto &cav = av;
  ASSERT_EQ(v.front(), *(cav.rend() - 1));
  ASSERT_TRUE(std::equal(cav.rbegin(), cav.rend(), v.crbegin()));
}

TEST(array_view, iterator_crend) {
  std::vector<int> v{2, 9, 5, 1};
  const cargo::array_view<int> av(v);
  ASSERT_EQ(v.front(), *(av.crend() - 1));
  ASSERT_TRUE(std::equal(av.crbegin(), av.crend(), v.rbegin()));
}

TEST(array_view, capacity_empty) {
  cargo::array_view<int> av;
  ASSERT_TRUE(av.empty());
  std::vector<int> v{2, 9, 5, 1};
  av = v;
  ASSERT_FALSE(av.empty());
}

TEST(array_view, capacity_size) {
  cargo::array_view<int> av;
  ASSERT_EQ(0u, av.size());
  std::vector<int> v{2, 9, 5, 1};
  av = v;
  ASSERT_EQ(v.size(), av.size());
}

TEST(array_view, modify_fill) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  av.fill(42);
  for (const auto &item : v) {
    ASSERT_EQ(42, item);
  }
}

TEST(array_view, modify_pop_front) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(2, av.front());
  av.pop_front();
  ASSERT_EQ(9, av.front());
}

TEST(array_view, modify_pop_back) {
  std::vector<int> v{2, 9, 5, 1};
  cargo::array_view<int> av(v);
  ASSERT_EQ(1, av.back());
  av.pop_back();
  ASSERT_EQ(5, av.back());
}

TEST(array_view, as_std_vector_same_type) {
  std::vector<int> v{12, 0, 15, 16, 14, 13};
  const cargo::array_view<int> av(v);
  std::vector<int> v2(cargo::as<std::vector<int>>(av));
  ASSERT_EQ(v, v2);
  v[0] = 13;
  ASSERT_EQ(v2[0], 12);
}

TEST(array_view, as_std_vector_convertible_type) {
  std::vector<int> v{12, 0, 15, 16, 14, 13};
  const cargo::array_view<int> av(v);
  auto v2(cargo::as<std::vector<size_t>>(av));
  static_assert(std::is_same_v<decltype(v2)::value_type, size_t>,
                "Deduction failed");
  ASSERT_TRUE(v2[0] == 12 && v2[2] == 15 && v2[5] == 13);
}

TEST(array_view, as_std_string) {
  std::string s{"string"};
  cargo::array_view<const char> av(s.data(), s.size());
  auto s2(cargo::as<std::string>(av));
  ASSERT_EQ(s2, "string");
  // av is backed by s, but s2 is not
  s[3] = 'u';
  ASSERT_EQ(av[3], 'u');
  ASSERT_EQ(s2, "string");
}

TEST(array_view, as_cargo_array_view) {
  std::array<int, 6> a{12, 0, 15, 16, 14, 13};
  cargo::array_view<int> av(a);
  // auto av2(cargo::as<cargo::array_view<int>>(av));
  cargo::array_view<int> av2(av.data(), av.size());
  ASSERT_TRUE(av2[0] == 12 && av2[2] == 15 && av2[5] == 13);
  // av2 is still backed by a
  a[0] = 13;
  ASSERT_EQ(av2[0], 13);
}
