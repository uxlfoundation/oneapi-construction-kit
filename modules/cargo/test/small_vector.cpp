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

#include <cargo/small_vector.h>
#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

#include "common.h"

TEST(small_vector, construct_default) {
  const cargo::small_vector<int, 8> v;
  ASSERT_EQ(0u, v.size());
  ASSERT_EQ(8u, v.capacity());
}

TEST(small_vector, construct_move) {
  static int instances = 0;
  struct element : public copyable_t {
    element(int x) : copyable_t(x) { instances++; }
    element(const element &other) : copyable_t(other) { instances++; }
    ~element() { instances--; }
  };

  {
    cargo::small_vector<element, 4> v;
    ASSERT_EQ(cargo::success, v.assign(2, 42));
    ASSERT_EQ(2, v.size());
    const cargo::small_vector<element, 4> m(std::move(v));
    v.clear();
    ASSERT_EQ(0, v.size());
    ASSERT_EQ(2, m.size());
    for (const element &value : m) {
      ASSERT_EQ(42, value.get());
    }
  }
  ASSERT_EQ(0, instances);

  {
    cargo::small_vector<element, 2> w;
    ASSERT_EQ(cargo::success, w.assign({0, 1, 2, 3}));
    ASSERT_EQ(4, w.size());
    cargo::small_vector<element, 2> n(std::move(w));
    w.clear();
    ASSERT_EQ(0, w.size());
    ASSERT_EQ(4, n.size());
    for (int index = 0; index < static_cast<int>(n.size()); index++) {
      ASSERT_EQ(index, n[index].get());
    }
  }
  ASSERT_EQ(0, instances);
}

TEST(small_vector, assign_operator_move_embedded) {
  static int instances = 0;
  struct element : public copyable_t {
    element(int x) : copyable_t(x) { instances++; }
    element(const element &other) : copyable_t(other) { instances++; }
    ~element() { instances--; }
  };

  {
    cargo::small_vector<element, 8> v;
    ASSERT_EQ(cargo::success, v.assign(8, 42));
    ASSERT_EQ(8, v.size());
    cargo::small_vector<element, 8> m;
    m = std::move(v);
    v.clear();
    ASSERT_EQ(0, v.size());
    ASSERT_EQ(8, m.size());
    for (const element &value : m) {
      ASSERT_EQ(42, value.get());
    }
  }

  ASSERT_EQ(0, instances);
}

TEST(small_vector, assign_operator_move_alloced) {
  static int instances = 0;
  struct element : public copyable_t {
    element(int x) : copyable_t(x) { instances++; }
    element(const element &other) : copyable_t(other) { instances++; }
    ~element() { instances--; }
  };

  {
    cargo::small_vector<element, 4> v;
    ASSERT_EQ(cargo::success, v.assign(8, 42));
    ASSERT_EQ(8, v.size());
    cargo::small_vector<element, 4> m;
    m = std::move(v);
    v.clear();
    ASSERT_EQ(0, v.size());
    ASSERT_EQ(8, m.size());
    for (const element &value : m) {
      ASSERT_EQ(42, value.get());
    }
  }

  ASSERT_EQ(0, instances);
}

TEST(small_vector, assign_range) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(1u, v.size());
  ASSERT_EQ(42, v[0]);
  const int size = 8;
  int data[size];
  for (int index = 0; index < size; index++) {
    data[index] = index;
  }
  ASSERT_EQ(cargo::success, v.assign(data, data + size));
  for (int index = 0; index < size; index++) {
    ASSERT_EQ(index, v[index]);
  }
}

TEST(small_vector, assign_size) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(1u, v.size());
  ASSERT_EQ(42, v[0]);
  ASSERT_EQ(cargo::success, v.assign(16, 0));
  for (const int value : v) {
    ASSERT_EQ(0, value);
  }
  ASSERT_EQ(cargo::success, v.assign(32, 23));
  for (const int value : v) {
    ASSERT_EQ(23, value);
  }
}

TEST(small_vector, assign_initalizer_list) {
  cargo::small_vector<int, 2> v;
  ASSERT_TRUE(v.empty());
  ASSERT_EQ(cargo::success, v.assign({0, 1, 2, 3, 4, 5, 6, 7}));
  ASSERT_EQ(8u, v.size());
  for (size_t index = 0; index < v.size(); index++) {
    ASSERT_EQ(index, v[index]);
  }
}

