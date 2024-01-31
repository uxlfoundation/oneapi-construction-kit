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

#include <compiler/context.h>
#include <gtest/gtest.h>

#include "cargo/error.h"
#include "common.h"
#include "mux/utils/helpers.h"

struct muxCreateExecutableTest : DeviceCompilerTest {
  cargo::array_view<std::uint8_t> buffer;

  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());
    ASSERT_EQ(compiler::Result::SUCCESS,
              createBinary("kernel void nop() {}", buffer));
  }
};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateExecutableTest);

TEST_P(muxCreateExecutableTest, Default) {
  mux_executable_t executable;
  ASSERT_SUCCESS(muxCreateExecutable(device, buffer.data(), buffer.size(),
                                     allocator, &executable));

  muxDestroyExecutable(device, executable, allocator);
}

TEST_P(muxCreateExecutableTest, InvalidSource) {
  mux_executable_t executable;
  const uint64_t length = 1;

  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCreateExecutable(device, 0, length, allocator, &executable));
}

TEST_P(muxCreateExecutableTest, InvalidSourceLength) {
  mux_executable_t executable;
  ASSERT_ERROR_EQ(
      mux_error_invalid_value,
      muxCreateExecutable(device, buffer.data(), 0, allocator, &executable));
}

TEST_P(muxCreateExecutableTest, InvalidOutExecutable) {
  ASSERT_ERROR_EQ(
      mux_error_null_out_parameter,
      muxCreateExecutable(device, buffer.data(), buffer.size(), allocator, 0));
}
