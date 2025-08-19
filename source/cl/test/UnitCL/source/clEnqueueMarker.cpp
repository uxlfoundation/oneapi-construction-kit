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

#include <vector>

#include "Common.h"

using clEnqueueMarkerTest = ucl::CommandQueueTest;

TEST_F(clEnqueueMarkerTest, Default) {
  cl_event event = nullptr;
  ASSERT_SUCCESS(clEnqueueMarker(command_queue, &event));
  ASSERT_TRUE(event);

  ASSERT_SUCCESS(clReleaseEvent(event));
}

TEST_F(clEnqueueMarkerTest, InvalidCommandQueue) {
  cl_event event = nullptr;
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_QUEUE, clEnqueueMarker(nullptr, &event));
  ASSERT_FALSE(event);
}

TEST_F(clEnqueueMarkerTest, InvalidEvent) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clEnqueueMarker(command_queue, nullptr));
}
