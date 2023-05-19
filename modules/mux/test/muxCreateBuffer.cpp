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

#include "common.h"

struct muxCreateBufferTest : public DeviceTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxCreateBufferTest);

TEST_P(muxCreateBufferTest, Default) {
  mux_buffer_t buffer;

  ASSERT_SUCCESS(muxCreateBuffer(device, 1, allocator, &buffer));

  muxDestroyBuffer(device, buffer, allocator);
}

TEST_P(muxCreateBufferTest, InvalidSize) {
  mux_buffer_t buffer;

  ASSERT_ERROR_EQ(mux_error_invalid_value,
                  muxCreateBuffer(device, 0, allocator, &buffer));
}

TEST_P(muxCreateBufferTest, InvalidOutBuffer) {
  ASSERT_ERROR_EQ(mux_error_null_out_parameter,
                  muxCreateBuffer(device, 1, allocator, 0));
}
