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
// Derived from CC0-licensed `tl::expected` library which can be found
// at https://github.com/TartanLlama/expected.  See expected.LICENSE.txt.

#include <cargo/expected.h>
#include <cargo/optional.h>  // Must compile with this included.
#include <gtest/gtest.h>

TEST(expected, assignment_simple) {
  cargo::expected<int, int> e1 = 42;
  cargo::expected<int, int> e2 = 17;
  const cargo::expected<int, int> e3 = 21;
  cargo::expected<int, int> e4 = cargo::make_unexpected(42);
  const cargo::expected<int, int> e5 = cargo::make_unexpected(17);
  cargo::expected<int, int> e6 = cargo::make_unexpected(21);

  e1 = e2;
  ASSERT_TRUE(bool(e1));
  ASSERT_TRUE(*e1 == 17);
  ASSERT_TRUE(bool(e2));
  ASSERT_TRUE(*e2 == 17);

  e1 = std::move(e2);
  ASSERT_TRUE(bool(e1));
  ASSERT_TRUE(*e1 == 17);

  e1 = 42;
  ASSERT_TRUE(bool(e1));
  ASSERT_TRUE(*e1 == 42);

  auto unex = cargo::make_unexpected(12);
  e1 = unex;
  ASSERT_TRUE(!e1);
  ASSERT_TRUE(e1.error() == 12);

  e1 = cargo::make_unexpected(42);
  ASSERT_TRUE(!e1);
  ASSERT_TRUE(e1.error() == 42);

  e1 = e3;
  ASSERT_TRUE(bool(e1));
  ASSERT_TRUE(*e1 == 21);

  e4 = e5;
  ASSERT_TRUE(!e4);
  ASSERT_TRUE(e4.error() == 17);

  e4 = std::move(e6);
  ASSERT_TRUE(!e4);
  ASSERT_TRUE(e4.error() == 21);

  e4 = e1;
  ASSERT_TRUE(bool(e4));
  ASSERT_TRUE(*e4 == 21);

  const bool expectedRefIsDefaultConstructible =
      std::is_default_constructible_v<cargo::expected<int &, int>>;
  ASSERT_FALSE(expectedRefIsDefaultConstructible);
}

TEST(expected, assignment_move_only) {
  cargo::expected<std::unique_ptr<int>, std::unique_ptr<int>> e1 =
      std::unique_ptr<int>{new int{42}};
  cargo::expected<std::unique_ptr<int>, std::unique_ptr<int>> e2 =
      cargo::make_unexpected(std::unique_ptr<int>{new int{23}});
  cargo::expected<std::unique_ptr<int>, std::unique_ptr<int>> e3;

  e3 = std::move(e1);
  ASSERT_TRUE(bool(e3));
  ASSERT_EQ(**e3, 42);

  e3 = std::move(e2);
  ASSERT_FALSE(bool(e3));
  ASSERT_EQ(*e3.error(), 23);
}

// One constructor is noexcept and the other is not so that different
// assignment operators may be tested.
struct maybe_throw {
  maybe_throw(int v) noexcept : fval(-1.0), ival(v) {}
  maybe_throw(float v) : fval(v), ival(-1) {}

  bool operator==(int v) { return v == ival; }
  bool operator==(float v) { return v == fval; }

 private:
  float fval;
  int ival;
};

// Test non-trivial
TEST(expected, assignment_nontrivial) {
  cargo::expected<int, int> e1 = 27;
  cargo::expected<maybe_throw, int> e2 = maybe_throw(27);
  cargo::expected<maybe_throw, int> e3 = maybe_throw(27.0f);
  cargo::expected<maybe_throw, maybe_throw> e4 =
      cargo::make_unexpected(maybe_throw(27));
  cargo::expected<maybe_throw, maybe_throw> e5 =
      cargo::make_unexpected(maybe_throw(27.0f));

  // Assigning a float when an int is expected hits a non-default move
  // assignment operator.
  e1 = 42.0f;
  ASSERT_TRUE(bool(e1));
  ASSERT_TRUE(*e1 == 42.0f);

  e2 = 42;
  ASSERT_TRUE(bool(e2));
  ASSERT_TRUE(*e2 == 42);

  e3 = 42.0f;
  ASSERT_TRUE(bool(e3));
  ASSERT_TRUE(*e3 == 42.0f);

  e4 = 42;
  ASSERT_TRUE(bool(e4));
  ASSERT_TRUE(*e4 == 42);

  e5 = 42.0f;
  ASSERT_TRUE(bool(e5));
  ASSERT_TRUE(*e5 == 42.0f);
}

