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
// Derived from CC0-licensed `tl::optional` library which can be found
// at https://github.com/TartanLlama/optional.  See optional.LICENSE.txt.

#include <cargo/expected.h>  // Must compile with this included.
#include <cargo/optional.h>
#include <gtest/gtest.h>

#include <string>
#include <tuple>
#include <vector>

TEST(optional, assignment) {
  cargo::optional<int> o1 = 42;
  cargo::optional<int> o2 = 12;
  const cargo::optional<int> o3;

  const cargo::optional<int> &o1cr = o1;
  o1 = o1cr;
  ASSERT_EQ(*o1, 42);

  o1 = o2;
  ASSERT_EQ(*o1, 12);

  o1 = o3;
  ASSERT_TRUE(!o1);

  o1 = 42;
  ASSERT_EQ(*o1, 42);

  o1 = cargo::nullopt;
  ASSERT_TRUE(!o1);

  o1 = std::move(o2);
  ASSERT_EQ(*o1, 12);

  cargo::optional<short> o4 = 42;

  o1 = o4;
  ASSERT_EQ(*o1, 42);

  o1 = std::move(o4);
  ASSERT_EQ(*o1, 42);

  int i = 23;
  cargo::optional<int &> o5;
  o5 = i;

  ASSERT_EQ(*o5, 23);
  *o5 = 42;
  ASSERT_EQ(*o5, 42);
  o5 = cargo::nullopt;
  ASSERT_FALSE(bool(o5));
}