TEST(small_vector, access_at) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(42, *v.at(0));
  v[0] = 23;
  ASSERT_EQ(23, *v.at(0));
  ASSERT_EQ(cargo::out_of_bounds, v.at(1).error());
}

TEST(small_vector, access_at_const) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  const cargo::small_vector<int, 2> &r = v;
  ASSERT_EQ(42, *r.at(0));
  ASSERT_EQ(cargo::out_of_bounds, v.at(1).error());
}

TEST(small_vector, access_operator_subscript) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(42, v[0]);
  v[0] = 23;
  ASSERT_EQ(23, v[0]);
}

TEST(small_vector, access_operator_subscript_const) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  const cargo::small_vector<int, 2> &r = v;
  ASSERT_EQ(42, r[0]);
}

TEST(small_vector, access_front) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(42, v.front());
}

TEST(small_vector, access_front_const) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  const cargo::small_vector<int, 2> &r = v;
  ASSERT_EQ(42, r.front());
}

TEST(small_vector, access_back) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(42, v.front());
  ASSERT_EQ(42, v.back());
  ASSERT_EQ(cargo::success, v.push_back(23));
  ASSERT_EQ(42, v.front());
  ASSERT_EQ(23, v.back());
}

TEST(small_vector, access_back_const) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(42, v.front());
  const cargo::small_vector<int, 2> &r = v;
  ASSERT_EQ(42, r.back());
  ASSERT_EQ(cargo::success, v.push_back(23));
  ASSERT_EQ(42, v.front());
  ASSERT_EQ(23, r.back());
}

TEST(small_vector, access_data) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(42, v.data()[0]);
  ASSERT_EQ(*v.begin(), *v.data());
}

TEST(small_vector, access_data_const) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  const cargo::small_vector<int, 2> &c = v;
  ASSERT_EQ(42, c.data()[0]);
  ASSERT_EQ(*c.begin(), *c.data());
}

TEST(small_vector, access_data_alignment) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.assign({0, 1}));
  ASSERT_EQ(0u, reinterpret_cast<uintptr_t>(v.data()) % alignof(int));
  auto iter = v.insert(v.end(), {2, 3});
  ASSERT_TRUE(bool(iter));
  ASSERT_EQ(0u, reinterpret_cast<uintptr_t>(v.data()) % alignof(int));
}

TEST(small_vector, iterator_begin) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(42, *v.begin());
}

TEST(small_vector, iterator_begin_const) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  cargo::small_vector<int, 2> &r = v;
  ASSERT_EQ(42, *r.begin());
}

TEST(small_vector, iterator_cbegin) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(42, *v.cbegin());
}

TEST(small_vector, iterator_end) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(v.begin() + 1, v.end());
  auto end = v.end();
  ASSERT_EQ(42, *(end - 1));
}

TEST(small_vector, iterator_end_const) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  const cargo::small_vector<int, 2> &r = v;
  ASSERT_EQ(r.begin() + 1, r.end());
  auto end = r.end();
  ASSERT_EQ(42, *(end - 1));
}

TEST(small_vector, iterator_cend) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(v.cbegin() + 1, v.cend());
  auto end = v.cend();
  ASSERT_EQ(42, *(end - 1));
}

TEST(small_vector, iterator_rbegin) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  auto end = v.end();
  ASSERT_EQ(*(end - 1), *v.rbegin());
  ASSERT_EQ(42, *v.rbegin());
}

TEST(small_vector, iterator_rbegin_const) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  const cargo::small_vector<int, 2> &r = v;
  auto end = r.end();
  ASSERT_EQ(*(end - 1), *r.rbegin());
  ASSERT_EQ(42, *r.rbegin());
}

TEST(small_vector, iterator_crbegin) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  auto end = v.cend();
  ASSERT_EQ(*(end - 1), *v.crbegin());
  ASSERT_EQ(42, *v.crbegin());
}

TEST(small_vector, iterator_rend) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  auto rend = v.rend();
  --rend;
  ASSERT_EQ(*v.begin(), *rend);
  ASSERT_EQ(42, *rend);
}