TEST(expected, assignment_deletion) {
  struct has_all {
    has_all() = default;
    has_all(const has_all &) = default;
    has_all(has_all &&) noexcept = default;
    has_all &operator=(const has_all &) = default;
  };

  cargo::expected<has_all, has_all> e1 = {};
  const cargo::expected<has_all, has_all> e2 = {};
  e1 = e2;
}

namespace {
struct takes_init_and_variadic {
  std::vector<int> v;
  std::tuple<int, int> t;
  template <class... Args>
  takes_init_and_variadic(std::initializer_list<int> l, Args &&...args)
      : v(l), t(std::forward<Args>(args)...) {}
};
}  // namespace

TEST(expected, constructors) {
  {
    const cargo::expected<int, int> e;
    ASSERT_TRUE(bool(e));
    ASSERT_TRUE(e == 0);
  }

  {
    cargo::expected<int, int> e = cargo::make_unexpected(0);
    ASSERT_TRUE(!e);
    ASSERT_TRUE(e.error() == 0);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 0);
    ASSERT_TRUE(!e);
    ASSERT_TRUE(e.error() == 0);
  }

  {
    const cargo::expected<int, int> e(cargo::in_place, 42);
    ASSERT_TRUE(bool(e));
    ASSERT_TRUE(e == 42);
  }

  {
    cargo::expected<std::vector<int>, int> e(cargo::in_place, {0, 1});
    ASSERT_TRUE(bool(e));
    ASSERT_TRUE((*e)[0] == 0);
    ASSERT_TRUE((*e)[1] == 1);
  }

  {
    cargo::expected<std::tuple<int, int>, int> e(cargo::in_place, 0, 1);
    ASSERT_TRUE(bool(e));
    ASSERT_TRUE(std::get<0>(*e) == 0);
    ASSERT_TRUE(std::get<1>(*e) == 1);
  }

  {
    cargo::expected<takes_init_and_variadic, int> e(cargo::in_place, {0, 1}, 2,
                                                    3);
    ASSERT_TRUE(bool(e));
    ASSERT_TRUE(e->v[0] == 0);
    ASSERT_TRUE(e->v[1] == 1);
    ASSERT_TRUE(std::get<0>(e->t) == 2);
    ASSERT_TRUE(std::get<1>(e->t) == 3);
  }

  {
    using e = cargo::expected<int, int>;
    ASSERT_TRUE(std::is_default_constructible_v<e>);
    ASSERT_TRUE(std::is_copy_constructible_v<e>);
    ASSERT_TRUE(std::is_move_constructible_v<e>);
    ASSERT_TRUE(std::is_copy_assignable_v<e>);
    ASSERT_TRUE(std::is_move_assignable_v<e>);
    ASSERT_TRUE(std::is_trivially_copy_constructible_v<e>);
    ASSERT_TRUE(std::is_trivially_copy_assignable_v<e>);
    ASSERT_TRUE(std::is_trivially_move_constructible_v<e>);
    ASSERT_TRUE(std::is_trivially_move_assignable_v<e>);
  }

  {
    using e = cargo::expected<int, std::string>;
    ASSERT_TRUE(std::is_default_constructible_v<e>);
    ASSERT_TRUE(std::is_copy_constructible_v<e>);
    ASSERT_TRUE(std::is_move_constructible_v<e>);
    ASSERT_TRUE(std::is_copy_assignable_v<e>);
    ASSERT_TRUE(std::is_move_assignable_v<e>);
    ASSERT_TRUE(!std::is_trivially_copy_constructible_v<e>);
    ASSERT_TRUE(!std::is_trivially_copy_assignable_v<e>);
    ASSERT_TRUE(!std::is_trivially_move_constructible_v<e>);
    ASSERT_TRUE(!std::is_trivially_move_assignable_v<e>);
  }

  {
    using e = cargo::expected<std::string, int>;
    ASSERT_TRUE(std::is_default_constructible_v<e>);
    ASSERT_TRUE(std::is_copy_constructible_v<e>);
    ASSERT_TRUE(std::is_move_constructible_v<e>);
    ASSERT_TRUE(std::is_copy_assignable_v<e>);
    ASSERT_TRUE(std::is_move_assignable_v<e>);
    ASSERT_TRUE(!std::is_trivially_copy_constructible_v<e>);
    ASSERT_TRUE(!std::is_trivially_copy_assignable_v<e>);
    ASSERT_TRUE(!std::is_trivially_move_constructible_v<e>);
    ASSERT_TRUE(!std::is_trivially_move_assignable_v<e>);
  }

  {
    using e = cargo::expected<std::string, std::string>;
    ASSERT_TRUE(std::is_default_constructible_v<e>);
    ASSERT_TRUE(std::is_copy_constructible_v<e>);
    ASSERT_TRUE(std::is_move_constructible_v<e>);
    ASSERT_TRUE(std::is_copy_assignable_v<e>);
    ASSERT_TRUE(std::is_move_assignable_v<e>);
    ASSERT_TRUE(!std::is_trivially_copy_constructible_v<e>);
    ASSERT_TRUE(!std::is_trivially_copy_assignable_v<e>);
    ASSERT_TRUE(!std::is_trivially_move_constructible_v<e>);
    ASSERT_TRUE(!std::is_trivially_move_assignable_v<e>);
  }

  {
    const cargo::expected<void, int> e;
    ASSERT_TRUE(bool(e));
  }

  {
    cargo::expected<void, int> e(cargo::unexpect, 42);
    ASSERT_TRUE(!e);
    ASSERT_TRUE(e.error() == 42);
  }

  {
    int i = 42;
    cargo::expected<int &, int> e = i;
    ASSERT_TRUE(bool(e));
    ASSERT_EQ(i, *e);
    ASSERT_EQ(std::addressof(i), std::addressof(*e));
  }

  {
    cargo::expected<int &, int> e = cargo::make_unexpected(23);
    ASSERT_FALSE(bool(e));
    ASSERT_EQ(23, e.error());
  }

  {
    cargo::expected<std::unique_ptr<int>, int> e =
        std::unique_ptr<int>(new int{42});
    ASSERT_TRUE(bool(e));
    ASSERT_EQ(42, *e.value());
  }

  {
    cargo::expected<std::unique_ptr<int>, int> e = cargo::make_unexpected(23);
    ASSERT_FALSE(bool(e));
    ASSERT_EQ(23, e.error());
  }
}

