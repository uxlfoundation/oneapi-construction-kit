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
#include <metadata/detail/allocator_helper.h>
#include <metadata/detail/md_stack.h>
#include <metadata/detail/metadata_impl.h>
#include <metadata/detail/stack_serializer.h>

#include "fixtures.h"

struct MDStackFinalizeFixture : MDAllocatorTest,
                                ::testing::WithParamInterface<MD_ENDIAN> {};

TEST_P(MDStackFinalizeFixture, FinalizeValueTypes) {
  const md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  md_stack_ stack(helper);
  stack.push_unsigned(33);
  stack.push_signed(-33);
  stack.push_real(2.718);
  stack.push_zstr("Hello World");

  auto endianness = GetParam();

  std::vector<uint8_t> binary;
  stack.finalize(binary, endianness);
  EXPECT_EQ(binary.size(), 8 + 8 + 8 + 12);
  uint8_t *binary_start = binary.data();

  // attempt to read the data back
  auto uint_val =
      md::utils::read_value<md_stack_::unsigned_t>(binary_start, endianness);
  auto tmp_sint = md::utils::read_value<uint64_t>(binary_start + 8, endianness);
  auto sint_val = cargo::bit_cast<md_stack_::signed_t>(tmp_sint);

  auto tmp_real =
      md::utils::read_value<uint64_t>(binary_start + 8 + 8, endianness);
  auto real_val = cargo::bit_cast<md_stack_::real_t>(tmp_real);

  auto str = reinterpret_cast<char *>(binary_start + 8 + 8 + 8);

  EXPECT_EQ(uint_val, 33);
  EXPECT_EQ(sint_val, -33);
  EXPECT_EQ(real_val, 2.718);
  EXPECT_STREQ(str, "Hello World");
}

TEST_P(MDStackFinalizeFixture, FinalizeArrayHashTypes) {
  const md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  md_stack_ stack(helper);

  auto hash_idx = stack.push_map(1).value();
  auto arr_idx = stack.push_arr(1).value();

  auto sint_idx = stack.push_signed(-101).value();
  auto zstr_idx = stack.push_zstr("Hello Metadata").value();
  auto real_idx = stack.push_real(-1.11).value();

  stack.hash_set_kv(hash_idx, sint_idx, zstr_idx);
  stack.arr_append(arr_idx, real_idx);

  auto endianness = GetParam();

  // Remove values from stack
  stack.pop();
  stack.pop();
  stack.pop();

  std::vector<uint8_t> binary;
  stack.finalize(binary, endianness);

  EXPECT_EQ(binary.size(), 8 + 15 + 8);

  // Read the data back from the binary
  uint8_t *binary_start = binary.data();
  auto sint_tmp = md::utils::read_value<uint64_t>(binary_start, endianness);
  auto sint_val = cargo::bit_cast<md_stack_::signed_t>(sint_tmp);

  const std::string md_str =
      std::string(reinterpret_cast<char *>(binary_start + 8));

  auto real_tmp =
      md::utils::read_value<uint64_t>(binary_start + 8 + 15, endianness);
  auto real_val = cargo::bit_cast<md_stack_::real_t>(real_tmp);

  EXPECT_EQ(sint_val, -101);
  EXPECT_STREQ(md_str.c_str(), "Hello Metadata");
  EXPECT_EQ(real_val, -1.11);
}

TEST_P(MDStackFinalizeFixture, MsgPackTest) {
  const md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  const MD_ENDIAN endianness = GetParam();
  md_stack_ stack(helper);
  stack.push_unsigned(1);
  stack.push_unsigned(2);
  stack.push_unsigned(3);

  std::vector<uint8_t> binary;

  stack.set_out_fmt(md_fmt::MD_FMT_MSGPACK);
  stack.finalize(binary, endianness);

  // attempt to read back the data
  md_stack_ read_stack(helper);
  const md::BasicMsgPackStackSerializer<decltype(read_stack)> serializer;
  serializer.deserialize(read_stack, binary.data(), binary.size(), endianness);

  uint64_t a;
  uint64_t b;
  uint64_t c;
  const int err =
      md_loadf(static_cast<md_stack>(&read_stack), "uuu", &a, &b, &c);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  EXPECT_EQ(a, 1);
  EXPECT_EQ(b, 2);
  EXPECT_EQ(c, 3);
}

