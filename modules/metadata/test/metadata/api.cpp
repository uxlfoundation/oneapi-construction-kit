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
#include <metadata/detail/utils.h>

#include <cstring>

#include "fixtures.h"

TEST_F(MDAllocatorTest, InitReleaseCtx) {
  md_ctx ctx = md_init(&hooks, &userdata);
  ASSERT_NE(ctx, nullptr);
  EXPECT_TRUE(userdata.allocated);
  md_release_ctx(ctx);
  EXPECT_TRUE(userdata.deallocated);
}

TEST_F(MDApiCtxTest, GetAndCreateBlock) {
  md_stack stack = md_create_block(ctx, "md_block");
  md_stack found_stack = md_get_block(ctx, "md_block");

  ASSERT_EQ(stack, found_stack);
}

TEST_F(MDApiCtxTest, CreateBlockWithDuplicateName) {
  md_stack stack = md_create_block(ctx, "md_block");
  ASSERT_NE(stack, nullptr);
  md_stack stack_same = md_create_block(ctx, "md_block");
  ASSERT_EQ(stack_same, nullptr);
}

TEST_F(MDApiCtxTest, GetNonExistentBlock) {
  md_stack stack = md_get_block(ctx, "md_block");
  ASSERT_EQ(stack, nullptr);
}

TEST_F(MDApiStackTest, PushValueTypes) {
  // UINT
  const int uint_idx = md_push_uint(stack, 3U);
  EXPECT_FALSE(MD_CHECK_ERR(uint_idx));
  md_value val = md_get_value(stack, uint_idx);
  ASSERT_NE(val, nullptr);
  uint64_t uint_val;
  int err = md_get_uint(val, &uint_val);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(uint_val, 3);

  // SINT
  const int sint_idx = md_push_sint(stack, -3);
  EXPECT_FALSE(MD_CHECK_ERR(sint_idx));
  val = md_get_value(stack, sint_idx);
  int64_t sint_val;
  err = md_get_sint(val, &sint_val);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(sint_val, -3);

  // REAL
  const int real_idx = md_push_real(stack, 2.718);
  EXPECT_FALSE(MD_CHECK_ERR(real_idx));
  val = md_get_value(stack, real_idx);
  double real_val;
  err = md_get_real(val, &real_val);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(real_val, 2.718);
}

TEST_F(MDApiStackTest, PushZstr) {
  const int zstr_idx = md_push_zstr(stack, "Hello Metadata");
  EXPECT_FALSE(MD_CHECK_ERR(zstr_idx));
  md_value val = md_get_value(stack, zstr_idx);
  char *out_str;
  size_t str_len;
  const int err = md_get_zstr(val, &out_str, &str_len);
  EXPECT_FALSE(MD_CHECK_ERR(err));

  EXPECT_STREQ(out_str, "Hello Metadata");
  EXPECT_EQ(str_len, 15);

  // Caller must manually deallocate
  hooks.deallocate(out_str, &userdata);
}

TEST_F(MDApiStackTest, PushByteArray) {
  std::vector<uint8_t> bytes = {0x00, 0x01, 0x02, 0x03};
  const int bytestr_idx = md_push_bytes(stack, bytes.data(), bytes.size());
  EXPECT_FALSE(MD_CHECK_ERR(bytestr_idx));
  md_value val = md_get_value(stack, bytestr_idx);
  ASSERT_NE(val, nullptr);

  char *out_bytes;
  size_t out_len;
  const int err = md_get_bytes(val, &out_bytes, &out_len);
  EXPECT_FALSE(MD_CHECK_ERR(err));

  EXPECT_EQ(out_len, 4);
  EXPECT_EQ(out_bytes[0], 0x00);
  EXPECT_EQ(out_bytes[1], 0x01);
  EXPECT_EQ(out_bytes[2], 0x02);
  EXPECT_EQ(out_bytes[3], 0x03);

  // Caller must manually deallocate
  hooks.deallocate(out_bytes, &userdata);
}

