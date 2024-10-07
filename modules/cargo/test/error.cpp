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

#include <cargo/error.h>
#include <gtest/gtest.h>

#include "common.h"

TEST(error_or, construct_error) {
  const cargo::error_or<int> eo(cargo::bad_alloc);
  ASSERT_FALSE(bool(eo));
  ASSERT_EQ(cargo::bad_alloc, eo.error());
}

TEST(error_or, construct_value) {
  cargo::error_or<int> eo(42);
  ASSERT_TRUE(bool(eo));
  ASSERT_EQ(42, *eo);
}

TEST(error_or, construct_copyable) {
  const copyable_t c(42);
  cargo::error_or<copyable_t> eo(c);
  ASSERT_TRUE(bool(eo));
  ASSERT_EQ(42, eo->get());
}

TEST(error_or, construct_copyable_rvalue) {
  const copyable_t c(42);
  cargo::error_or<copyable_t> eo(c);
  ASSERT_TRUE(bool(eo));
  ASSERT_EQ(42, eo->get());
}

TEST(error_or, construct_movable) {
  movable_t m(42);
  cargo::error_or<movable_t> eo(std::move(m));
  ASSERT_TRUE(bool(eo));
  ASSERT_EQ(42, eo->get());
}

TEST(error_or, construct_movable_rvalue) {
  cargo::error_or<movable_t> eo(movable_t(42));
  ASSERT_EQ(true, bool(eo));
  ASSERT_EQ(42, eo->get());
}

TEST(error_or, construct_value_default) {
  struct value_t {
    value_t() : i(42) {}
    int i;
  };
  auto value = []() -> cargo::error_or<value_t> { return {}; }();
  ASSERT_EQ(cargo::success, value.error());
  ASSERT_EQ(42, value->i);
}

TEST(error_or, construct_value_single) {
  struct value_t {
    value_t(int i) : i(i) {}
    int i;
  };
  auto value = []() -> cargo::error_or<value_t> { return 42; }();
  ASSERT_EQ(cargo::success, value.error());
  ASSERT_EQ(42, value->i);
}

TEST(error_or, construct_value_initializer_list) {
  struct value_t {
    value_t(int i, float f) : i(i), f(f) {}
    int i;
    float f;
  };
  auto value = []() -> cargo::error_or<value_t> { return {42, 3.14f}; }();
  ASSERT_EQ(cargo::success, value.error());
  ASSERT_EQ(42, value->i);
  ASSERT_FLOAT_EQ(3.14f, value->f);
}

TEST(error_or, construct_copy_ref) {
  cargo::error_or<copyable_t> eo(42);
  ASSERT_TRUE(bool(eo));
  ASSERT_EQ(42, eo->get());
  const cargo::error_or<copyable_t> &c(eo);
  ASSERT_TRUE(bool(c));
  ASSERT_EQ(42, c->get());
}

TEST(error_or, construct_copy_const_ref) {
  cargo::error_or<copyable_t> eo(42);
  ASSERT_TRUE(bool(eo));
  ASSERT_EQ(42, eo->get());
  const cargo::error_or<copyable_t> &ref = eo;
  const cargo::error_or<copyable_t> &c(ref);
  ASSERT_TRUE(bool(c));
  ASSERT_EQ(42, c->get());
}

TEST(error_or, construct_move) {
  cargo::error_or<int> eo(42);
  ASSERT_TRUE(bool(eo));
  ASSERT_EQ(42, *eo);
  cargo::error_or<int> m(std::move(eo));
  ASSERT_TRUE(bool(m));
  ASSERT_EQ(42, *m);
}

TEST(error_or, assignment_copy) {
  const cargo::error_or<int> src(42);
  cargo::error_or<int> dst;
  dst = src;
  ASSERT_TRUE(bool(src));
  ASSERT_TRUE(bool(dst));
  ASSERT_EQ(42, *src);
  ASSERT_EQ(42, *dst);
}

TEST(error_or, assignment_move) {
  cargo::error_or<int> src(42);
  cargo::error_or<int> dst;
  dst = std::move(src);
  ASSERT_TRUE(bool(dst));
  ASSERT_EQ(42, *dst);
}