TEST(expected, emplace) {
  {
    cargo::expected<std::unique_ptr<int>, int> e;
    e.emplace(new int{42});
    ASSERT_TRUE(bool(e));
    ASSERT_TRUE(**e == 42);
  }

  {
    cargo::expected<std::vector<int>, int> e;
    e.emplace({0, 1});
    ASSERT_TRUE(bool(e));
    ASSERT_TRUE((*e)[0] == 0);
    ASSERT_TRUE((*e)[1] == 1);
  }

  {
    cargo::expected<std::tuple<int, int>, int> e;
    e.emplace(2, 3);
    ASSERT_TRUE(bool(e));
    ASSERT_TRUE(std::get<0>(*e) == 2);
    ASSERT_TRUE(std::get<1>(*e) == 3);
  }

  {
    cargo::expected<takes_init_and_variadic, int> e = cargo::make_unexpected(0);
    e.emplace({0, 1}, 2, 3);
    ASSERT_TRUE(bool(e));
    ASSERT_TRUE(e->v[0] == 0);
    ASSERT_TRUE(e->v[1] == 1);
    ASSERT_TRUE(std::get<0>(e->t) == 2);
    ASSERT_TRUE(std::get<1>(e->t) == 3);
  }
}

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#define STATIC_ASSERT_TRUE(e)                      \
  constexpr bool TOKENPASTE2(rqure, __LINE__) = e; \
  ASSERT_TRUE(e);

