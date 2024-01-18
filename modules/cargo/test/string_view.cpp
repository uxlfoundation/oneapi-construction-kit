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
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

TEST(string_view, construct_default) {
  const cargo::string_view sv;
  ASSERT_EQ(0u, sv.size());
}

TEST(string_view, construct_std_string) {
  std::string string("string");
  const cargo::string_view sv(string);
  ASSERT_EQ(string.size(), sv.size());
  ASSERT_STREQ(string.data(), sv.data());
}

TEST(string_view, construct_std_array) {
  const std::array<char, 6> array{{'s', 't', 'r', 'i', 'n', 'g'}};
  const cargo::string_view sv(array);
  ASSERT_EQ(strlen("string"), sv.size());
  ASSERT_TRUE(cargo::string_view("string") == sv);
}

TEST(string_view, construct_std_vector) {
  const std::vector<char> vector({'s', 't', 'r', 'i', 'n', 'g'});
  const cargo::string_view sv(vector);
  ASSERT_EQ(strlen("string"), sv.size());
  ASSERT_TRUE(cargo::string_view("string") == sv);
}

TEST(string_view, construct_array_view) {
  const char *cstring = "string";
  const cargo::array_view<const char> array_view(cstring,
                                                 cstring + strlen(cstring));
  const cargo::string_view sv(array_view);
  ASSERT_EQ(strlen("string"), sv.size());
  ASSERT_TRUE(cargo::string_view("string") == sv);
}

TEST(string_view, construct_small_vector) {
  cargo::small_vector<char, 6> vector;
  ASSERT_EQ(cargo::success, vector.assign({'s', 't', 'r', 'i', 'n', 'g'}));
  const cargo::string_view sv(vector);
  ASSERT_EQ(strlen("string"), sv.size());
  ASSERT_TRUE(cargo::string_view("string") == sv);
}

TEST(string_view, construct_copy) {
  const cargo::string_view sv("string");
  const cargo::string_view c(sv);
  ASSERT_EQ(sv.data(), c.data());
  ASSERT_EQ(sv.size(), c.size());
}

TEST(string_view, construct_string_count) {
  const cargo::string_view sv("string", 3);
  ASSERT_EQ(3, sv.size());
  ASSERT_EQ('s', *sv.data());
}

TEST(string_view, construct_string_null_terminates) {
  const cargo::string_view sv("string");
  ASSERT_EQ(strlen("string"), sv.size());
  ASSERT_STREQ("string", sv.data());
}

TEST(string_view, assign_copy) {
  const cargo::string_view sv("string");
  const cargo::string_view c = sv;
  ASSERT_EQ(sv.data(), c.data());
  ASSERT_EQ(sv.size(), c.size());
}

TEST(string_view, iterator_begin) {
  const char *cstr = "string";
  cargo::string_view sv(cstr);
  ASSERT_EQ('s', *sv.begin());
  ASSERT_EQ('s', *sv.cbegin());
  ASSERT_EQ(cstr, sv.begin());
  ASSERT_EQ(cstr, sv.cbegin());
  sv = cargo::string_view(cstr + 1);
  ASSERT_EQ('t', *sv.begin());
  ASSERT_EQ('t', *sv.cbegin());
  ASSERT_EQ(cstr + 1, sv.begin());
  ASSERT_EQ(cstr + 1, sv.cbegin());
}

TEST(string_view, iterator_end) {
  const char *cstr = "string";
  cargo::string_view sv(cstr);
  ASSERT_EQ(cstr + strlen(cstr), sv.end());
  ASSERT_EQ(cstr + strlen(cstr), sv.cend());
  sv = cargo::string_view(cstr, 4);
  ASSERT_EQ('i', *(sv.end() - 1));
  ASSERT_EQ('i', *(sv.cend() - 1));
}

TEST(string_view, iterator_rbegin) {
  const cargo::string_view sv("string");
  ASSERT_EQ('g', *sv.rbegin());
  ASSERT_EQ('g', *sv.crbegin());
}

TEST(string_view, iterator_rend) {
  const cargo::string_view sv("string");
  ASSERT_EQ('s', *(sv.rend() - 1));
  ASSERT_EQ('s', *(sv.crend() - 1));
}

TEST(string_view, access_operator_subscript) {
  const char *cstr = "string";
  const cargo::string_view sv(cstr);
  for (size_t index = 0; index < strlen(cstr); index++) {
    ASSERT_EQ(cstr[index], sv[index]);
  }
}