TEST_F(MDApiStackTest, PushArray) {
  const int array_idx = md_push_array(stack, 3);
  EXPECT_FALSE(MD_CHECK_ERR(array_idx));

  int uint_idx = md_push_uint(stack, 3);
  int real_idx = md_push_real(stack, 3.141);
  int zstr_idx = md_push_zstr(stack, "Great Heavens!");
  EXPECT_FALSE(MD_CHECK_ERR(uint_idx));
  EXPECT_FALSE(MD_CHECK_ERR(real_idx));
  EXPECT_FALSE(MD_CHECK_ERR(zstr_idx));

  uint_idx = md_array_append(stack, array_idx, uint_idx);
  EXPECT_FALSE(MD_CHECK_ERR(uint_idx));
  real_idx = md_array_append(stack, array_idx, real_idx);
  EXPECT_FALSE(MD_CHECK_ERR(real_idx));
  zstr_idx = md_array_append(stack, array_idx, zstr_idx);
  EXPECT_FALSE(MD_CHECK_ERR(zstr_idx));

  int err = md_pop(stack);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  err = md_pop(stack);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  err = md_pop(stack);
  EXPECT_FALSE(MD_CHECK_ERR(err));

  md_value val = md_get_value(stack, array_idx);
  ASSERT_NE(val, nullptr);

  // Check UINT
  md_value out_val = nullptr;
  err = md_get_array_idx(val, uint_idx, &out_val);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  ASSERT_NE(out_val, nullptr);
  uint64_t out_uint;
  err = md_get_uint(out_val, &out_uint);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(out_uint, 3);

  // Check REAL
  out_val = nullptr;
  err = md_get_array_idx(val, real_idx, &out_val);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  ASSERT_NE(out_val, nullptr);
  double out_real;
  err = md_get_real(out_val, &out_real);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(out_real, 3.141);

  // Check ZSTR
  out_val = nullptr;
  err = md_get_array_idx(val, zstr_idx, &out_val);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  ASSERT_NE(out_val, nullptr);
  char *out_zstr;
  size_t out_size;
  err = md_get_zstr(out_val, &out_zstr, &out_size);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  EXPECT_STREQ(out_zstr, "Great Heavens!");
  EXPECT_EQ(out_size, 15);
  hooks.deallocate(out_zstr, &userdata);
}

TEST_F(MDApiStackTest, PushHashTable) {
  // Setup the stack
  const int hash_idx = md_push_hashtable(stack, 2);
  EXPECT_FALSE(MD_CHECK_ERR(hash_idx));

  const int age_key_idx = md_push_zstr(stack, "Age");
  EXPECT_FALSE(MD_CHECK_ERR(age_key_idx));
  const int age_val_idx = md_push_uint(stack, 11);
  EXPECT_FALSE(MD_CHECK_ERR(age_val_idx));

  const int name_key_idx = md_push_zstr(stack, "Name");
  EXPECT_FALSE(MD_CHECK_ERR(name_key_idx));
  const int name_val_idx = md_push_zstr(stack, "Steven Gerrard");
  EXPECT_FALSE(MD_CHECK_ERR(name_val_idx));

  // Set key/values in the hashtable
  const int age_kv_idx =
      md_hashtable_setkv(stack, hash_idx, age_key_idx, age_val_idx);
  EXPECT_FALSE(MD_CHECK_ERR(age_kv_idx));

  const int name_kv_idx =
      md_hashtable_setkv(stack, hash_idx, name_key_idx, name_val_idx);
  EXPECT_FALSE(MD_CHECK_ERR(name_kv_idx));

  // Get references to the hashtable and the keys
  md_value hash_val = md_get_value(stack, hash_idx);
  ASSERT_NE(hash_val, nullptr);
  md_value age_key = md_get_value(stack, age_key_idx);
  ASSERT_NE(age_key, nullptr);
  md_value name_key = md_get_value(stack, name_key_idx);
  ASSERT_NE(name_key, nullptr);

  // Find the value associated with "Age"
  md_value found_age_val;
  int err = md_get_hashtable_key(hash_val, age_key, &found_age_val);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  uint64_t age;
  err = md_get_uint(found_age_val, &age);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(age, 11);

  // Find the value associated with "Name"
  md_value found_name_val;
  err = md_get_hashtable_key(hash_val, name_key, &found_name_val);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  char *name;
  size_t name_len;
  err = md_get_zstr(found_name_val, &name, &name_len);
  EXPECT_FALSE(MD_CHECK_ERR(err));
  EXPECT_STREQ(name, "Steven Gerrard");
  EXPECT_EQ(name_len, 15);
  hooks.deallocate(name, &userdata);
}