TEST(expected, extensions_map) {
  auto mul2 = [](int a) { return a * 2; };
  auto ret_void = [](int) {};

  {
    cargo::expected<int, int> e = 21;
    auto ret = e.map(mul2);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = e.map(mul2);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = std::move(e).map(mul2);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = std::move(e).map(mul2);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.map(mul2);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.map(mul2);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).map(mul2);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).map(mul2);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = e.map(ret_void);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE((std::is_same_v<decltype(ret), cargo::expected<void, int>>));
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = e.map(ret_void);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE((std::is_same_v<decltype(ret), cargo::expected<void, int>>));
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = std::move(e).map(ret_void);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE((std::is_same_v<decltype(ret), cargo::expected<void, int>>));
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = std::move(e).map(ret_void);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE((std::is_same_v<decltype(ret), cargo::expected<void, int>>));
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.map(ret_void);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE((std::is_same_v<decltype(ret), cargo::expected<void, int>>));
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.map(ret_void);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE((std::is_same_v<decltype(ret), cargo::expected<void, int>>));
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).map(ret_void);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE((std::is_same_v<decltype(ret), cargo::expected<void, int>>));
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).map(ret_void);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE((std::is_same_v<decltype(ret), cargo::expected<void, int>>));
  }

  // mapping functions which return references
  {
    cargo::expected<int, int> e(42);
    auto ret = e.map([](int &i) -> int & { return i; });
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(ret == 42);
  }
}

TEST(expected, extensions_map_error) {
  auto mul2 = [](int a) { return a * 2; };
  auto ret_void = [](int) {};

  {
    cargo::expected<int, int> e = 21;
    auto ret = e.map_error(mul2);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = e.map_error(mul2);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = std::move(e).map_error(mul2);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = std::move(e).map_error(mul2);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.map_error(mul2);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 42);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.map_error(mul2);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 42);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).map_error(mul2);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 42);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).map_error(mul2);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 42);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = e.map_error(ret_void);
    ASSERT_TRUE(bool(ret));
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = e.map_error(ret_void);
    ASSERT_TRUE(bool(ret));
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = std::move(e).map_error(ret_void);
    ASSERT_TRUE(bool(ret));
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = std::move(e).map_error(ret_void);
    ASSERT_TRUE(bool(ret));
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.map_error(ret_void);
    ASSERT_TRUE(!ret);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.map_error(ret_void);
    ASSERT_TRUE(!ret);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).map_error(ret_void);
    ASSERT_TRUE(!ret);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).map_error(ret_void);
    ASSERT_TRUE(!ret);
  }
}