TEST(small_vector, iterator_rend_const) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  const cargo::small_vector<int, 2> &r = v;
  auto rend = r.rend();
  --rend;
  ASSERT_EQ(*r.begin(), *rend);
  ASSERT_EQ(42, *rend);
}

TEST(small_vector, iterator_crend) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  auto rend = v.crend();
  --rend;
  ASSERT_EQ(*v.cbegin(), *rend);
  ASSERT_EQ(42, *rend);
}

TEST(small_vector, capacity_empty) {
  const cargo::small_vector<int, 1> v;
  ASSERT_TRUE(v.empty());
}

TEST(small_vector, capacity_size) {
  cargo::small_vector<int, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(0));
  ASSERT_EQ(cargo::success, v.push_back(1));
  ASSERT_EQ(cargo::success, v.push_back(2));
  ASSERT_EQ(3u, v.size());
  ASSERT_EQ(0, v[0]);
  ASSERT_EQ(1, v[1]);
  ASSERT_EQ(2, v[2]);
}

// TODO: capacity_max_size

TEST(small_vector, capacity_reserve) {
  cargo::small_vector<int, 1> v;
  ASSERT_EQ(cargo::success, v.reserve(4096));
  ASSERT_GE(4096u, v.capacity());
}

TEST(small_vector, capacity_capacity) {
  cargo::small_vector<int, 16> v;
  ASSERT_EQ(16u, v.capacity());
  ASSERT_EQ(cargo::success, v.reserve(256));
  ASSERT_GE(256u, v.capacity());
}

TEST(small_vector, capacity_shrink_to_fit) {
  cargo::small_vector<int, 4> v;
  ASSERT_EQ(cargo::success, v.assign({0, 1, 2, 3, 4, 5}));
  for (int index = 0; index < 4; index++) {
    v.pop_back();
  }
  v.shrink_to_fit();
  for (int index = 0; index < static_cast<int>(v.size()); index++) {
    ASSERT_EQ(index, v[index]);
  }
}

TEST(small_vector, modify_clear) {
  cargo::small_vector<int, 8> v;
  ASSERT_EQ(cargo::success, v.assign({0, 1, 2, 3, 4, 5, 6, 7}));
  ASSERT_EQ(8u, v.size());
  ASSERT_EQ(cargo::success,
            v.assign({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}));
  ASSERT_EQ(16u, v.size());
  v.clear();
  ASSERT_EQ(0u, v.size());
}

TEST(small_vector, modify_insert_single_copy) {
  cargo::small_vector<copyable_t, 2> v;
  const copyable_t c(23);
  ASSERT_EQ(cargo::success, v.push_back(c));
  ASSERT_EQ(1u, v.size());
  auto i = v.insert(v.begin(), copyable_t(42));
  ASSERT_TRUE(bool(i));
  ASSERT_EQ(v.begin(), *i);
  ASSERT_EQ(2u, v.size());
  ASSERT_EQ(42, v[0].get());
  ASSERT_EQ(23, v[1].get());
}

TEST(small_vector, modify_insert_single_move) {
  cargo::small_vector<movable_t, 2> v;
  ASSERT_EQ(cargo::success, v.push_back(movable_t(23)));
  ASSERT_EQ(1u, v.size());
  auto i = v.insert(v.begin(), movable_t(42));
  ASSERT_TRUE(bool(i));
  ASSERT_EQ(v.begin(), *i);
  ASSERT_EQ(2u, v.size());
  ASSERT_EQ(42, v[0].get());
  ASSERT_EQ(23, v[1].get());
}

TEST(small_vector, modify_insert_count) {
  cargo::small_vector<copyable_t, 2> v;
  auto i = v.insert(v.begin(), 4, copyable_t(42));
  ASSERT_TRUE(bool(i));
  ASSERT_EQ(v.begin(), *i);
  ASSERT_EQ(4u, v.size());
  for (const auto &item : v) {
    ASSERT_EQ(42, item.get());
  }
}

TEST(small_vector, modify_insert_range) {
  cargo::small_vector<movable_t, 2> v;
  movable_t ms[4] = {0, 1, 2, 3};
  auto i = v.insert(v.end(), ms, ms + 4);
  ASSERT_TRUE(bool(i));
  ASSERT_EQ(v.begin(), *i);
  ASSERT_EQ(4u, v.size());
  for (int index = 0; index < 4; index++) {
    ASSERT_EQ(index, v[index].get());
  }
}