TEST_F(MDApiStackTest, HashTablePushInvalidKey) {
  const int hash_idx = md_push_hashtable(stack, 2);
  EXPECT_FALSE(MD_CHECK_ERR(hash_idx));

  const int byte_arr_idx = md_push_bytes(stack, nullptr, 0);
  EXPECT_FALSE(MD_CHECK_ERR(byte_arr_idx));

  const int uint_idx = md_push_uint(stack, 0);
  EXPECT_FALSE(MD_CHECK_ERR(uint_idx));

  int err = md_hashtable_setkv(stack, hash_idx, byte_arr_idx, uint_idx);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_KEY_ERR);

  // attempt to push to a value that isn't a hashtable
  err = md_hashtable_setkv(stack, byte_arr_idx, uint_idx, uint_idx);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_TYPE_ERR);
}

TEST_F(MDApiStackTest, StackPop) {
  int top = md_pop(stack);
  EXPECT_TRUE(MD_CHECK_ERR(top));

  md_push_uint(stack, 3);
  top = md_top(stack);
  EXPECT_EQ(top, 0);

  md_push_uint(stack, 5);
  top = md_top(stack);
  EXPECT_EQ(top, 1);

  md_pop(stack);
  md_pop(stack);

  top = md_top(stack);
  EXPECT_TRUE(MD_CHECK_ERR(top));
}

TEST_F(MDApiStackTest, GetInvalidType) {
  const int uint_idx = md_push_uint(stack, 3);
  const int sint_idx = md_push_sint(stack, -3);
  EXPECT_FALSE(MD_CHECK_ERR(uint_idx));
  EXPECT_FALSE(MD_CHECK_ERR(sint_idx));
  md_value uint_val = md_get_value(stack, uint_idx);
  md_value sint_val = md_get_value(stack, sint_idx);
  ASSERT_NE(uint_val, nullptr);
  ASSERT_NE(sint_val, nullptr);

  int err = md_get_sint(uint_val, nullptr);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_TYPE_ERR);

  err = md_get_uint(sint_val, nullptr);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_TYPE_ERR);

  err = md_get_real(uint_val, nullptr);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_TYPE_ERR);

  err = md_get_bytes(uint_val, nullptr, nullptr);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_TYPE_ERR);

  err = md_get_array_idx(uint_val, 0, nullptr);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_TYPE_ERR);

  err = md_get_hashtable_key(uint_val, nullptr, nullptr);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_TYPE_ERR);

  err = md_get_zstr(uint_val, nullptr, nullptr);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_TYPE_ERR);
}

TEST_F(MDApiStackTest, md_pushf_md_load_f_SimpleTypes) {
  // FMT => string, byte_str, float, unsigned, signed
  auto fmt_str = "zsfui";

  const char *str_v = "Hello pushf";
  std::vector<uint8_t> bytes_v = {0x01, 0x02, 0x03, 0x04};
  const double real_v = 0.31;
  const uint64_t unsigned_v = 21;
  const int64_t signed_v = -31;
  int err = md_pushf(stack, fmt_str, str_v, bytes_v.size(), bytes_v.data(),
                     real_v, unsigned_v, signed_v);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  // Index of top element should be 4
  EXPECT_EQ(md_top(stack), 4);

  char *out_str;
  size_t out_bytes_len;
  char *out_bytes;
  double out_real;
  uint64_t out_unsigned;
  int64_t out_signed;
  err = md_loadf(stack, fmt_str, &out_str, &out_bytes_len, &out_bytes,
                 &out_real, &out_unsigned, &out_signed);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  EXPECT_STREQ(out_str, str_v);
  EXPECT_EQ(out_real, real_v);
  EXPECT_EQ(out_unsigned, unsigned_v);
  EXPECT_EQ(out_signed, signed_v);

  // deallocate zstr & byte_str
  hooks.deallocate(out_str, &userdata);
  hooks.deallocate(out_bytes, &userdata);
}