TEST_P(MDStackFinalizeFixture, MsgPackTestArrays) {
  const md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  std::vector<uint8_t> binary;
  const char *fmt_str = "[u,f,z]";
  const MD_ENDIAN endianness = GetParam();
  {
    md_stack_ stack(helper);
    const int err = md_pushf(static_cast<md_stack>(&stack), fmt_str,
                             static_cast<uint64_t>(3), 3.141, "Hello World!!!");
    ASSERT_FALSE(MD_CHECK_ERR(err));

    // serialize
    stack.set_out_fmt(md_fmt::MD_FMT_MSGPACK);
    stack.finalize(binary, endianness);
  }

  // deserialize
  md_stack_ read_stack(helper);
  const md::BasicMsgPackStackSerializer<decltype(read_stack)> serializer;
  serializer.deserialize(read_stack, binary.data(), binary.size(), endianness);

  uint64_t load_uint;
  double load_double;
  char *load_str;
  const int err = md_loadf(static_cast<md_stack>(&read_stack), fmt_str,
                           &load_uint, &load_double, &load_str);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(load_uint, 3);
  EXPECT_EQ(load_double, 3.141);
  EXPECT_STREQ(load_str, "Hello World!!!");
  hooks.deallocate(load_str, &userdata);
}

TEST_P(MDStackFinalizeFixture, MsgPackTestHashTable) {
  const md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  std::vector<uint8_t> binary;
  const char *fmt_str = "{z:u,u:i}";
  const MD_ENDIAN endianness = GetParam();
  {
    md_stack_ stack(helper);
    const int err =
        md_pushf(static_cast<md_stack>(&stack), fmt_str, "Age",
                 static_cast<uint64_t>(23), static_cast<uint64_t>(42),
                 static_cast<uint64_t>(-42));
    ASSERT_FALSE(MD_CHECK_ERR(err));

    // serialize
    stack.set_out_fmt(md_fmt::MD_FMT_MSGPACK);
    stack.finalize(binary, endianness);
  }

  // deserialize
  md_stack_ read_stack(helper);
  const md::BasicMsgPackStackSerializer<decltype(read_stack)> serializer;
  serializer.deserialize(read_stack, binary.data(), binary.size(), endianness);

  char *key_1_str;
  uint64_t val_1_uint;
  uint64_t key_2_uint;
  int64_t val_2_int;
  const int err = md_loadf(static_cast<md_stack>(&read_stack), fmt_str,
                           &key_1_str, &val_1_uint, &key_2_uint, &val_2_int);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  EXPECT_STREQ(key_1_str, "Age");
  hooks.deallocate(key_1_str, &userdata);
}

TEST_P(MDStackFinalizeFixture, MsgPackTestComplex) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  const char *fmt_str = "[{z:z}, z, [z]]";
  std::vector<uint8_t> binary;
  const MD_ENDIAN endianness = GetParam();
  {
    md_stack_ stack(helper);
    const int err =
        md_pushf(static_cast<md_stack>(&stack), fmt_str, "A", "B", "C", "D");
    ASSERT_FALSE(MD_CHECK_ERR(err));
    stack.set_out_fmt(md_fmt::MD_FMT_MSGPACK);
    stack.finalize(binary, endianness);
  }

  md_stack_ read_stack(helper);
  const md::BasicMsgPackStackSerializer<decltype(read_stack)> serializer;
  serializer.deserialize(read_stack, binary.data(), binary.size(), endianness);
  char *a;
  char *b;
  char *c;
  char *d;
  const int err =
      md_loadf(static_cast<md_stack>(&read_stack), fmt_str, &a, &b, &c, &d);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  ASSERT_STREQ(a, "A");
  ASSERT_STREQ(b, "B");
  ASSERT_STREQ(c, "C");
  ASSERT_STREQ(d, "D");

  hooks.deallocate(a, &userdata);
  hooks.deallocate(b, &userdata);
  hooks.deallocate(c, &userdata);
  hooks.deallocate(d, &userdata);
}
INSTANTIATE_TEST_SUITE_P(FinalizeTests, MDStackFinalizeFixture,
                         ::testing::Values(MD_ENDIAN::BIG, MD_ENDIAN::LITTLE));
