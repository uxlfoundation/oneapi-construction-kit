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

#include "uur/fixtures.h"

using urEnqueueMemBufferReadTest = uur::MemBufferQueueTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urEnqueueMemBufferReadTest);

TEST_P(urEnqueueMemBufferReadTest, Success) {
  std::vector<uint32_t> output(count, 42);
  ASSERT_SUCCESS(urEnqueueMemBufferRead(queue, buffer, true, 0, size,
                                        output.data(), 0, nullptr, nullptr));
}

TEST_P(urEnqueueMemBufferReadTest, InvalidNullHandleQueue) {
  std::vector<uint32_t> output(count, 42);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueMemBufferRead(nullptr, buffer, true, 0, size,
                                          output.data(), 0, nullptr, nullptr));
}

TEST_P(urEnqueueMemBufferReadTest, InvalidNullHandleBuffer) {
  std::vector<uint32_t> output(count, 42);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueMemBufferRead(queue, nullptr, true, 0, size,
                                          output.data(), 0, nullptr, nullptr));
}

TEST_P(urEnqueueMemBufferReadTest, InvalidNullPointerDst) {
  const std::vector<uint32_t> output(count, 42);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urEnqueueMemBufferRead(queue, buffer, true, 0, size, nullptr,
                                          0, nullptr, nullptr));
}

using urEnqueueMemBufferReadMultiDeviceTest =
    uur::MultiDeviceMemBufferQueueTest;

TEST_F(urEnqueueMemBufferReadMultiDeviceTest, WriteReadDifferentQueues) {
  // First queue does a blocking write of 42 into the buffer.
  std::vector<uint32_t> input(count, 42);
  ASSERT_SUCCESS(urEnqueueMemBufferWrite(queues[0], buffer, true, 0, size,
                                         input.data(), 0, nullptr, nullptr));

  // Then the remaining queues do blocking reads from the buffer. Since the
  // queues target different devices this checks that any devices memory has
  // been synchronized.
  for (unsigned i = 1; i < queues.size(); ++i) {
    const auto queue = queues[i];
    std::vector<uint32_t> output(count, 0);
    ASSERT_SUCCESS(urEnqueueMemBufferRead(queue, buffer, true, 0, size,
                                          output.data(), 0, nullptr, nullptr));
    ASSERT_EQ(input, output) << "Result on queue " << i << " did not match!";
  }
}