TEST_F(MDApiStackTest, md_pushf_md_loadf_ArrayHashTypes) {
  auto fmt_str = "[u,u,{i:f,f:[u]}]z";
  int err = md_pushf(stack, fmt_str, static_cast<uint64_t>(1),
                     static_cast<uint64_t>(2), static_cast<int64_t>(-3), 2.718,
                     3.141, static_cast<uint64_t>(3), "finalize");
  ASSERT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(md_top(stack), 1);

  uint64_t out_arr_pos1;
  uint64_t out_arr_pos2;
  int64_t out_key_pos1;
  double out_val_po1;
  double out_key_pos2;
  uint64_t out_val_pos2;
  char *out_str;

  err = md_loadf(stack, fmt_str, &out_arr_pos1, &out_arr_pos2, &out_key_pos1,
                 &out_val_po1, &out_key_pos2, &out_val_pos2, &out_str);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  EXPECT_EQ(out_arr_pos1, 1);
  EXPECT_EQ(out_arr_pos2, 2);
  EXPECT_EQ(out_key_pos1, -3);
  EXPECT_EQ(out_val_po1, 2.718);
  EXPECT_EQ(out_key_pos2, 3.141);
  EXPECT_EQ(out_val_pos2, 3);
  EXPECT_STREQ(out_str, "finalize");

  // deallocate zstr
  hooks.deallocate(out_str, &userdata);
}

TEST_F(MDApiStackTest, md_pushf_md_loadf_EmptyArrayHash) {
  auto fmt_str = "[]{}";

  int err = md_pushf(stack, fmt_str);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(md_top(stack), 1);

  err = md_loadf(stack, fmt_str);
  ASSERT_FALSE(MD_CHECK_ERR(err));
}

TEST_F(MDApiStackTest, md_pushf_md_loadf_EmptyFmtString) {
  auto fmt_str = "";

  int err = md_pushf(stack, fmt_str);
  EXPECT_EQ(err, md_err::MD_E_EMPTY_STACK);

  err = md_loadf(stack, fmt_str);
  EXPECT_EQ(err, md_err::MD_E_EMPTY_STACK);
}

TEST_F(MDApiStackTest, md_pushf_InvalidFmtStrArray) {
  auto inv_fmt_str = "[u,u";
  const int err = md_pushf(stack, inv_fmt_str, 3UL, 3UL);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_INVALID_FMT_STR);
  // stack should be empty
  EXPECT_EQ(md_top(stack), md_err::MD_E_EMPTY_STACK);
}

TEST_F(MDApiStackTest, md_pushf_InvalidFmtStrHash) {
  auto inv_fmt_str = "[u, u, {u:u]}";
  const int err = md_pushf(stack, inv_fmt_str, static_cast<uint64_t>(3),
                           static_cast<uint64_t>(3), static_cast<uint64_t>(3),
                           static_cast<uint64_t>(3));
  ASSERT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_INVALID_FMT_STR);
}

TEST_F(MDApiStackTest, md_loadf_InvalidFmtStr) {
  int err = md_pushf(stack, "[u, u]z{z:z}", static_cast<uint64_t>(1),
                     static_cast<uint64_t>(2), "Hello", "Name", "Billy");
  ASSERT_FALSE(MD_CHECK_ERR(err));

  // out values
  uint64_t out_1, out_2;
  char *out_zstr_1, out_zstr_2, out_zstr_3;

  auto invalid_type_fmt = "[u,i]z{z:z}";
  err = md_loadf(stack, invalid_type_fmt, &out_1, &out_2, &out_zstr_1,
                 &out_zstr_2, &out_zstr_3);
  EXPECT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_TYPE_ERR);
}

