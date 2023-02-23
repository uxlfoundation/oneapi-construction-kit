// Copyright (C) Codeplay Software Limited. All Rights Reserved.
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
