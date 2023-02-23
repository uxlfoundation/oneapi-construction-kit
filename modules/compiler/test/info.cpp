// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <gtest/gtest.h>

#include "common.h"
#include "compiler/library.h"
#include "mux/mux.h"

/// @file This file contains all tests for the compiler::Info object.

/// @brief Test fixture for testing behaviour of the
/// compiler::Info::createTarget API.
struct CreateTargetTest : CompilerContextTest {};

TEST_P(CreateTargetTest, InvalidContext) {
  ASSERT_EQ(compiler_info->createTarget(nullptr, nullptr), nullptr);
}

INSTANTIATE_COMPILER_TARGET_TEST_SUITE_P(CreateTargetTest);