TEST(expected, extensions_and_then) {
  auto succeed = [](int) { return cargo::expected<int, int>(21 * 2); };
  auto fail = [](int) {
    return cargo::expected<int, int>(cargo::unexpect, 17);
  };

  {
    cargo::expected<int, int> e = 21;
    auto ret = e.and_then(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = e.and_then(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = std::move(e).and_then(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = std::move(e).and_then(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = e.and_then(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 17);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = e.and_then(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 17);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = std::move(e).and_then(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 17);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = std::move(e).and_then(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 17);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.and_then(succeed);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.and_then(succeed);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).and_then(succeed);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).and_then(succeed);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.and_then(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.and_then(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).and_then(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).and_then(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }
}

TEST(expected, extensions_or_else) {
  using eptr = std::unique_ptr<int>;
  auto succeed = [](int) { return cargo::expected<int, int>(21 * 2); };
  auto succeedptr = [](eptr) { return cargo::expected<int, eptr>(21 * 2); };
  auto fail = [](int) {
    return cargo::expected<int, int>(cargo::unexpect, 17);
  };
  auto efail = [](eptr e) {
    *e = 17;
    return cargo::expected<int, eptr>(cargo::unexpect, std::move(e));
  };
  auto failvoid = [](int) {};
  auto failvoidptr = [](const eptr &) { /* don't consume */ };
  auto consumeptr = [](eptr) {};
  auto make_u_int = [](int n) { return std::unique_ptr<int>(new int(n)); };

  {
    cargo::expected<int, int> e = 21;
    auto ret = e.or_else(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = e.or_else(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = std::move(e).or_else(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    cargo::expected<int, eptr> e = 21;
    auto ret = std::move(e).or_else(succeedptr);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = std::move(e).or_else(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = e.or_else(fail);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = e.or_else(fail);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    cargo::expected<int, int> e = 21;
    auto ret = std::move(e).or_else(fail);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(ret == 21);
  }

  {
    cargo::expected<int, eptr> e = 21;
    auto ret = std::move(e).or_else(efail);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(ret == 21);
  }

  {
    const cargo::expected<int, int> e = 21;
    auto ret = std::move(e).or_else(fail);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 21);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.or_else(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.or_else(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).or_else(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    cargo::expected<int, eptr> e(cargo::unexpect, make_u_int(21));
    auto ret = std::move(e).or_else(succeedptr);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).or_else(succeed);
    ASSERT_TRUE(bool(ret));
    ASSERT_TRUE(*ret == 42);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.or_else(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 17);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.or_else(failvoid);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.or_else(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 17);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = e.or_else(failvoid);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).or_else(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 17);
  }

  {
    cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).or_else(failvoid);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }

  {
    cargo::expected<int, eptr> e(cargo::unexpect, make_u_int(21));
    auto ret = std::move(e).or_else(failvoidptr);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(*ret.error() == 21);
  }

  {
    cargo::expected<int, eptr> e(cargo::unexpect, make_u_int(21));
    auto ret = std::move(e).or_else(consumeptr);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == nullptr);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).or_else(fail);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 17);
  }

  {
    const cargo::expected<int, int> e(cargo::unexpect, 21);
    auto ret = std::move(e).or_else(failvoid);
    ASSERT_TRUE(!ret);
    ASSERT_TRUE(ret.error() == 21);
  }
}
struct S {
  int x;
};

struct F {
  int x;
};

TEST(expected, map_error_callable_with_expected_type) {
  auto res = cargo::expected<S, F>{cargo::unexpect, F{}};
  res.map_error([](F) {});
}

cargo::expected<int, std::string> getInt3(int val) { return val; }

cargo::expected<int, std::string> getInt2(int val) { return val; }

cargo::expected<int, std::string> getInt1() {
  return getInt2(5).and_then(getInt3);
}

TEST(expected, and_then_std_string) { getInt1(); }

cargo::expected<int, int> operation1() { return 42; }

cargo::expected<std::string, int> operation2(const int) { return "Bananas"; }

TEST(expected, and_then_non_constexpr) {
  const auto intermediate_result = operation1();

  intermediate_result.and_then(operation2);
}

struct a {};
struct b : a {};

TEST(expected, constructors_converting) {
  const cargo::expected<a, int> exp =
      cargo::expected<b, int>(cargo::unexpect, 0);
  ASSERT_TRUE(!exp.has_value());
}

struct move_detector {
  move_detector() = default;
  move_detector(move_detector &&rhs) { rhs.been_moved = true; }
  bool been_moved = false;
};

TEST(expected, observers) {
  cargo::expected<int, int> o1 = 42;
  cargo::expected<int, int> o2{cargo::unexpect, 0};
  const cargo::expected<int, int> o3 = 42;

  ASSERT_TRUE(*o1 == 42);
  ASSERT_TRUE(*o1 == o1.value());
  ASSERT_TRUE(o2.value_or(42) == 42);
  ASSERT_TRUE(o2.error() == 0);
  ASSERT_TRUE(o3.value() == 42);
  auto success = std::is_same_v<decltype(o1.value()), int &>;
  ASSERT_TRUE(success);
  success = std::is_same_v<decltype(o3.value()), const int &>;
  ASSERT_TRUE(success);
  success = std::is_same_v<decltype(std::move(o1).value()), int &&>;
  ASSERT_TRUE(success);

  success = std::is_same_v<decltype(std::move(o3).value()), const int &&>;
  ASSERT_TRUE(success);

  cargo::expected<move_detector, int> o4{cargo::in_place};
  const move_detector o5 = std::move(o4).value();
  ASSERT_TRUE(o4->been_moved);
  ASSERT_TRUE(!o5.been_moved);
}