TEST_F(MDApiStackTest, md_pushf_UnsuportedCharacters) {
  const int err = md_pushf(stack, "[u, u]f*&^", static_cast<uint64_t>(3),
                           static_cast<uint64_t>(5), 3.141);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  EXPECT_EQ(err, md_err::MD_E_INVALID_FMT_STR);
}

TEST_F(MDApiStackTest, md_finalize_ctx_FailAfterFinalize) {
  int err = md_push_uint(stack, 33);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  err = md_finalize_block(stack);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  err = md_push_uint(stack, 33);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_push_sint(stack, -33);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_push_real(stack, 3.141);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_push_zstr(stack, "Hello World!");
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_push_hashtable(stack, 0);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_push_array(stack, 0);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_push_bytes(stack, nullptr, 0);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_pop(stack);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_array_append(stack, 0, 0);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_hashtable_setkv(stack, 0, 0, 0);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);

  err = md_finalize_block(stack);
  ASSERT_TRUE(MD_CHECK_ERR(err));
  ASSERT_EQ(err, md_err::MD_E_STACK_FINALIZED);
}

TEST_F(MDApiStackTest, SerializeMsgPack) {
  static std::vector<uint8_t> binary;
  hooks.write = [](void *, const void *data, size_t n) -> md_err {
    binary.insert(binary.end(), (const uint8_t *)data,
                  (const uint8_t *)data + n);
    return MD_SUCCESS;
  };
  hooks.finalize = [](void *) {};
  const char *fmt_str = "[{z:i},u]u{u:u}";
  int err = md_pushf(stack, fmt_str, "metadata", static_cast<int64_t>(-101),
                     static_cast<uint64_t>(42), static_cast<uint64_t>(102),
                     static_cast<uint64_t>(77), static_cast<uint64_t>(44));
  ASSERT_FALSE(MD_CHECK_ERR(err));
  err = md_set_out_fmt(stack, md_fmt::MD_FMT_MSGPACK);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  err = md_finalize_block(stack);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  err = md_finalize_ctx(ctx);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  ASSERT_NE(binary.size(), 0);

  // Deserialize the bytes
  {
    md_hooks read_hooks{};
    read_hooks.map = [](const void *, size_t *n) -> const void * {
      *n = binary.size();
      return binary.data();
    };

    md_ctx read_ctx = md_init(&read_hooks, nullptr);
    ASSERT_NE(read_ctx, nullptr);

    md_stack read_stack = md_get_block(read_ctx, "md_stack");
    ASSERT_NE(read_stack, nullptr);

    char *a;
    int64_t b;
    uint64_t c;
    uint64_t d;
    uint64_t e;
    uint64_t f;

    err = md_loadf(read_stack, fmt_str, &a, &b, &c, &d, &e, &f);
    ASSERT_FALSE(MD_CHECK_ERR(err));
    ASSERT_STREQ(a, "metadata");
    ASSERT_EQ(b, -101);
    ASSERT_EQ(c, 42);
    ASSERT_EQ(d, 102);
    ASSERT_EQ(e, 77);
    ASSERT_EQ(f, 44);

    md_release_ctx(read_ctx);
    hooks.deallocate(a, &userdata);
  }
  binary.clear();
}

