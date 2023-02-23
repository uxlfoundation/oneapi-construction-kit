// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urEnqueueUSMMemsetTest = uur::QueueTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urEnqueueUSMMemsetTest);

// RISCV target does not support host memory
TEST_P(urEnqueueUSMMemsetTest, Success) {
  bool host_usm = false;
  size_t size = sizeof(bool);
  ASSERT_SUCCESS(urDeviceGetInfo(device, UR_DEVICE_INFO_HOST_UNIFIED_MEMORY,
                                 size, &host_usm, nullptr));

  ur_event_handle_t event = nullptr;
  if (host_usm) {  // Only test host USM if device supports it
    int *host_ptr{nullptr};
    ur_usm_mem_flags_t flags = 0;
    ASSERT_SUCCESS(urUSMHostAlloc(context, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&host_ptr)));
    ASSERT_SUCCESS(urEnqueueUSMMemset(queue, host_ptr, 0, sizeof(int), 0,
                                      nullptr, &event));
    EXPECT_SUCCESS(urQueueFlush(queue));
    ASSERT_SUCCESS(urEventWait(1, &event));
    EXPECT_SUCCESS(urEventRelease(event));
    ASSERT_EQ(*host_ptr, 0);
    ASSERT_SUCCESS(urMemFree(context, host_ptr));
  }

  int *device_ptr = nullptr;
  ur_usm_mem_flags_t flags = 0;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&device_ptr)));
  ASSERT_SUCCESS(urEnqueueUSMMemset(queue, device_ptr, 1, sizeof(int), 0,
                                    nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  ASSERT_SUCCESS(urEventWait(1, &event));
  EXPECT_SUCCESS(urEventRelease(event));

  int expected;
  std::memset(&expected, 1, sizeof(int));

  ASSERT_EQ(*device_ptr, expected);

  ASSERT_SUCCESS(urMemFree(context, device_ptr));
}

TEST_P(urEnqueueUSMMemsetTest, InvalidNullQueueHandle) {
  int *ptr = nullptr;
  ur_usm_mem_flags_t flags = 0;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&ptr)));
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_HANDLE,
      urEnqueueUSMMemset(nullptr, ptr, 1, sizeof(int), 0, nullptr, nullptr));
  ASSERT_SUCCESS(urMemFree(context, ptr));
}

TEST_P(urEnqueueUSMMemsetTest, InvalidNullPtr) {
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_POINTER,
      urEnqueueUSMMemset(queue, nullptr, 1, sizeof(int), 0, nullptr, nullptr));
}

TEST_P(urEnqueueUSMMemsetTest, InvalidNullPtrEventWaitList) {
  int *ptr = nullptr;
  ur_usm_mem_flags_t flags = 0;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&ptr)));
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_NULL_POINTER,
      urEnqueueUSMMemset(queue, ptr, 1, sizeof(int), 1, nullptr, nullptr));
  ASSERT_SUCCESS(urMemFree(context, ptr));
}

TEST_P(urEnqueueUSMMemsetTest, InvalidMemObject) {
  // Random pointer which is not a usm allocation
  intptr_t address = 0xDEADBEEF;
  int *ptr = reinterpret_cast<int *>(address);
  ASSERT_EQ_RESULT(
      UR_RESULT_ERROR_INVALID_MEM_OBJECT,
      urEnqueueUSMMemset(queue, ptr, 1, sizeof(int), 0, nullptr, nullptr));
}