TEST(string_view, access_at) {
  const char *cstr = "string";
  const cargo::string_view sv(cstr);
  for (size_t index = 0; index < strlen(cstr); index++) {
    auto value = sv.at(index);
    ASSERT_EQ(cargo::success, value.error());
    ASSERT_EQ(cstr[index], *value);
  }
  ASSERT_EQ(cargo::out_of_bounds, sv.at(10).error());
}

TEST(string_view, access_front_back) {
  cargo::string_view sv("string");
  ASSERT_EQ('s', sv.front());
  ASSERT_EQ('g', sv.back());
  sv = cargo::string_view("string", 3);
  ASSERT_EQ('s', sv.front());
  ASSERT_EQ('r', sv.back());
}

TEST(string_view, capacity_size_length) {
  const cargo::string_view sv("string");
  ASSERT_EQ(strlen("string"), sv.size());
  ASSERT_EQ(strlen("string"), sv.length());
}

TEST(string_view, capacity_empty) {
  cargo::string_view sv;
  ASSERT_TRUE(sv.empty());
  sv = cargo::string_view("string");
  ASSERT_FALSE(sv.empty());
}

TEST(string_view, modify_remove_prefix) {
  cargo::string_view sv("string");
  sv.remove_prefix(3);
  ASSERT_EQ(3, sv.size());
  ASSERT_EQ('i', sv.front());
}

TEST(string_view, modify_remove_suffix) {
  cargo::string_view sv("string");
  sv.remove_suffix(3);
  ASSERT_EQ(3, sv.size());
  ASSERT_EQ('r', sv.back());
}

TEST(string_view, modify_swap) {
  cargo::string_view sv0("string");
  cargo::string_view sv1("other");
  sv0.swap(sv1);
  ASSERT_STREQ("other", sv0.data());
  ASSERT_STREQ("string", sv1.data());
}

TEST(string_view, operation_copy) {
  const cargo::string_view sv("string");
  char c[4];
  sv.copy(c, 4, 2);
  ASSERT_EQ('r', c[0]);
  ASSERT_EQ('i', c[1]);
  ASSERT_EQ('n', c[2]);
  ASSERT_EQ('g', c[3]);
}

TEST(string_view, operation_substr) {
  const cargo::string_view sv("string");
  auto ss1 = sv.substr(2);
  ASSERT_EQ(cargo::success, ss1.error());
  ASSERT_EQ(4u, ss1->size());
  ASSERT_EQ('r', ss1->front());
  ASSERT_EQ('g', ss1->back());
  auto ss2 = sv.substr(2, 2);
  ASSERT_EQ(cargo::success, ss2.error());
  ASSERT_EQ(2u, ss2->size());
  ASSERT_EQ('r', ss2->front());
  ASSERT_EQ('i', ss2->back());
}

TEST(string_view, operation_compare_string_view) {
  const cargo::string_view sv("string");
  ASSERT_EQ(0, sv.compare(cargo::string_view("string")));
  ASSERT_LT(0, sv.compare(cargo::string_view("str")));
  ASSERT_GT(0, sv.compare(cargo::string_view("strings")));
  ASSERT_LT(0, sv.compare(cargo::string_view("algorithm")));
  ASSERT_GT(0, sv.compare(cargo::string_view("view")));
}

TEST(string_view, operation_compare_substr_string_view) {
  const cargo::string_view sv("string");
  ASSERT_EQ(0, sv.compare(2, 3, cargo::string_view("rin")));
  ASSERT_LT(0, sv.compare(2, 3, cargo::string_view("pos")));
  ASSERT_GT(0, sv.compare(2, 3, cargo::string_view("tin")));
}

TEST(string_view, operation_compare_substr_string_view_substr) {
  const cargo::string_view sv("string");
  ASSERT_EQ(0, sv.compare(0, 3, cargo::string_view("string"), 0, 3));
  ASSERT_LT(0, sv.compare(0, 3, cargo::string_view("string"), 2, 3));
  ASSERT_GT(0, sv.compare(0, 3, cargo::string_view("string"), 1, 3));
}

TEST(string_view, operation_compare_null_string) {
  const cargo::string_view sv("string");
  ASSERT_EQ(0, sv.compare("string"));
  ASSERT_LT(0, sv.compare("algorithm"));
  ASSERT_GT(0, sv.compare("view"));
}

