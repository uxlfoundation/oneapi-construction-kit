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
// Derived from CC0-licensed `tl::function_ref` library which can be found
// at https://github.com/TartanLlama/function_ref.  See
// function_ref.LICENSE.txt.

#include <cargo/function_ref.h>
#include <gtest/gtest.h>

namespace {
void f() {}

int getValue() { return 1337; }

struct b {
  bool baz_called = false;
  void baz() { baz_called = true; }
  bool qux_called = false;
  void qux() { qux_called = true; }
};

struct Base {};
struct Derived : Base {};

Derived *getDerived() { return nullptr; }
}  // namespace

TEST(function_ref, constructors) {
  const cargo::function_ref<void(void)> fr1 = [] {};
  const cargo::function_ref<void(void)> fr2 = f;
  const cargo::function_ref<void(b)> fr3 = &b::baz;

  // Silence warnings
  (void)fr1;
  (void)fr2;
  (void)fr3;
}

TEST(function_ref, assignment) {
  {
    cargo::function_ref<void(void)> fr = f;
    fr = [] {};
  }

  {
    cargo::function_ref<void(b)> fr = &b::baz;
    fr = &b::qux;
  }
}

TEST(function_ref, call) {
  {
    const cargo::function_ref<int(void)> fr = getValue;
    EXPECT_EQ(fr(), 1337);
  }

  {
    b o;
    auto x = &b::baz;
    cargo::function_ref<void(b &)> fr = x;
    fr(o);
    EXPECT_TRUE(o.baz_called);
    x = &b::qux;
    fr = x;
    fr(o);
    EXPECT_TRUE(o.qux_called);
  }

  {
    auto x = [] { return 42; };
    const cargo::function_ref<int()> fr = x;
    EXPECT_EQ(fr(), 42);
  }

  {
    int i = 0;
    auto x = [&i] { i = 42; };
    const cargo::function_ref<void()> fr = x;
    fr();
    EXPECT_EQ(i, 42);
  }
}

TEST(function_ref, pass_then_call) {
  auto call_ref = [](cargo::function_ref<int(void)> func) { return func(); };

  {
    const int r = call_ref([] { return 1337; });
    EXPECT_EQ(r, 1337);
  }

  {
    struct Callable {
      int operator()() { return 1337; }
    };
    const int r = call_ref(Callable{});
    EXPECT_EQ(r, 1337);
  }

  {
    const int r = call_ref(getValue);
    EXPECT_EQ(r, 1337);
  }

  {
    const int r = call_ref(&getValue);
    EXPECT_EQ(r, 1337);
  }
}

TEST(function_ref, call_with_complex_type) {
  auto call_ref =
      [](const cargo::function_ref<int(const std::vector<int>)> &func) {
        EXPECT_EQ(func({12}), 144);
      };

  int z = 12;
  auto f = [&](const std::vector<int> i) { return i[0] * z; };
  call_ref(f);
}

TEST(function_ref, call_with_upcast) {
  auto call_base = [](cargo::function_ref<Base *()> get_base) {
    EXPECT_EQ(get_base(), nullptr);
  };
  call_base(getDerived);
}