TEST_F(MDApiCtxTest, finalizeCtx) {
  static std::vector<uint8_t> out_binary;

  hooks.finalize = [](void *) {};
  hooks.write = [](void *, const void *src, size_t n) -> md_err {
    const uint8_t *data = static_cast<const uint8_t *>(src);
    out_binary.insert(out_binary.end(), data, data + n);
    return md_err::MD_SUCCESS;
  };

  md_stack compiler_md = md_create_block(ctx, "compiler");
  ASSERT_NE(compiler_md, nullptr);
  md_stack host_md = md_create_block(ctx, "host");
  ASSERT_NE(host_md, nullptr);

  int err = md_pushf(compiler_md, "zui", "Compiler Metadata",
                     static_cast<uint64_t>(3), static_cast<int64_t>(-3));
  ASSERT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(md_top(compiler_md), 2);
  err = md_pushf(host_md, "uu", static_cast<uint64_t>(55),
                 static_cast<uint64_t>(1000));
  ASSERT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(md_top(host_md), 1);

  err = md_finalize_block(compiler_md);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  err = md_finalize_block(host_md);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  err = md_finalize_ctx(ctx);
  ASSERT_FALSE(MD_CHECK_ERR(err));

  // attempt to read back the binary

  md_hooks read_hooks{};
  read_hooks.map = [](const void *, size_t *n) -> const void * {
    *n = out_binary.size();
    return out_binary.data();
  };

  md_ctx read_ctx = md_init(&read_hooks, nullptr);
  ASSERT_NE(read_ctx, nullptr);

  md_stack compiler_stack = md_get_block(read_ctx, "compiler");
  ASSERT_NE(compiler_stack, nullptr);
  md_stack host_stack = md_get_block(read_ctx, "host");
  ASSERT_NE(host_stack, nullptr);

  // Since RAW_BYTES was used we only get one item on the stack
  // compiler_md
  char *compiler_md_bytes;
  size_t compiler_md_size;
  err = md_loadf(compiler_stack, "s", &compiler_md_size, &compiler_md_bytes);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  constexpr auto zstr_len = 18;
  EXPECT_TRUE(std::strncmp(compiler_md_bytes, "Compiler Metadata", zstr_len) ==
              0);
  const MD_ENDIAN endianness = md_get_endianness(read_ctx);
  const auto r_uint = md::utils::read_value<uint64_t>(
      (uint8_t *)compiler_md_bytes + zstr_len, endianness);
  EXPECT_EQ(r_uint, 3);

  const auto tmp_sint = md::utils::read_value<uint64_t>(
      (uint8_t *)compiler_md_bytes + zstr_len + 8, endianness);
  const auto sint = cargo::bit_cast<int64_t>(tmp_sint);
  EXPECT_EQ(sint, -3);

  // host_md
  char *host_md_bytes;
  size_t host_md_size;
  err = md_loadf(host_stack, "s", &host_md_size, &host_md_bytes);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(
      md::utils::read_value<uint64_t>((uint8_t *)host_md_bytes, endianness),
      55);
  EXPECT_EQ(
      md::utils::read_value<uint64_t>((uint8_t *)host_md_bytes + 8, endianness),
      1000);

  md_finalize_ctx(read_ctx);
  md_release_ctx(read_ctx);

  // Deallocate
  hooks.deallocate(compiler_md_bytes, &userdata);
  hooks.deallocate(host_md_bytes, &userdata);
}

TEST_F(MDAllocatorTest, DecodeBinary) {
  hooks.map = [](const void *, size_t *n) -> const void * {
    *n = sizeof(example_md_bin);
    return &example_md_bin[0];
  };

  md_ctx ctx = md_init(&hooks, &userdata);
  ASSERT_NE(ctx, nullptr);

  md_stack compiler_md = md_get_block(ctx, "compiler");
  ASSERT_NE(compiler_md, nullptr);
  size_t compiler_size;
  char *compiler_bytes;
  int err = md_loadf(compiler_md, "s", &compiler_size, &compiler_bytes);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(compiler_size, 20);

  md_stack host_md = md_get_block(ctx, "host_md");
  ASSERT_NE(host_md, nullptr);
  size_t host_size;
  char *host_bytes;
  err = md_loadf(host_md, "s", &host_size, &host_bytes);
  ASSERT_FALSE(MD_CHECK_ERR(err));
  EXPECT_EQ(host_size, 14);

  md_finalize_ctx(ctx);
  md_release_ctx(ctx);

  // Deallocate
  hooks.deallocate(compiler_bytes, &userdata);
  hooks.deallocate(host_bytes, &userdata);
}

TEST_F(MDAllocatorTest, DecodeInvalidBinary) {
  hooks.map = [](const void *, size_t *n) -> const void * {
    *n = 0;
    return nullptr;
  };

  md_ctx ctx = md_init(&hooks, &userdata);
  ASSERT_EQ(ctx, nullptr);
}
