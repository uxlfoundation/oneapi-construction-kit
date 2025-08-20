// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef MD_TEST_FIXTURES_H_INCLUDED
#define MD_TEST_FIXTURES_H_INCLUDED
#include <gtest/gtest.h>
#include <metadata/detail/metadata_impl.h>
#include <metadata/metadata.h>

/// @brief Return if a fatal failure or skip occurred invoking an expression.
///
/// Intended for use in test fixture `SetUp()` calls which explicitly call the
/// base class `SetUp()`, if a fatal error or skip occurs in the base class
/// immediately return to avoid crashing the test suite by using uninitialized
/// state.
///
/// @param ... Expression to invoke.
#define UMD_RETURN_ON_FATAL_FAILURE(...)              \
  __VA_ARGS__;                                        \
  if (this->HasFatalFailure() || this->IsSkipped()) { \
    return;                                           \
  }                                                   \
  (void)0

struct MetadataTest : ::testing::Test {
  md_hooks hooks{};
  void *userdata;
};

struct UserData {
  UserData() : allocated(false), deallocated(false) {}
  bool allocated;
  bool deallocated;
};

struct MDAllocatorTest : MetadataTest {
  void SetUp() override {
    UMD_RETURN_ON_FATAL_FAILURE(MetadataTest::SetUp());
    hooks.allocate = [](size_t size, size_t, void *userdata) -> void * {
      UserData *data = (UserData *)userdata;
      void *p = std::malloc(size);
      data->allocated = true;
      return p;
    };

    hooks.deallocate = [](void *ptr, void *userdata) {
      UserData *data = (UserData *)userdata;
      std::free(ptr);
      data->deallocated = true;
    };

    userdata = UserData{};
  }
  UserData userdata;
};

struct MDApiCtxTest : MDAllocatorTest {
  void SetUp() override {
    UMD_RETURN_ON_FATAL_FAILURE(MDAllocatorTest::SetUp());
    ctx = md_init(&hooks, &userdata);
    ASSERT_NE(ctx, nullptr);
  }

  void TearDown() override {
    MDAllocatorTest::TearDown();
    if (ctx) {
      md_release_ctx(ctx);
    }
  }

  md_ctx ctx = nullptr;
};

struct MDApiStackTest : MDApiCtxTest {
  void SetUp() override {
    UMD_RETURN_ON_FATAL_FAILURE(MDApiCtxTest::SetUp());
    stack = md_create_block(ctx, "md_stack");
    ASSERT_NE(stack, nullptr);
  }
  md_stack stack = nullptr;
};

static constexpr uint8_t example_md_bin[] = {
    /// ** HEADER - 16 bytes ** ///
    // CAMD - MAGIC 4-bytes
    0x43, 0x41, 0x4D, 0x44,

    // Endianness (big-endian) 1-byte
    0x02,

    // Version (1) 1-byte
    0x01,

    // padding 2-bytes
    0x00, 0x00,

    // block_list_offset - 4-bytes (16 + 24)
    0x00, 0x00, 0x00, 0x28,

    // n blocks = 2 : 4-bytes
    0x00, 0x00, 0x00, 0x02,

    /// *** String Table - 24-bytes *** ///
    // text == "compiler" 9-bytes
    0x63, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x00,
    // text == "host_md" 8-bytes
    0x68, 0x6F, 0x73, 0x74, 0x5F, 0x6D, 0x64, 0x00,
    // Pad out to 8-byte alignment 7-bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /// *** Block Info List 48-bytes *** ///

    /// --- "compiler" block info
    // offset -> 8-bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58,
    // size -> 8-bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14,
    // name idx 4-bytes
    0x00, 0x00, 0x00, 0x10,
    // flags 4-bytes
    0x00, 0x00, 0x00, 0x00,

    /// --- "host_md" block info
    // offset -> 8-bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70,
    // size -> 8-bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E,
    // name idx 4-bytes
    0x00, 0x00, 0x00, 0x19,
    // flags 4-bytes
    0x00, 0x00, 0x00, 0x00,

    /// *** Data Blocks *** ///

    // "compiler" -> 20 bytes + (4-bytes padding)
    0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,

    // "host_md" -> 14 bytes + (2-bytes padding)
    0x00, 0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00};

#endif  // MD_TEST_FIXTURES_H_INCLUDED
