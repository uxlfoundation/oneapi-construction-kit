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
#include <metadata/detail/md_stack.h>

#include "fixtures.h"

struct MDStackTest : public MDAllocatorTest {};

TEST_F(MDStackTest, EmptyStack) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  md_stack_ stack(helper);

  EXPECT_TRUE(stack.empty());

  cargo::expected<size_t, md_err> err_or_idx = stack.top();
  EXPECT_FALSE(err_or_idx.has_value());
  EXPECT_TRUE(MD_CHECK_ERR(err_or_idx.error()));
  EXPECT_EQ(err_or_idx.error(), md_err::MD_E_EMPTY_STACK);

  err_or_idx = stack.pop();
  EXPECT_FALSE(err_or_idx.has_value());
  EXPECT_TRUE(MD_CHECK_ERR(err_or_idx.error()));
  EXPECT_EQ(err_or_idx.error(), md_err::MD_E_EMPTY_STACK);

  stack.push_unsigned(22);

  err_or_idx = stack.top();
  EXPECT_TRUE(err_or_idx.has_value());

  // After popping the stack should be empty
  err_or_idx = stack.pop();
  EXPECT_FALSE(err_or_idx.has_value());
  EXPECT_EQ(err_or_idx.error(), md_err::MD_E_EMPTY_STACK);

  EXPECT_TRUE(stack.empty());
}

TEST_F(MDStackTest, PushValues) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  md_stack_ stack(helper);

  cargo::expected<size_t, md_err> err_or_idx = stack.push_unsigned(33);
  ASSERT_TRUE(err_or_idx.has_value());
  auto top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(err_or_idx.value(), top_idx.value());
  EXPECT_EQ(stack.at(top_idx.value()).get_type(), md_value_type::MD_TYPE_UINT);

  err_or_idx = stack.push_signed(-191);
  ASSERT_TRUE(err_or_idx.has_value());
  top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(err_or_idx.value(), top_idx.value());
  EXPECT_EQ(stack.at(top_idx.value()).get_type(), md_value_type::MD_TYPE_SINT);

  err_or_idx = stack.push_real(3.141592654);
  ASSERT_TRUE(err_or_idx.has_value());
  top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(err_or_idx.value(), top_idx.value());
  EXPECT_EQ(stack.at(top_idx.value()).get_type(), md_value_type::MD_TYPE_REAL);

  err_or_idx = stack.push_zstr("Hello Metadata!");
  ASSERT_TRUE(err_or_idx.has_value());
  top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(err_or_idx.value(), top_idx.value());
  EXPECT_EQ(stack.at(top_idx.value()).get_type(), md_value_type::MD_TYPE_ZSTR);

  err_or_idx = stack.push_map(4);
  ASSERT_TRUE(err_or_idx.has_value());
  top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(err_or_idx.value(), top_idx.value());
  EXPECT_EQ(stack.at(top_idx.value()).get_type(), md_value_type::MD_TYPE_HASH);

  err_or_idx = stack.push_arr(4);
  ASSERT_TRUE(err_or_idx.has_value());
  top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(err_or_idx.value(), top_idx.value());
  EXPECT_EQ(stack.at(top_idx.value()).get_type(), md_value_type::MD_TYPE_ARRAY);

  const uint8_t bytes[] = {0x01, 0x02, 0x03, 0x04};
  const size_t bytes_len = sizeof(bytes) / sizeof(bytes[0]);
  err_or_idx = stack.push_bytes(&bytes[0], bytes_len);
  ASSERT_TRUE(err_or_idx.has_value());
  top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(err_or_idx.value(), top_idx.value());
  EXPECT_EQ(stack.at(top_idx.value()).get_type(),
            md_value_type::MD_TYPE_BYTESTR);
}

TEST_F(MDStackTest, ArrayAppend) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  md_stack_ stack(helper);

  cargo::expected<size_t, md_err> arr_err_or_idx = stack.push_arr(4);
  ASSERT_TRUE(arr_err_or_idx.has_value());
  auto top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(arr_err_or_idx, top_idx.value());
  EXPECT_EQ(stack.at(top_idx.value()).get_type(), md_value_type::MD_TYPE_ARRAY);

  cargo::expected<size_t, md_err> zstr_err_or_idx =
      stack.push_zstr("TO BE APPENDED");
  ASSERT_TRUE(zstr_err_or_idx.has_value());
  top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(zstr_err_or_idx.value(), top_idx.value());

  const cargo::expected<size_t, md_err> append_err_or_idx =
      stack.arr_append(arr_err_or_idx.value(), zstr_err_or_idx.value());
  ASSERT_TRUE(append_err_or_idx.has_value());

  // Value still remains on the stack - caller is responsible for popping it off
  top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(top_idx.value(), 1);
  stack.pop();
}

