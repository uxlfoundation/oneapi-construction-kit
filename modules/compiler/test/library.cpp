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
