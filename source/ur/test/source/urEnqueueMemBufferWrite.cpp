// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urEnqueueMemBufferWriteTest = uur::MemBufferQueueTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urEnqueueMemBufferWriteTest);

TEST_P(urEnqueueMemBufferWriteTest, Success) {
  std::vector<uint32_t> input(count, 42);
  ASSERT_SUCCESS(urEnqueueMemBufferWrite(queue, buffer, true, 0, size,
                                         input.data(), 0, nullptr, nullptr));
}

TEST_P(urEnqueueMemBufferWriteTest, SuccessWriteRead) {
  std::vector<uint32_t> input(count, 42);
  ASSERT_SUCCESS(urEnqueueMemBufferWrite(queue, buffer, true, 0, size,
                                         input.data(), 0, nullptr, nullptr));
  std::vector<uint32_t> output(count, 0);
  ASSERT_SUCCESS(urEnqueueMemBufferRead(queue, buffer, true, 0, size,
                                        output.data(), 0, nullptr, nullptr));
  for (size_t index = 0; index < count; index++) {
    ASSERT_EQ(input[index], output[index]);
  }
}

TEST_P(urEnqueueMemBufferWriteTest, SuccessWaitEvents) {
  ur_event_handle_t event = nullptr;
  std::vector<uint32_t> input(count, 42);
  ASSERT_SUCCESS(urEnqueueMemBufferWrite(queue, buffer, false, 0, size,
                                         input.data(), 0, nullptr, &event));
  std::vector<uint32_t> output(count, 0);
  ASSERT_SUCCESS(urEnqueueMemBufferRead(queue, buffer, false, 0, size,
                                        output.data(), 1, &event, nullptr));
  ASSERT_SUCCESS(urQueueFinish(queue));
  for (size_t index = 0; index < count; index++) {
    ASSERT_EQ(input[index], output[index]);
  }
  EXPECT_SUCCESS(urEventRelease(event));
}

TEST_P(urEnqueueMemBufferWriteTest, InvalidNullHandleQueue) {
  std::vector<uint32_t> input(count, 42);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueMemBufferWrite(nullptr, buffer, true, 0, size,
                                           input.data(), 0, nullptr, nullptr));
}

TEST_P(urEnqueueMemBufferWriteTest, InvalidNullHandleBuffer) {
  std::vector<uint32_t> input(count, 42);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueMemBufferWrite(queue, nullptr, true, 0, size,
                                           input.data(), 0, nullptr, nullptr));
}

TEST_P(urEnqueueMemBufferWriteTest, InvalidNullHandleEvent) {
  std::vector<uint32_t> input(count, 42);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueMemBufferWrite(queue, buffer, true, 0, size,
                                           input.data(), 1, nullptr, nullptr));
}

TEST_P(urEnqueueMemBufferWriteTest, InvalidNullPointerSrc) {
  std::vector<uint32_t> input(count, 42);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urEnqueueMemBufferWrite(queue, buffer, true, 0, size,
                                           nullptr, 0, nullptr, nullptr));
}
