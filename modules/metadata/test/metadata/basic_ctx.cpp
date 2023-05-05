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
#include <metadata/detail/md_ctx.h>

#include "fixtures.h"

struct ContextTest : public MDAllocatorTest {};

TEST_F(ContextTest, CreateBlock) {
  md_ctx_ ctx(&hooks, &userdata);
  auto stack = ctx.create_block("kernel_metadata");
  ASSERT_TRUE(stack.has_value());
  stack.value()->push_zstr("Kernel Arguments");
}

TEST_F(ContextTest, CreateExistingBlock) {
  md_ctx_ ctx(&hooks, &userdata);
  auto stack = ctx.create_block("kernel_metadata");
  ASSERT_TRUE(stack.has_value());

  auto same_stack = ctx.create_block("kernel_metadata");
  ASSERT_FALSE(same_stack.has_value());
  EXPECT_EQ(same_stack.error(), md_err::MD_E_STACK_ALREADY_REGISTERED);
}

TEST_F(ContextTest, GetBlock) {
  md_ctx_ ctx(&hooks, &userdata);
  {
    auto stack = ctx.create_block("kernel_metadata");
    ASSERT_TRUE(stack.has_value());

    auto zstr_idx = stack.value()->push_zstr("Args");
    EXPECT_TRUE(zstr_idx.has_value());
  }

  auto stack = ctx.get_block("kernel_metadata");
  ASSERT_TRUE(stack.has_value());
}

TEST_F(ContextTest, GetNonExistantBlock) {
  md_ctx_ ctx(&hooks, &userdata);
  auto block = ctx.get_block("kernel_metadata");
  ASSERT_FALSE(block.has_value());
  EXPECT_EQ(block.error(), md_err::MD_E_STACK_NOT_REGISTERED);
}
