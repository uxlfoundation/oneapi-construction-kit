// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include "compiler/library.h"

#include <gtest/gtest.h>

#include "common.h"
#include "mux/mux.h"

/// @file This file contains all tests for the standalone functions in the
/// compiler library.

/// @brief Test fixture for testing behaviour of the compiler::available API.
struct GetCompilerForDeviceTest : testing::Test {};

TEST_F(GetCompilerForDeviceTest, NullDeviceInfo) {
  ASSERT_EQ(compiler::getCompilerForDevice(nullptr), nullptr);
}