TEST(small_vector, modify_insert_iterator_list) {
  cargo::small_vector<int, 4> v;
  ASSERT_EQ(cargo::success, v.push_back(42));
  ASSERT_EQ(42, v[0]);
  ASSERT_EQ(1u, v.size());
  auto i = v.insert(v.end(), {0, 1, 2, 3, 4, 5, 6, 7});
  ASSERT_TRUE(bool(i));
  ASSERT_EQ(v.begin() + 1, *i);
  ASSERT_EQ(9u, v.size());
  ASSERT_EQ(42, v[0]);
  for (int index = 0; index < 8; index++) {
    ASSERT_EQ(index, v[index + 1]);
  }
}

TEST(small_vector, modify_emplace) {
  cargo::small_vector<movable_t, 2> v;
  auto i = v.emplace(v.begin(), 42);
  ASSERT_TRUE(bool(i));
  ASSERT_EQ(v.begin(), *i);
  ASSERT_EQ(1u, v.size());
  ASSERT_EQ(42, v[0].get());
}

TEST(small_vector, modify_erase_single) {
  cargo::small_vector<movable_t, 2> v;
  movable_t ms[] = {42, 23};
  ASSERT_EQ(cargo::success, v.assign(ms, ms + 2));
  ASSERT_EQ(2u, v.size());
  auto i = v.erase(v.begin());
  ASSERT_EQ(v.begin(), i);
  ASSERT_EQ(1u, v.size());
  ASSERT_EQ(23, v[0].get());
  i = v.erase(v.begin());
  ASSERT_EQ(v.end(), i);
  ASSERT_EQ(0u, v.size());
}

TEST(small_vector, modify_pointer_erase_single) {
  static unsigned int destroyed = 0;
  struct element : public movable_t {
    element(int x) : movable_t(x) {}
    ~element() { destroyed++; }
  };

  {
    cargo::small_vector<std::unique_ptr<element>, 1> v;

    ASSERT_EQ(cargo::success,
              v.push_back(std::unique_ptr<element>(new element(42))));
    ASSERT_EQ(cargo::success,
              v.push_back(std::unique_ptr<element>(new element(23))));
    ASSERT_EQ(2u, v.size());

    auto i = v.erase(v.begin());
    ASSERT_EQ(v.begin(), i);
    ASSERT_EQ(1u, v.size());
    ASSERT_EQ(23, v[0]->get());

    i = v.erase(v.begin());
    ASSERT_EQ(v.end(), i);
    ASSERT_EQ(0u, v.size());
  }
  ASSERT_EQ(2u, destroyed);
}

TEST(small_vector, modify_erase_range) {
  cargo::small_vector<movable_t, 4> v;
  movable_t ms[] = {0, 1, 2, 3};
  ASSERT_EQ(cargo::success, v.assign(ms, ms + 4));
  ASSERT_EQ(4u, v.size());
  auto i = v.erase(v.begin(), v.end());
  ASSERT_EQ(v.end(), i);
  ASSERT_EQ(v.begin(), v.end());
  ASSERT_EQ(0u, v.size());
}

TEST(small_vector, modify_erase_range_same) {
  // cargo::small_vector<>::erase(first,last) removes the elements in the range
  // [first, last). Erasing an empty range is a no-op, as tested here where
  // first==last both begin the begin() iterator.
  cargo::small_vector<movable_t, 4> v;
  movable_t ms[] = {0, 1, 2, 3};
  ASSERT_EQ(cargo::success, v.assign(ms, ms + 4));
  ASSERT_EQ(4u, v.size());
  auto i = v.erase(v.begin(), v.begin());
  ASSERT_EQ(v.begin(), i);
  ASSERT_EQ(4u, v.size());
  ASSERT_EQ(0, v[0].get());
  ASSERT_EQ(1, v[1].get());
  ASSERT_EQ(2, v[2].get());
  ASSERT_EQ(3, v[3].get());
}