TEST(optional, bases_triviality) {
  ASSERT_TRUE(std::is_trivially_copy_constructible_v<cargo::optional<int>>);
  ASSERT_TRUE(std::is_trivially_copy_assignable_v<cargo::optional<int>>);
  ASSERT_TRUE(std::is_trivially_move_constructible_v<cargo::optional<int>>);
  ASSERT_TRUE(std::is_trivially_move_assignable_v<cargo::optional<int>>);
  ASSERT_TRUE(std::is_trivially_destructible_v<cargo::optional<int>>);

  {
    struct T {
      T(const T &) = default;
      T(T &&) = default;
      T &operator=(const T &) = default;
      T &operator=(T &&) = default;
      ~T() = default;
    };
    ASSERT_TRUE(std::is_trivially_copy_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_trivially_copy_assignable_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_trivially_move_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_trivially_move_assignable_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_trivially_destructible_v<cargo::optional<T>>);
  }

  {
    struct T {
      T(const T &) {}
      T(T &&) {};
      T &operator=(const T &) { return *this; }
      T &operator=(T &&) { return *this; };
      ~T() {}
    };
    ASSERT_TRUE(!std::is_trivially_copy_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(!std::is_trivially_copy_assignable_v<cargo::optional<T>>);
    ASSERT_TRUE(!std::is_trivially_move_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(!std::is_trivially_move_assignable_v<cargo::optional<T>>);
    ASSERT_TRUE(!std::is_trivially_destructible_v<cargo::optional<T>>);
  }
}

TEST(optional, bases_deletion) {
  ASSERT_TRUE(std::is_copy_constructible_v<cargo::optional<int>>);
  ASSERT_TRUE(std::is_copy_assignable_v<cargo::optional<int>>);
  ASSERT_TRUE(std::is_move_constructible_v<cargo::optional<int>>);
  ASSERT_TRUE(std::is_move_assignable_v<cargo::optional<int>>);
  ASSERT_TRUE(std::is_destructible_v<cargo::optional<int>>);

  {
    struct T {
      T(const T &) = default;
      T(T &&) = default;
      T &operator=(const T &) = default;
      T &operator=(T &&) = default;
      ~T() = default;
    };
    ASSERT_TRUE(std::is_copy_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_copy_assignable_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_move_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_move_assignable_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_destructible_v<cargo::optional<T>>);
  }

  {
    struct T {
      T(const T &) = delete;
      T(T &&) = delete;
      T &operator=(const T &) = delete;
      T &operator=(T &&) = delete;
    };
    ASSERT_TRUE(!std::is_copy_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(!std::is_copy_assignable_v<cargo::optional<T>>);
    ASSERT_TRUE(!std::is_move_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(!std::is_move_assignable_v<cargo::optional<T>>);
  }

  {
    struct T {
      T(const T &) = delete;
      T(T &&) = default;
      T &operator=(const T &) = delete;
      T &operator=(T &&) = default;
    };
    ASSERT_TRUE(!std::is_copy_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(!std::is_copy_assignable_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_move_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_move_assignable_v<cargo::optional<T>>);
  }

  {
    struct T {
      T(const T &) = default;
      T(T &&) = delete;
      T &operator=(const T &) = default;
      T &operator=(T &&) = delete;
    };
    ASSERT_TRUE(std::is_copy_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_copy_assignable_v<cargo::optional<T>>);

    // These should both be true, as it should just copy instead of move
    ASSERT_TRUE(std::is_move_constructible_v<cargo::optional<T>>);
    ASSERT_TRUE(std::is_move_assignable_v<cargo::optional<T>>);
  }
}

TEST(optional, constexpr) {
  {
    constexpr cargo::optional<int> o2{};
    constexpr cargo::optional<int> o3 = {};
    constexpr cargo::optional<int> o4 = cargo::nullopt;
    constexpr cargo::optional<int> o5 = {cargo::nullopt};
    constexpr cargo::optional<int> o6(cargo::nullopt);

    ASSERT_TRUE(!o2);
    ASSERT_TRUE(!o3);
    ASSERT_TRUE(!o4);
    ASSERT_TRUE(!o5);
    ASSERT_TRUE(!o6);
  }
  {
    constexpr cargo::optional<int> o1 = 42;
    constexpr cargo::optional<int> o2{42};
    constexpr cargo::optional<int> o3(42);
    constexpr cargo::optional<int> o4 = {42};
    constexpr int i = 42;
    constexpr cargo::optional<int> o5 = std::move(i);
    constexpr cargo::optional<int> o6{std::move(i)};
    constexpr cargo::optional<int> o7(std::move(i));
    constexpr cargo::optional<int> o8 = {std::move(i)};

    ASSERT_TRUE(*o1 == 42);
    ASSERT_TRUE(*o2 == 42);
    ASSERT_TRUE(*o3 == 42);
    ASSERT_TRUE(*o4 == 42);
    ASSERT_TRUE(*o5 == 42);
    ASSERT_TRUE(*o6 == 42);
    ASSERT_TRUE(*o7 == 42);
    ASSERT_TRUE(*o8 == 42);
  }
}

TEST(optional, constructors) {
  const cargo::optional<int> o1;
  ASSERT_TRUE(!o1);

  const cargo::optional<int> o2 = cargo::nullopt;
  ASSERT_TRUE(!o2);

  cargo::optional<int> o3 = 42;
  ASSERT_EQ(*o3, 42);

  cargo::optional<int> o4 = o3;
  ASSERT_EQ(*o4, 42);

  const cargo::optional<int> o5 = o1;
  ASSERT_TRUE(!o5);

  cargo::optional<int> o6 = std::move(o3);
  ASSERT_EQ(*o6, 42);

  cargo::optional<short> o7 = 42;
  ASSERT_EQ(*o7, 42);

  cargo::optional<int> o8 = o7;
  ASSERT_EQ(*o8, 42);

  cargo::optional<int> o9 = std::move(o7);
  ASSERT_EQ(*o9, 42);

  int i = 42;
  cargo::optional<int &> o10 = i;
  ASSERT_EQ(*o10, 42);
}

static constexpr int get_int(int) { return 42; }
static constexpr cargo::optional<int> get_opt_int(int) { return 42; }

TEST(optional, map) {
  // lhs is empty
  cargo::optional<int> o1;
  auto o1r = o1.map([](int i) { return i + 2; });
  ASSERT_TRUE((std::is_same_v<decltype(o1r), cargo::optional<int>>));
  ASSERT_TRUE(!o1r);

  // lhs has value
  cargo::optional<int> o2 = 40;
  auto o2r = o2.map([](int i) { return i + 2; });
  ASSERT_TRUE((std::is_same_v<decltype(o2r), cargo::optional<int>>));
  ASSERT_EQ(o2r.value(), 42);

  struct rval_call_map {
    double operator()(int) && { return 42.0; };
  };

  // ensure that function object is forwarded
  cargo::optional<int> o3 = 42;
  auto o3r = o3.map(rval_call_map{});
  ASSERT_TRUE((std::is_same_v<decltype(o3r), cargo::optional<double>>));
  ASSERT_EQ(o3r.value(), 42);

  // ensure that lhs is forwarded
  cargo::optional<int> o4 = 40;
  auto o4r = std::move(o4).map([](int &&i) { return i + 2; });
  ASSERT_TRUE((std::is_same_v<decltype(o4r), cargo::optional<int>>));
  ASSERT_EQ(o4r.value(), 42);

  // ensure that lhs is const-propagated
  const cargo::optional<int> o5 = 40;
  auto o5r = o5.map([](const int &i) { return i + 2; });
  ASSERT_TRUE((std::is_same_v<decltype(o5r), cargo::optional<int>>));
  ASSERT_EQ(o5r.value(), 42);

  // test void return
  cargo::optional<int> o7 = 40;
  auto f7 = [](const int &) { return; };
  auto o7r = o7.map(f7);
  ASSERT_TRUE(
      (std::is_same_v<decltype(o7r), cargo::optional<cargo::monostate>>));
  ASSERT_TRUE(o7r.has_value());

  // test each overload in turn
  cargo::optional<int> o8 = 42;
  auto o8r = o8.map([](int) { return 42; });
  ASSERT_EQ(*o8r, 42);

  cargo::optional<int> o9 = 42;
  auto o9r = o9.map([](int) { return; });
  ASSERT_TRUE(!!o9r);

  cargo::optional<int> o12 = 42;
  auto o12r = std::move(o12).map([](int) { return 42; });
  ASSERT_EQ(*o12r, 42);

  cargo::optional<int> o13 = 42;
  auto o13r = std::move(o13).map([](int) { return; });
  ASSERT_TRUE(!!o13r);

  const cargo::optional<int> o16 = 42;
  auto o16r = o16.map([](int) { return 42; });
  ASSERT_EQ(*o16r, 42);

  const cargo::optional<int> o17 = 42;
  auto o17r = o17.map([](int) { return; });
  ASSERT_TRUE(!!o17r);

  const cargo::optional<int> o20 = 42;
  auto o20r = std::move(o20).map([](int) { return 42; });
  ASSERT_EQ(*o20r, 42);

  const cargo::optional<int> o21 = 42;
  auto o21r = std::move(o21).map([](int) { return; });
  ASSERT_TRUE(!!o21r);

  cargo::optional<int> o24 = cargo::nullopt;
  auto o24r = o24.map([](int) { return 42; });
  ASSERT_TRUE(!o24r);

  cargo::optional<int> o25 = cargo::nullopt;
  auto o25r = o25.map([](int) { return; });
  ASSERT_TRUE(!o25r);

  cargo::optional<int> o28 = cargo::nullopt;
  auto o28r = std::move(o28).map([](int) { return 42; });
  ASSERT_TRUE(!o28r);

  cargo::optional<int> o29 = cargo::nullopt;
  auto o29r = std::move(o29).map([](int) { return; });
  ASSERT_TRUE(!o29r);

  const cargo::optional<int> o32 = cargo::nullopt;
  auto o32r = o32.map([](int) { return 42; });
  ASSERT_TRUE(!o32r);

  const cargo::optional<int> o33 = cargo::nullopt;
  auto o33r = o33.map([](int) { return; });
  ASSERT_TRUE(!o33r);

  const cargo::optional<int> o36 = cargo::nullopt;
  auto o36r = std::move(o36).map([](int) { return 42; });
  ASSERT_TRUE(!o36r);

  const cargo::optional<int> o37 = cargo::nullopt;
  auto o37r = std::move(o37).map([](int) { return; });
  ASSERT_TRUE(!o37r);

  int i = 23;
  const cargo::optional<int &> o38 = i;
  auto o38r = o38.map([](int &ir) {
    ir = 42;
    return ir;
  });
  ASSERT_EQ(42, i);
  ASSERT_EQ(*o38r, i);
}

TEST(optional, map_constexpr) {
  // test each overload in turn
  constexpr cargo::optional<int> o16 = 42;
  constexpr auto o16r = o16.map(get_int);
  ASSERT_TRUE(*o16r == 42);

  constexpr cargo::optional<int> o20 = 42;
  constexpr auto o20r = std::move(o20).map(get_int);
  ASSERT_TRUE(*o20r == 42);

  constexpr cargo::optional<int> o32 = cargo::nullopt;
  constexpr auto o32r = o32.map(get_int);
  ASSERT_TRUE(!o32r);
  constexpr cargo::optional<int> o36 = cargo::nullopt;
  constexpr auto o36r = std::move(o36).map(get_int);
  ASSERT_TRUE(!o36r);
}

TEST(optional, and_then) {
  // lhs is empty
  cargo::optional<int> o1;
  auto o1r = o1.and_then([](int) { return cargo::optional<float>{42}; });
  ASSERT_TRUE((std::is_same_v<decltype(o1r), cargo::optional<float>>));
  ASSERT_TRUE(!o1r);

  // lhs has value
  cargo::optional<int> o2 = 12;
  auto o2r = o2.and_then([](int) { return cargo::optional<float>{42}; });
  ASSERT_TRUE((std::is_same_v<decltype(o2r), cargo::optional<float>>));
  ASSERT_EQ(o2r.value(), 42.f);

  // lhs is empty, rhs returns empty
  cargo::optional<int> o3;
  auto o3r = o3.and_then([](int) { return cargo::optional<float>{}; });
  ASSERT_TRUE((std::is_same_v<decltype(o3r), cargo::optional<float>>));
  ASSERT_TRUE(!o3r);

  // rhs returns empty
  cargo::optional<int> o4 = 12;
  auto o4r = o4.and_then([](int) { return cargo::optional<float>{}; });
  ASSERT_TRUE((std::is_same_v<decltype(o4r), cargo::optional<float>>));
  ASSERT_TRUE(!o4r);

  struct rval_call_and_then {
    cargo::optional<double> operator()(int) && {
      return cargo::optional<double>(42.0);
    };
  };

  // ensure that function object is forwarded
  cargo::optional<int> o5 = 42;
  auto o5r = o5.and_then(rval_call_and_then{});
  ASSERT_TRUE((std::is_same_v<decltype(o5r), cargo::optional<double>>));
  ASSERT_EQ(o5r.value(), 42);

  // ensure that lhs is forwarded
  cargo::optional<int> o6 = 42;
  auto o6r = std::move(o6).and_then(
      [](int &&i) { return cargo::optional<double>(i); });
  ASSERT_TRUE((std::is_same_v<decltype(o6r), cargo::optional<double>>));
  ASSERT_EQ(o6r.value(), 42);

  // ensure that function object is const-propagated
  const cargo::optional<int> o7 = 42;
  auto o7r =
      o7.and_then([](const int &i) { return cargo::optional<double>(i); });
  ASSERT_TRUE((std::is_same_v<decltype(o7r), cargo::optional<double>>));
  ASSERT_EQ(o7r.value(), 42);

  // test each overload in turn
  cargo::optional<int> o8 = 42;
  auto o8r = o8.and_then([](int) { return cargo::make_optional(42); });
  ASSERT_EQ(*o8r, 42);

  cargo::optional<int> o9 = 42;
  auto o9r =
      std::move(o9).and_then([](int) { return cargo::make_optional(42); });
  ASSERT_EQ(*o9r, 42);

  const cargo::optional<int> o10 = 42;
  auto o10r = o10.and_then([](int) { return cargo::make_optional(42); });
  ASSERT_EQ(*o10r, 42);

  const cargo::optional<int> o11 = 42;
  auto o11r =
      std::move(o11).and_then([](int) { return cargo::make_optional(42); });
  ASSERT_EQ(*o11r, 42);

  cargo::optional<int> o16 = cargo::nullopt;
  auto o16r = o16.and_then([](int) { return cargo::make_optional(42); });
  ASSERT_TRUE(!o16r);

  cargo::optional<int> o17 = cargo::nullopt;
  auto o17r =
      std::move(o17).and_then([](int) { return cargo::make_optional(42); });
  ASSERT_TRUE(!o17r);

  const cargo::optional<int> o18 = cargo::nullopt;
  auto o18r = o18.and_then([](int) { return cargo::make_optional(42); });
  ASSERT_TRUE(!o18r);

  const cargo::optional<int> o19 = cargo::nullopt;
  auto o19r =
      std::move(o19).and_then([](int) { return cargo::make_optional(42); });
  ASSERT_TRUE(!o19r);

  int i = 23;
  cargo::optional<int &> o20 = i;
  auto o20r = o20.and_then([](int &i) -> cargo::optional<int &> {
    i = 42;
    return i;
  });
  ASSERT_EQ(i, 42);
  ASSERT_EQ(i, *o20r);
}

TEST(optional, constexpr_and_then) {
  constexpr cargo::optional<int> o10 = 42;
  constexpr auto o10r = o10.and_then(get_opt_int);
  ASSERT_EQ(*o10r, 42);

  constexpr cargo::optional<int> o11 = 42;
  constexpr auto o11r = std::move(o11).and_then(get_opt_int);
  ASSERT_EQ(*o11r, 42);

  constexpr cargo::optional<int> o18 = cargo::nullopt;
  constexpr auto o18r = o18.and_then(get_opt_int);
  ASSERT_TRUE(!o18r);

  constexpr cargo::optional<int> o19 = cargo::nullopt;
  constexpr auto o19r = std::move(o19).and_then(get_opt_int);
  ASSERT_TRUE(!o19r);
}

TEST(optional, or_else) {
  cargo::optional<int> o1 = 42;
  auto r1 = *(o1.or_else([] { return cargo::make_optional(13); }));
  ASSERT_EQ(r1, 42);

  cargo::optional<int> o2;
  auto r2 = *(o2.or_else([] { return cargo::make_optional(13); }));
  ASSERT_EQ(r2, 13);
}

TEST(optional, disjunction) {
  cargo::optional<int> o1 = 42;
  cargo::optional<int> o2 = 12;
  cargo::optional<int> o3;

  ASSERT_EQ(*o1.disjunction(o2), 42);
  ASSERT_EQ(*o1.disjunction(o3), 42);
  ASSERT_EQ(*o2.disjunction(o1), 12);
  ASSERT_EQ(*o2.disjunction(o3), 12);
  ASSERT_EQ(*o3.disjunction(o1), 42);
  ASSERT_EQ(*o3.disjunction(o2), 12);
}

TEST(optional, conjunction) {
  const cargo::optional<int> o1 = 42;
  ASSERT_EQ(*o1.conjunction(42.0), 42.0);
  ASSERT_EQ(*o1.conjunction(std::string{"hello"}), std::string{"hello"});
  ASSERT_TRUE(!o1.conjunction(cargo::nullopt));

  const cargo::optional<int> o2;
  ASSERT_TRUE(!o2.conjunction(42.0));
  ASSERT_TRUE(!o2.conjunction(std::string{"hello"}));
  ASSERT_TRUE(!o2.conjunction(cargo::nullopt));
}

TEST(optional, error_or) {
  cargo::optional<int> o1 = 21;
  auto r1 = (o1.map_or([](int x) { return x * 2; }, 13));
  ASSERT_EQ(r1, 42);

  cargo::optional<int> o2;
  auto r2 = (o2.map_or([](int x) { return x * 2; }, 13));
  ASSERT_EQ(r2, 13);
}

TEST(optional, map_or_else) {
  cargo::optional<int> o1 = 21;
  auto r1 = (o1.map_or_else([](int x) { return x * 2; }, [] { return 13; }));
  ASSERT_EQ(r1, 42);

  cargo::optional<int> o2;
  auto r2 = (o2.map_or_else([](int x) { return x * 2; }, [] { return 13; }));
  ASSERT_EQ(r2, 13);
}

TEST(optional, take) {
  cargo::optional<int> o1 = 42;
  ASSERT_EQ(*o1.take(), 42);
  ASSERT_TRUE(!o1);

  cargo::optional<int> o2;
  ASSERT_TRUE(!o2.take());
  ASSERT_TRUE(!o2);
}

struct foo {
  void non_const() {}
};

struct takes_init_and_variadic {
  std::vector<int> v;
  std::tuple<int, int> t;
  template <class... Args>
  takes_init_and_variadic(std::initializer_list<int> l, Args &&...args)
      : v(l), t(std::forward<Args>(args)...) {}
};

TEST(optional, in_place) {
  const cargo::optional<int> o1{cargo::in_place};
  const cargo::optional<int> o2(cargo::in_place);
  ASSERT_TRUE(!!o1);
  ASSERT_EQ(o1, 0);
  ASSERT_TRUE(!!o2);
  ASSERT_EQ(o2, 0);

  const cargo::optional<int> o3(cargo::in_place, 42);
  ASSERT_EQ(o3, 42);

  cargo::optional<std::tuple<int, int>> o4(cargo::in_place, 0, 1);
  ASSERT_TRUE(!!o4);
  ASSERT_EQ(std::get<0>(*o4), 0);
  ASSERT_EQ(std::get<1>(*o4), 1);

  cargo::optional<std::vector<int>> o5(cargo::in_place, {0, 1});
  ASSERT_TRUE(!!o5);
  ASSERT_EQ((*o5)[0], 0);
  ASSERT_EQ((*o5)[1], 1);

  cargo::optional<takes_init_and_variadic> o6(cargo::in_place, {0, 1}, 2, 3);
  ASSERT_EQ(o6->v[0], 0);
  ASSERT_EQ(o6->v[1], 1);
  ASSERT_EQ(std::get<0>(o6->t), 2);
  ASSERT_EQ(std::get<1>(o6->t), 3);
}

TEST(optional, make_optional) {
  auto o1 = cargo::make_optional(42);
  auto o2 = cargo::optional<int>(42);

  constexpr bool is_same = std::is_same_v<decltype(o1), cargo::optional<int>>;
  ASSERT_TRUE(is_same);
  ASSERT_EQ(o1, o2);

  auto o3 = cargo::make_optional<std::tuple<int, int, int, int>>(0, 1, 2, 3);
  ASSERT_EQ(std::get<0>(*o3), 0);
  ASSERT_EQ(std::get<1>(*o3), 1);
  ASSERT_EQ(std::get<2>(*o3), 2);
  ASSERT_EQ(std::get<3>(*o3), 3);

  auto o4 = cargo::make_optional<std::vector<int>>({0, 1, 2, 3});
  ASSERT_EQ(o4.value()[0], 0);
  ASSERT_EQ(o4.value()[1], 1);
  ASSERT_EQ(o4.value()[2], 2);
  ASSERT_EQ(o4.value()[3], 3);

  auto o5 = cargo::make_optional<takes_init_and_variadic>({0, 1}, 2, 3);
  ASSERT_EQ(o5->v[0], 0);
  ASSERT_EQ(o5->v[1], 1);
  ASSERT_EQ(std::get<0>(o5->t), 2);
  ASSERT_EQ(std::get<1>(o5->t), 3);
}

TEST(optional, nullopt) {
  const cargo::optional<int> o1 = cargo::nullopt;
  const cargo::optional<int> o2{cargo::nullopt};
  const cargo::optional<int> o3(cargo::nullopt);
  const cargo::optional<int> o4 = {cargo::nullopt};

  ASSERT_TRUE(!o1);
  ASSERT_TRUE(!o2);
  ASSERT_TRUE(!o3);
  ASSERT_TRUE(!o4);

  ASSERT_TRUE(!std::is_default_constructible_v<cargo::nullopt_t>);
}

TEST(optional, emplace) {
  cargo::optional<int> trivial;

  ASSERT_TRUE(!trivial);
  trivial.emplace();
  ASSERT_TRUE(!!trivial);
  ASSERT_EQ(0, *trivial);
  trivial.emplace(8);
  ASSERT_TRUE(!!trivial);
  ASSERT_EQ(8, *trivial);

  cargo::optional<std::vector<int>> nontrivial;

  ASSERT_TRUE(!nontrivial);
  nontrivial.emplace({1, 2, 3});
  ASSERT_TRUE(!!nontrivial);
  ASSERT_EQ(3, nontrivial->size());
  nontrivial.emplace({42});
  ASSERT_TRUE(!!nontrivial);
  ASSERT_EQ(1, nontrivial->size());
}

struct move_detector {
  move_detector() = default;
  move_detector(move_detector &&rhs) { rhs.been_moved = true; }
  bool been_moved = false;
};

TEST(optional, observers) {
  cargo::optional<int> o1 = 42;
  const cargo::optional<int> o2;
  const cargo::optional<int> o3 = 42;

  ASSERT_EQ(*o1, 42);
  ASSERT_EQ(*o1, o1.value());
  ASSERT_EQ(o2.value_or(42), 42);
  ASSERT_EQ(o3.value(), 42);
  auto success = std::is_same_v<decltype(o1.value()), int &>;
  ASSERT_TRUE(success);
  success = std::is_same_v<decltype(o3.value()), const int &>;
  ASSERT_TRUE(success);
  success = std::is_same_v<decltype(std::move(o1).value()), int &&>;
  ASSERT_TRUE(success);

  success = std::is_same_v<decltype(std::move(o3).value()), const int &&>;
  ASSERT_TRUE(success);

  cargo::optional<move_detector> o4{cargo::in_place};
  const move_detector o5 = std::move(o4).value();
  ASSERT_TRUE(o4->been_moved);
  ASSERT_TRUE(!o5.been_moved);
}

TEST(optional, relops) {
  const cargo::optional<int> o1{4};
  const cargo::optional<int> o2{42};
  const cargo::optional<int> o3{};

  ASSERT_TRUE(!(o1 == o2));
  ASSERT_EQ(o1, o1);
  ASSERT_NE(o1, o2);
  ASSERT_TRUE(!(o1 != o1));
  ASSERT_TRUE(o1 < o2);
  ASSERT_TRUE(!(o1 < o1));
  ASSERT_TRUE(!(o1 > o2));
  ASSERT_TRUE(!(o1 > o1));
  ASSERT_TRUE(o1 <= o2);
  ASSERT_TRUE(o1 <= o1);
  ASSERT_TRUE(!(o1 >= o2));
  ASSERT_TRUE(o1 >= o1);

  ASSERT_TRUE(!(o1 == cargo::nullopt));
  ASSERT_TRUE(!(cargo::nullopt == o1));
  ASSERT_TRUE(o1 != cargo::nullopt);
  ASSERT_TRUE(cargo::nullopt != o1);
  ASSERT_TRUE(!(o1 < cargo::nullopt));
  ASSERT_TRUE(cargo::nullopt < o1);
  ASSERT_TRUE(o1 > cargo::nullopt);
  ASSERT_TRUE(!(cargo::nullopt > o1));
  ASSERT_TRUE(!(o1 <= cargo::nullopt));
  ASSERT_TRUE(cargo::nullopt <= o1);
  ASSERT_TRUE(o1 >= cargo::nullopt);
  ASSERT_TRUE(!(cargo::nullopt >= o1));

  ASSERT_EQ(o3, cargo::nullopt);
  ASSERT_EQ(cargo::nullopt, o3);
  ASSERT_TRUE(!(o3 != cargo::nullopt));
  ASSERT_TRUE(!(cargo::nullopt != o3));
  ASSERT_TRUE(!(o3 < cargo::nullopt));
  ASSERT_TRUE(!(cargo::nullopt < o3));
  ASSERT_TRUE(!(o3 > cargo::nullopt));
  ASSERT_TRUE(!(cargo::nullopt > o3));
  ASSERT_TRUE(o3 <= cargo::nullopt);
  ASSERT_TRUE(cargo::nullopt <= o3);
  ASSERT_TRUE(o3 >= cargo::nullopt);
  ASSERT_TRUE(cargo::nullopt >= o3);

  ASSERT_TRUE(!(o1 == 1));
  ASSERT_TRUE(!(1 == o1));
  ASSERT_TRUE(o1 != 1);
  ASSERT_TRUE(1 != o1);
  ASSERT_TRUE(!(o1 < 1));
  ASSERT_TRUE(1 < o1);
  ASSERT_TRUE(o1 > 1);
  ASSERT_TRUE(!(1 > o1));
  ASSERT_TRUE(!(o1 <= 1));
  ASSERT_TRUE(1 <= o1);
  ASSERT_TRUE(o1 >= 1);
  ASSERT_TRUE(!(1 >= o1));

  ASSERT_EQ(o1, 4);
  ASSERT_EQ(4, o1);
  ASSERT_TRUE(!(o1 != 4));
  ASSERT_TRUE(!(4 != o1));
  ASSERT_TRUE(!(o1 < 4));
  ASSERT_TRUE(!(4 < o1));
  ASSERT_TRUE(!(o1 > 4));
  ASSERT_TRUE(!(4 > o1));
  ASSERT_TRUE(o1 <= 4);
  ASSERT_TRUE(4 <= o1);
  ASSERT_TRUE(o1 >= 4);
  ASSERT_TRUE(4 >= o1);

  const cargo::optional<std::string> o4{"hello"};
  const cargo::optional<std::string> o5{"xyz"};

  ASSERT_TRUE(!(o4 == o5));
  ASSERT_EQ(o4, o4);
  ASSERT_TRUE(o4 != o5);
  ASSERT_TRUE(!(o4 != o4));
  ASSERT_TRUE(o4 < o5);
  ASSERT_TRUE(!(o4 < o4));
  ASSERT_TRUE(!(o4 > o5));
  ASSERT_TRUE(!(o4 > o4));
  ASSERT_TRUE(o4 <= o5);
  ASSERT_TRUE(o4 <= o4);
  ASSERT_TRUE(!(o4 >= o5));
  ASSERT_TRUE(o4 >= o4);

  ASSERT_TRUE(!(o4 == cargo::nullopt));
  ASSERT_TRUE(!(cargo::nullopt == o4));
  ASSERT_TRUE(o4 != cargo::nullopt);
  ASSERT_TRUE(cargo::nullopt != o4);
  ASSERT_TRUE(!(o4 < cargo::nullopt));
  ASSERT_TRUE(cargo::nullopt < o4);
  ASSERT_TRUE(o4 > cargo::nullopt);
  ASSERT_TRUE(!(cargo::nullopt > o4));
  ASSERT_TRUE(!(o4 <= cargo::nullopt));
  ASSERT_TRUE(cargo::nullopt <= o4);
  ASSERT_TRUE(o4 >= cargo::nullopt);
  ASSERT_TRUE(!(cargo::nullopt >= o4));

  ASSERT_EQ(o3, cargo::nullopt);
  ASSERT_EQ(cargo::nullopt, o3);
  ASSERT_TRUE(!(o3 != cargo::nullopt));
  ASSERT_TRUE(!(cargo::nullopt != o3));
  ASSERT_TRUE(!(o3 < cargo::nullopt));
  ASSERT_TRUE(!(cargo::nullopt < o3));
  ASSERT_TRUE(!(o3 > cargo::nullopt));
  ASSERT_TRUE(!(cargo::nullopt > o3));
  ASSERT_TRUE(o3 <= cargo::nullopt);
  ASSERT_TRUE(cargo::nullopt <= o3);
  ASSERT_TRUE(o3 >= cargo::nullopt);
  ASSERT_TRUE(cargo::nullopt >= o3);

  ASSERT_TRUE(!(o4 == "a"));
  ASSERT_TRUE(!("a" == o4));
  ASSERT_TRUE(o4 != "a");
  ASSERT_TRUE("a" != o4);
  ASSERT_TRUE(!(o4 < "a"));
  ASSERT_TRUE("a" < o4);
  ASSERT_TRUE(o4 > "a");
  ASSERT_TRUE(!("a" > o4));
  ASSERT_TRUE(!(o4 <= "a"));
  ASSERT_TRUE("a" <= o4);
  ASSERT_TRUE(o4 >= "a");
  ASSERT_TRUE(!("a" >= o4));

  ASSERT_EQ(o4, "hello");
  ASSERT_EQ("hello", o4);
  ASSERT_TRUE(!(o4 != "hello"));
  ASSERT_TRUE(!("hello" != o4));
  ASSERT_TRUE(!(o4 < "hello"));
  ASSERT_TRUE(!("hello" < o4));
  ASSERT_TRUE(!(o4 > "hello"));
  ASSERT_TRUE(!("hello" > o4));
  ASSERT_TRUE(o4 <= "hello");
  ASSERT_TRUE("hello" <= o4);
  ASSERT_TRUE(o4 >= "hello");
  ASSERT_TRUE("hello" >= o4);
}