TEST(string_view, operation_compare_substr_null_string) {
  const cargo::string_view sv("string");
  ASSERT_EQ(0, sv.compare(2, 3, "rin"));
  ASSERT_LT(0, sv.compare(2, 3, "pos"));
  ASSERT_GT(0, sv.compare(2, 3, "tin"));
}

TEST(string_view, operation_compare_substr_string) {
  const cargo::string_view sv("string");
  ASSERT_EQ(0, sv.compare(2, 3, "rint", 3));
  ASSERT_LT(0, sv.compare(1, 3, "trap", 3));
  ASSERT_GT(0, sv.compare(2, 3, "rite", 4));
}

TEST(string_view, operation_starts_with_string_view) {
  const cargo::string_view sv("string"), empty;
  ASSERT_TRUE(sv.starts_with(cargo::string_view("str")));
  ASSERT_TRUE(sv.starts_with(cargo::string_view("")));
  ASSERT_TRUE(empty.starts_with(cargo::string_view("")));
  ASSERT_FALSE(sv.starts_with(cargo::string_view("stringly")));
  ASSERT_FALSE(sv.starts_with(cargo::string_view("not")));
  ASSERT_FALSE(empty.starts_with(cargo::string_view("stringly")));
}

TEST(string_view, operation_starts_with_std_string) {
  const cargo::string_view sv("string"), empty;
  ASSERT_TRUE(sv.starts_with(std::string("str")));
  ASSERT_TRUE(sv.starts_with(std::string("")));
  ASSERT_TRUE(empty.starts_with(std::string("")));
  ASSERT_FALSE(sv.starts_with(std::string("stringly")));
  ASSERT_FALSE(sv.starts_with(std::string("not")));
  ASSERT_FALSE(empty.starts_with(std::string("stringly")));
}

TEST(string_view, operation_starts_with_char) {
  const cargo::string_view sv("string"), empty;
  ASSERT_TRUE(sv.starts_with('s'));
  ASSERT_FALSE(sv.starts_with('n'));
  ASSERT_FALSE(empty.starts_with('n'));
}

TEST(string_view, operation_starts_with_string_null_terminates) {
  const cargo::string_view sv("string"), empty;
  ASSERT_TRUE(sv.starts_with("str"));
  ASSERT_TRUE(sv.starts_with(""));
  ASSERT_TRUE(empty.starts_with(""));
  ASSERT_FALSE(sv.starts_with("stringly"));
  ASSERT_FALSE(sv.starts_with("not"));
  ASSERT_FALSE(empty.starts_with("stringly"));
}

TEST(string_view, operation_ends_with_string_view) {
  const cargo::string_view sv("string"), empty;
  ASSERT_TRUE(sv.ends_with(cargo::string_view("ing")));
  ASSERT_TRUE(sv.ends_with(cargo::string_view("")));
  ASSERT_TRUE(empty.ends_with(cargo::string_view("")));
  ASSERT_FALSE(sv.ends_with(cargo::string_view("a_string")));
  ASSERT_FALSE(sv.ends_with(cargo::string_view("not")));
  ASSERT_FALSE(empty.ends_with(cargo::string_view("not")));
}

TEST(string_view, operation_ends_with_std_string) {
  const cargo::string_view sv("string"), empty;
  ASSERT_TRUE(sv.ends_with(std::string("ing")));
  ASSERT_TRUE(sv.ends_with(std::string("")));
  ASSERT_TRUE(empty.ends_with(std::string("")));
  ASSERT_FALSE(sv.ends_with(std::string("a_string")));
  ASSERT_FALSE(sv.ends_with(std::string("not")));
  ASSERT_FALSE(empty.ends_with(std::string("not")));
}

TEST(string_view, operation_ends_with_char) {
  const cargo::string_view sv("string"), empty;
  ASSERT_TRUE(sv.ends_with('g'));
  ASSERT_FALSE(sv.ends_with('n'));
  ASSERT_FALSE(empty.ends_with('n'));
}

TEST(string_view, operation_ends_with_string_null_terminates) {
  const cargo::string_view sv("string"), empty;
  ASSERT_TRUE(sv.ends_with("ing"));
  ASSERT_TRUE(sv.ends_with(""));
  ASSERT_TRUE(empty.ends_with(""));
  ASSERT_FALSE(sv.ends_with("a_string"));
  ASSERT_FALSE(sv.ends_with("not"));
  ASSERT_FALSE(empty.ends_with("not"));
}