TEST_F(MDStackTest, ArrayAppendInvalidStackPosition) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  md_stack_ stack(helper);

  // The value is located bellow the array on the stack
  // NOT-ALLOWED!
  cargo::expected<size_t, md_err> zstr_err_or_idx =
      stack.push_zstr("TO BE APPENDED");
  ASSERT_TRUE(zstr_err_or_idx.has_value());
  auto top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(zstr_err_or_idx.value(), top_idx.value());

  cargo::expected<size_t, md_err> arr_err_or_idx = stack.push_arr(4);
  ASSERT_TRUE(arr_err_or_idx.has_value());
  top_idx = stack.top();
  ASSERT_TRUE(top_idx.has_value());
  EXPECT_EQ(stack.at(top_idx.value()).get_type(), md_value_type::MD_TYPE_ARRAY);

  cargo::expected<size_t, md_err> append_err_or_idx =
      stack.arr_append(arr_err_or_idx.value(), zstr_err_or_idx.value());
  ASSERT_FALSE(append_err_or_idx.has_value());
  EXPECT_EQ(append_err_or_idx.error(), md_err::MD_E_INDEX_ERR);
}

TEST_F(MDStackTest, ArrayAppendInvalidType) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  md_stack_ stack(helper);

  cargo::expected<size_t, md_err> zstr_idx = stack.push_zstr("Hello");
  ASSERT_TRUE(zstr_idx.has_value());

  cargo::expected<size_t, md_err> uint_idx = stack.push_unsigned(22);
  ASSERT_TRUE(uint_idx.has_value());

  cargo::expected<size_t, md_err> append_idx_err =
      stack.arr_append(zstr_idx.value(), uint_idx.value());
  ASSERT_FALSE(append_idx_err.has_value());
  EXPECT_EQ(append_idx_err.error(), md_err::MD_E_TYPE_ERR);
}

TEST_F(MDStackTest, HashSetKeyValue) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  md_stack_ stack(helper);

  cargo::expected<size_t, md_err> hash_idx = stack.push_map(2);
  ASSERT_TRUE(hash_idx.has_value());

  cargo::expected<size_t, md_err> key_idx = stack.push_zstr("Age");
  ASSERT_TRUE(key_idx.has_value());

  cargo::expected<size_t, md_err> value_idx = stack.push_unsigned(23);
  ASSERT_TRUE(value_idx.has_value());

  const cargo::expected<size_t, md_err> kv_or_err =
      stack.hash_set_kv(hash_idx.value(), key_idx.value(), value_idx.value());
  ASSERT_TRUE(kv_or_err.has_value());

  // Values remain on the stack
  EXPECT_EQ(stack.top().value_or(0), 2);
}

TEST_F(MDStackTest, HashSetKeyValueInvalidKeyType) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  md_stack_ stack(helper);

  cargo::expected<size_t, md_err> hash_idx = stack.push_map(2);
  ASSERT_TRUE(hash_idx.has_value());

  cargo::expected<size_t, md_err> key_idx = stack.push_arr(1);
  ASSERT_TRUE(key_idx.has_value());

  cargo::expected<size_t, md_err> value_idx = stack.push_unsigned(23);
  ASSERT_TRUE(value_idx.has_value());

  cargo::expected<size_t, md_err> kv_or_err =
      stack.hash_set_kv(hash_idx.value(), key_idx.value(), value_idx.value());
  ASSERT_FALSE(kv_or_err.has_value());
  EXPECT_EQ(kv_or_err.error(), md_err::MD_E_KEY_ERR);
}

TEST_F(MDStackTest, HashSetKeyValueInvalidStackPosition) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  md_stack_ stack(helper);

  cargo::expected<size_t, md_err> key_idx = stack.push_zstr("Age");
  ASSERT_TRUE(key_idx.has_value());

  cargo::expected<size_t, md_err> value_idx = stack.push_unsigned(23);
  ASSERT_TRUE(value_idx.has_value());

  cargo::expected<size_t, md_err> hash_idx = stack.push_map(2);
  ASSERT_TRUE(hash_idx.has_value());

  cargo::expected<size_t, md_err> kv_or_err =
      stack.hash_set_kv(hash_idx.value(), key_idx.value(), value_idx.value());
  ASSERT_FALSE(kv_or_err.has_value());
  EXPECT_EQ(kv_or_err.error(), md_err::MD_E_INDEX_ERR);
}

TEST_F(MDStackTest, HashSetKeyValueInvalidType) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  md_stack_ stack(helper);

  cargo::expected<size_t, md_err> zstr_idx = stack.push_zstr("Not a Hashtable");
  ASSERT_TRUE(zstr_idx.has_value());

  cargo::expected<size_t, md_err> key_idx = stack.push_zstr("Age");
  ASSERT_TRUE(key_idx.has_value());

  cargo::expected<size_t, md_err> value_idx = stack.push_unsigned(23);
  ASSERT_TRUE(value_idx.has_value());

  cargo::expected<size_t, md_err> kv_err =
      stack.hash_set_kv(zstr_idx.value(), key_idx.value(), value_idx.value());
  ASSERT_FALSE(kv_err.has_value());
  EXPECT_EQ(kv_err.error(), md_err::MD_E_TYPE_ERR);
}
