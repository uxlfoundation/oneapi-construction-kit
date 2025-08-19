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
#include <gtest/gtest.h>

#include "common.h"

/// @file This file contains all tests for the compiler::Target object.

/// @brief Test fixture for testing behaviour of the compiler::Target::init API.
struct InitTest : CompilerTargetTest {};

TEST_P(InitTest, UnsupportedBuiltinCapabilities) {
  // compiler::Target::init must return compiler::Result::INVALID_VALUE if the
  // requested builtin capabilities are not supported.
  ASSERT_EQ(compiler::Result::INVALID_VALUE, target->init(~0x0));
}

INSTANTIATE_COMPILER_TARGET_TEST_SUITE_P(InitTest);