TEST(string_view, operation_find_no_overflow) {
  std::vector<char> buffer({'0', '1', '2', '3', '4', '5'});
  const cargo::string_view sv(buffer.data(), 3);  // "012"
  ASSERT_EQ(cargo::string_view::npos, sv.find(cargo::string_view("23")));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind(cargo::string_view("23")));
  ASSERT_EQ(cargo::string_view::npos,
            sv.find(cargo::string_view("0123456789012345")));
  ASSERT_EQ(cargo::string_view::npos,
            sv.rfind(cargo::string_view("0123456789012345")));
}

TEST(string_view, operation_find_string_view) {
  const cargo::string_view sv("string");
  ASSERT_EQ(2, sv.find(cargo::string_view("ring")));
  ASSERT_EQ(4, sv.find(cargo::string_view("ng"), 2));
  ASSERT_EQ(cargo::string_view::npos, sv.find(cargo::string_view("!!")));
}

TEST(string_view, operation_find_char) {
  const cargo::string_view sv("string");
  ASSERT_EQ(5, sv.find('g', 2));
  ASSERT_EQ(cargo::string_view::npos, sv.find('!', 2));
}

TEST(string_view, operation_find_string) {
  const cargo::string_view sv("string");
  ASSERT_EQ(2, sv.find("ring"));
  ASSERT_EQ(cargo::string_view::npos, sv.find("!!"));
  ASSERT_EQ(4, sv.find("ng", 2));
  ASSERT_EQ(cargo::string_view::npos, sv.find("!!", 2));
}

TEST(string_view, operation_find_null_string) {
  const cargo::string_view sv("string");
  ASSERT_EQ(2, sv.find("ring"));
  ASSERT_EQ(4, sv.find("ng", 2));
}

TEST(string_view, operation_rfind_string_view) {
  const cargo::string_view sv("string");
  ASSERT_EQ(1, sv.rfind(cargo::string_view("tr")));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind(cargo::string_view("!!")));
  ASSERT_EQ(2, sv.rfind(cargo::string_view("ring"), 3));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind(cargo::string_view("!!"), 3));
}

TEST(string_view, operation_rfind_char) {
  const cargo::string_view sv("string");
  ASSERT_EQ(1, sv.rfind('t'));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind('!'));
  ASSERT_EQ(2, sv.rfind('r', 3));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind('!', 3));
}

TEST(string_view, operation_rfind_string) {
  const cargo::string_view sv("string");
  ASSERT_EQ(1, sv.rfind("tr", 5));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind("!!", 5));
  ASSERT_EQ(2, sv.rfind("ring", 4, 3));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind("!!", 4, 3));
}

TEST(string_view, operation_rfind_null_string) {
  const cargo::string_view sv("string");
  ASSERT_EQ(1, sv.rfind("tr"));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind("!!"));
  ASSERT_EQ(2, sv.rfind("ring", 3));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind("!!", 3));
}

TEST(string_view, operation_find_first_of_string_view) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(3, sv.find_first_of(cargo::string_view(" \n\t\ri")));
  ASSERT_EQ(3, sv.find_first_of(cargo::string_view(" \n\t\ri"), 2));
  ASSERT_EQ(cargo::string_view::npos,
            sv.find_first_of(cargo::string_view("!@#")));
}

TEST(string_view, operation_find_first_of_char) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(3, sv.find_first_of('i'));
  ASSERT_EQ(3, sv.find_first_of('i', 2));
  ASSERT_EQ(cargo::string_view::npos, sv.find_first_of('!'));
}

TEST(string_view, operation_find_first_of_null_string) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(3, sv.find_first_of(" \n\t\ri"));
  ASSERT_EQ(3, sv.find_first_of(" \n\t\ri", 2));
  ASSERT_EQ(cargo::string_view::npos, sv.find_first_of("!@#"));
}

TEST(string_view, operation_find_first_of_string) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(1, sv.find_first_of(" \nt\ri", 0));
  ASSERT_EQ(1, sv.find_first_of(" \nt\ri", 0, 3));
  ASSERT_EQ(cargo::string_view::npos, sv.find_first_of("!@#", 0));
}

TEST(string_view, operation_find_last_of_string_view) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(5, sv.find_last_of(cargo::string_view("sg")));
  ASSERT_EQ(0, sv.find_last_of(cargo::string_view("sg"), 4));
  ASSERT_EQ(cargo::string_view::npos,
            sv.find_last_of(cargo::string_view("!@#")));
}

TEST(string_view, operation_find_last_of_char) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(5, sv.find_last_of('g'));
  ASSERT_EQ(0, sv.find_last_of('s', 4));
  ASSERT_EQ(cargo::string_view::npos, sv.find_last_of('!'));
}