TEST(small_vector, modify_pointer_erase_range) {
  static unsigned int destroyed = 0;
  struct element : public movable_t {
    element(int x) : movable_t(x) {}
    ~element() { destroyed++; }
  };

  {
    cargo::small_vector<std::unique_ptr<element>, 1> v;

    ASSERT_EQ(cargo::success,
              v.push_back(std::unique_ptr<element>(new element(0xA))));
    ASSERT_EQ(cargo::success,
              v.push_back(std::unique_ptr<element>(new element(0xB))));
    ASSERT_EQ(cargo::success,
              v.push_back(std::unique_ptr<element>(new element(0xC))));
    ASSERT_EQ(cargo::success,
              v.push_back(std::unique_ptr<element>(new element(0xD))));
    ASSERT_EQ(4u, v.size());

    auto i = v.erase(v.begin(), v.begin() + 2);
    ASSERT_EQ(v.begin(), i);
    ASSERT_EQ(2u, v.size());
    ASSERT_EQ(0xC, v[0]->get());

    i = v.erase(v.begin(), v.end());
    ASSERT_EQ(v.end(), i);
    ASSERT_EQ(v.begin(), v.end());
    ASSERT_EQ(0u, v.size());
  }
  ASSERT_EQ(4u, destroyed);
}

TEST(small_vector, modify_push_back_copy) {
  cargo::small_vector<copyable_t, 2> v;
  ASSERT_TRUE(v.empty());
  for (int index = 0; index < 256; index++) {
    const copyable_t c(index);
    ASSERT_EQ(cargo::success, v.push_back(c));
    ASSERT_EQ(size_t(index + 1), v.size());
    ASSERT_EQ(index, v.back().get());
  }
}

TEST(small_vector, modify_push_back_move) {
  cargo::small_vector<movable_t, 4> v;
  ASSERT_TRUE(v.empty());
  for (int index = 0; index < 256; index++) {
    movable_t m(index);
    ASSERT_EQ(cargo::success, v.push_back(std::move(m)));
    ASSERT_EQ(size_t(index + 1), v.size());
    ASSERT_EQ(index, v.back().get());
  }
}

TEST(small_vector, modify_emplace_back) {
  cargo::small_vector<movable_t, 4> v;
  ASSERT_EQ(cargo::success, v.emplace_back(42));
  ASSERT_EQ(1u, v.size());
  ASSERT_EQ(42, v.back().get());
}

TEST(small_vector, modify_pop_back) {
  struct destroyed_t {
    destroyed_t(bool &flag) : flag(flag) {}
    destroyed_t(const destroyed_t &other) = default;
    destroyed_t &operator=(const destroyed_t &other) {
      flag = other.flag;
      return *this;
    }
    ~destroyed_t() { flag = true; }
    bool &flag;
  };
  cargo::small_vector<destroyed_t, 4> v;
  bool was_destroyed = false;
  ASSERT_EQ(cargo::success, v.emplace_back(was_destroyed));
  ASSERT_EQ(1u, v.size());
  v.pop_back();
  ASSERT_EQ(0u, v.size());
  ASSERT_TRUE(was_destroyed);
}

TEST(small_vector, modify_resize) {
  cargo::small_vector<int, 1> v;
  ASSERT_EQ(cargo::success, v.push_back(1));
  ASSERT_EQ(1u, v.size());
  ASSERT_EQ(1, v[0]);
  ASSERT_EQ(cargo::success, v.resize(2));
  ASSERT_EQ(2u, v.size());
  ASSERT_EQ(1, v[0]);
  ASSERT_EQ(0, v[1]);
  ASSERT_EQ(cargo::success, v.resize(3, 42));
  ASSERT_EQ(3u, v.size());
  ASSERT_EQ(1, v[0]);
  ASSERT_EQ(0, v[1]);
  ASSERT_EQ(42, v[2]);
}

TEST(small_vector, modify_swap_embedded) {
  cargo::small_vector<int, 4> a;
  ASSERT_EQ(cargo::success, a.assign({0, 1, 2, 3}));
  cargo::small_vector<int, 4> b;
  ASSERT_EQ(cargo::success, b.assign({3, 2, 1, 0}));
  a.swap(b);
  int av = 3;
  int bv = 0;
  for (auto index = 0; index < 4; index++) {
    ASSERT_EQ(av, a[index]);
    ASSERT_EQ(bv, b[index]);
    av--;
    bv++;
  }
}

TEST(small_vector, modify_cargo_swap) {
  cargo::small_vector<int, 4> a;
  ASSERT_EQ(cargo::success, a.assign({0, 1, 2, 3}));
  cargo::small_vector<int, 4> b;
  ASSERT_EQ(cargo::success, b.assign({3, 2, 1, 0}));
  cargo::swap(a, b);
  int av = 3;
  int bv = 0;
  for (auto index = 0; index < 4; index++) {
    ASSERT_EQ(av, a[index]);
    ASSERT_EQ(bv, b[index]);
    av--;
    bv++;
  }
}

TEST(small_vector, modify_swap_external) {
  cargo::small_vector<int, 2> a;
  ASSERT_EQ(cargo::success, a.assign({0, 1, 2, 3}));
  cargo::small_vector<int, 2> b;
  ASSERT_EQ(cargo::success, b.assign({3, 2, 1, 0}));
  a.swap(b);
  int av = 3;
  int bv = 0;
  for (auto index = 0; index < 4; index++) {
    ASSERT_EQ(av, a[index]);
    ASSERT_EQ(bv, b[index]);
    av--;
    bv++;
  }
}

TEST(small_vector, clone) {
  cargo::small_vector<int, 4> v;
  ASSERT_EQ(cargo::success, v.assign({0, 1, 2, 3}));
  auto c = v.clone();
  ASSERT_TRUE(bool(c));
  ASSERT_EQ(cargo::success, c.error());
  ASSERT_EQ(v, *c);
}

TEST(small_vector, string_pushes) {
  cargo::small_vector<std::string, 4> v;
  const std::array<std::string, 8> strs = {
      {"1", "2", "3", "4", "5", "6", "7", "8"}};
  for (const auto &S : strs) {
    ASSERT_EQ(cargo::success, v.push_back(S));
  }
  ASSERT_EQ(strs.size(), v.size());
}

TEST(small_vector, movable_pushes) {
  static bool errored = false;
  struct element : public movable_t {
    element() : movable_t(0), initialized(true) {}
    element(element &&rhs) : movable_t(std::move(rhs)), initialized(true) {}
    ~element() {
      if (!initialized) errored = true;
    }
    bool initialized;
  };

  {
    cargo::small_vector<element, 4> v;
    for (int i = 0; i < 8; i++) {
      ASSERT_EQ(cargo::success, v.push_back(element{}));
      ASSERT_FALSE(errored);
    }
    ASSERT_EQ(8, v.size());
  }
  ASSERT_FALSE(errored);
}

TEST(small_vector, copyable_pushes) {
  static bool errored = false;
  struct element : public copyable_t {
    element() : copyable_t(0), initialized(true) {}
    ~element() {
      if (!initialized) errored = true;
    }
    bool initialized;
  };

  {
    cargo::small_vector<element, 4> v;
    for (int i = 0; i < 8; i++) {
      ASSERT_EQ(cargo::success, v.push_back(element{}));
      ASSERT_FALSE(errored);
    }
    ASSERT_EQ(8, v.size());
  }
  ASSERT_FALSE(errored);
}

TEST(small_vector, as_std_vector) {
  cargo::small_vector<int, 16> sv;
  for (int index = 0; index < 16; index++) {
    ASSERT_EQ(cargo::success, sv.push_back(index));
  }
  auto v(cargo::as<std::vector<int>>(sv));
  ASSERT_EQ(16u, v.size());
  for (int index = 0; index < 16; index++) {
    ASSERT_EQ(index, v[index]);
  }
  // v is not backed by sv
  sv[0] = 13;
  ASSERT_EQ(v[0], 0);
}

TEST(small_vector, as_std_string) {
  cargo::small_vector<char, 16> sv;
  for (int index = 0; index < 16; index++) {
    ASSERT_EQ(cargo::success, sv.push_back(static_cast<char>(index + 65)));
  }
  auto s(cargo::as<std::string>(sv));
  ASSERT_EQ(16u, s.size());
  ASSERT_EQ("ABCDEFGHIJKLMNOP", s);
  // s is not backed by sv
  sv[0] = 13;
  ASSERT_EQ(s[0], 65);
}

// TODO: non_member_operator_equal

// TODO: non_member_operator_not_equal

// TODO: non_member_operator_less_than

// TODO: non_member_operator_less_equal

// TODO: non_member_operator_greater_than

// TODO: non_member_operator_greater_equal