TEST(string_view, operation_find_last_of_string) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(5, sv.find_last_of("gs", sv.size(), 1));
  ASSERT_EQ(0, sv.find_last_of("sg", 0, 2));
  ASSERT_EQ(cargo::string_view::npos, sv.find_last_of("!@#", sv.size(), 1));
}

TEST(string_view, operation_find_last_of_null_string) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(5, sv.find_last_of("sg"));
  ASSERT_EQ(0, sv.find_last_of("sg", 4));
  ASSERT_EQ(cargo::string_view::npos, sv.find_last_of("!@#"));
}

TEST(string_view, operation_find_first_not_of_string_view) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(3, sv.find_first_not_of(cargo::string_view("strng")));
  ASSERT_EQ(3, sv.find_first_not_of(cargo::string_view("trng"), 3));
  ASSERT_EQ(cargo::string_view::npos,
            sv.find_first_not_of(cargo::string_view("string")));
}

TEST(string_view, operation_find_first_not_of_char) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(0, sv.find_first_not_of('i'));
  ASSERT_EQ(4, sv.find_first_not_of('i', 4));
  auto sv2 = cargo::string_view("sssss");
  ASSERT_EQ(cargo::string_view::npos, sv2.find_first_not_of('s'));
}

TEST(string_view, operation_find_first_not_of_string) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(4, sv.find_first_not_of("strg", 4));
  ASSERT_EQ(3, sv.find_first_not_of("trng", 2, 3));
  ASSERT_EQ(cargo::string_view::npos, sv.find_first_not_of("ing", 3, 3));
}

TEST(string_view, operation_find_first_not_of_null_string) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(3, sv.find_first_not_of("strng"));
  ASSERT_EQ(3, sv.find_first_not_of("trng", 3));
  ASSERT_EQ(cargo::string_view::npos, sv.find_first_not_of("string"));
}

TEST(string_view, operation_find_last_not_of_string_view) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(3, sv.find_last_not_of(cargo::string_view("strng")));
  ASSERT_EQ(2, sv.find_last_not_of(cargo::string_view("sting"), 3));
  ASSERT_EQ(cargo::string_view::npos,
            sv.find_last_not_of(cargo::string_view("string")));
}

TEST(string_view, operation_find_last_not_of_char) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(4, sv.find_last_not_of('g'));
  ASSERT_EQ(1, sv.find_last_not_of('r', 2));
  auto sv2 = cargo::string_view("sssss");
  ASSERT_EQ(cargo::string_view::npos, sv2.find_last_not_of('s'));
}

TEST(string_view, operation_find_last_not_of_string) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(3, sv.find_last_not_of("strng", 4));
  ASSERT_EQ(2, sv.find_last_not_of("sting", 2, 4));
  ASSERT_EQ(cargo::string_view::npos, sv.find_last_not_of("string"));
}

TEST(string_view, operation_find_last_not_of_null_string) {
  auto sv = cargo::string_view("string");
  ASSERT_EQ(3, sv.find_last_not_of("strng"));
  ASSERT_EQ(2, sv.find_last_not_of("sting", 3));
  ASSERT_EQ(cargo::string_view::npos, sv.find_last_not_of("string"));
}

TEST(string_view, operation_find_empty_input) {
  const cargo::string_view sv;

  ASSERT_EQ(cargo::string_view::npos, sv.find("string"));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind("string"));
  ASSERT_EQ(cargo::string_view::npos, sv.find(cargo::string_view("string")));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind(cargo::string_view("string")));
  ASSERT_EQ(cargo::string_view::npos, sv.find('s'));
  ASSERT_EQ(cargo::string_view::npos, sv.rfind('s'));

  ASSERT_EQ(cargo::string_view::npos, sv.find_first_of("string"));
  ASSERT_EQ(cargo::string_view::npos, sv.find_last_of("string"));
  ASSERT_EQ(cargo::string_view::npos,
            sv.find_first_of(cargo::string_view("string")));
  ASSERT_EQ(cargo::string_view::npos,
            sv.find_last_of(cargo::string_view("string")));
  ASSERT_EQ(cargo::string_view::npos, sv.find_first_of('s'));
  ASSERT_EQ(cargo::string_view::npos, sv.find_last_of('s'));

  ASSERT_EQ(cargo::string_view::npos, sv.find_first_not_of("string"));
  ASSERT_EQ(cargo::string_view::npos, sv.find_last_not_of("string"));
  ASSERT_EQ(cargo::string_view::npos,
            sv.find_first_not_of(cargo::string_view("string")));
  ASSERT_EQ(cargo::string_view::npos,
            sv.find_last_not_of(cargo::string_view("string")));
  ASSERT_EQ(cargo::string_view::npos, sv.find_first_not_of('s'));
  ASSERT_EQ(cargo::string_view::npos, sv.find_last_not_of('s'));
}

TEST(string_view, non_member_operator_equal) {
  ASSERT_TRUE(cargo::string_view("string") == cargo::string_view("string"));
  ASSERT_FALSE(cargo::string_view("") == cargo::string_view("string"));
  ASSERT_FALSE(cargo::string_view("string") == cargo::string_view("view"));
}

TEST(string_view, non_member_operator_not_equal) {
  ASSERT_FALSE(cargo::string_view("string") != cargo::string_view("string"));
  ASSERT_TRUE(cargo::string_view("") != cargo::string_view("string"));
  ASSERT_TRUE(cargo::string_view("string") != cargo::string_view("view"));
}

TEST(string_view, non_member_operator_less_than) {
  ASSERT_TRUE(cargo::string_view("string") < cargo::string_view("view"));
  ASSERT_FALSE(cargo::string_view("string") < cargo::string_view("string"));
}

TEST(string_view, non_member_operator_less_than_equal) {
  ASSERT_FALSE(cargo::string_view("view") <= cargo::string_view("string"));
  ASSERT_TRUE(cargo::string_view("string") <= cargo::string_view("string"));
}

TEST(string_view, non_member_operator_greater_than) {
  ASSERT_TRUE(cargo::string_view("view") > cargo::string_view("string"));
  ASSERT_FALSE(cargo::string_view("string") > cargo::string_view("string"));
}

TEST(string_view, non_member_operator_greater_than_equal) {
  ASSERT_FALSE(cargo::string_view("string") >= cargo::string_view("view"));
  ASSERT_TRUE(cargo::string_view("string") >= cargo::string_view("string"));
}

TEST(string_view, non_member_operator_ostream) {
  std::stringstream stream;
  const cargo::string_view view("view");
  stream << view;
  ASSERT_TRUE(view == cargo::string_view(stream.str()));
}

TEST(string_view, npos_min) {
  ASSERT_EQ(
      1u, std::min(cargo::string_view::size_type(1), cargo::string_view::npos));
}

TEST(string_view, has_hash) {
  // don't test for the actual hash values, because it's not a part of the
  // interface (implementation could be changed at any time)
  std::vector<cargo::string_view> strs{"", "a", "b", "abc"};
  const std::hash<cargo::string_view> hash{};
  for (size_t i = 0; i < strs.size(); i++) {
    ASSERT_EQ(hash(strs[i]), hash(strs[i]));
    for (size_t j = i + 1; j < strs.size(); j++) {
      ASSERT_NE(hash(strs[i]), hash(strs[j]));
    }
  }
  // check same strings with different addresses;
  std::array<char, 4> str1{"abc"};
  std::array<char, 4> str2{"abc"};
  ASSERT_NE(str1.data(), str2.data());
  ASSERT_EQ(hash(str1), hash(str2));
}

TEST(string_view, as_std_string) {
  std::string s{"string"};
  const cargo::string_view view(s);
  auto s2(cargo::as<std::string>(view));
  ASSERT_EQ(s2.size(), 6);
  ASSERT_FALSE(s2.compare("string"));
  // s2 is not backed by s
  s[3] = 'u';
  ASSERT_FALSE(s2.compare("string"));
}

TEST(string_view, as_std_vector) {
  std::string s{"string"};
  const cargo::string_view view(s);
  auto vector(cargo::as<std::vector<char>>(view));
  ASSERT_EQ(vector.size(), 6);
  ASSERT_TRUE(vector[0] == 's' && vector[3] == 'i' && vector[5] == 'g');
  // vector is not backed by s
  s[3] = 'u';
  ASSERT_TRUE(vector[0] == 's' && vector[3] == 'i' && vector[5] == 'g');
}

TEST(string_view, as_cargo_string_view) {
  std::string s{"string"};
  const cargo::string_view view(s);
  auto view2(cargo::as<cargo::string_view>(view));
  ASSERT_FALSE(view2.compare("string"));
  // view2 is still backed by s
  s[3] = 'u';
  ASSERT_FALSE(view2.compare("strung"));
}
