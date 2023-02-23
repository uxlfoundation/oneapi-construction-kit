// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urEnqueueUSMMemcpyTest = uur::QueueTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urEnqueueUSMMemcpyTest);

TEST_P(urEnqueueUSMMemcpyTest, Success) {
  bool host_usm = false;
  size_t size = sizeof(bool);
  ASSERT_SUCCESS(urDeviceGetInfo(device, UR_DEVICE_INFO_HOST_UNIFIED_MEMORY,
                                 size, &host_usm, nullptr));

  ur_event_handle_t event = nullptr;
  if (host_usm) {  // Only test host USM if device supports it
    int *host_dst = nullptr, *host_src = nullptr;
    ur_usm_mem_flags_t flags = 0;
    ASSERT_SUCCESS(urUSMHostAlloc(context, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&host_dst)));

    ASSERT_SUCCESS(urUSMHostAlloc(context, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&host_src)));
    *host_src = 42;
    *host_dst = 0;
    ASSERT_SUCCESS(urEnqueueUSMMemcpy(queue, false, host_dst, host_src,
                                      sizeof(int), 0, nullptr, &event));
    EXPECT_SUCCESS(urQueueFlush(queue));
    ASSERT_SUCCESS(urEventWait(1, &event));
    EXPECT_SUCCESS(urEventRelease(event));
    ASSERT_EQ(*host_dst, *host_src);
    ASSERT_SUCCESS(urMemFree(context, host_dst));
    ASSERT_SUCCESS(urMemFree(context, host_src));
  }
  int *device_dst = nullptr, *device_src = nullptr;
  ur_usm_mem_flags_t flags = 0;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&device_dst)));
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&device_src)));

  // Fill the allocations with different values first
  ASSERT_SUCCESS(urEnqueueUSMMemset(queue, device_dst, 0, sizeof(int), 0,
                                    nullptr, nullptr));
  ASSERT_SUCCESS(urEnqueueUSMMemset(queue, device_src, 1, sizeof(int), 0,
                                    nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  ASSERT_SUCCESS(urEventWait(1, &event));
  EXPECT_SUCCESS(urEventRelease(event));

  ASSERT_SUCCESS(urEnqueueUSMMemcpy(queue, false, device_dst, device_src,
                                    sizeof(int), 0, nullptr, &event));
  EXPECT_SUCCESS(urQueueFlush(queue));
  EXPECT_SUCCESS(urEventWait(1, &event));
  EXPECT_SUCCESS(urEventRelease(event));

  ASSERT_EQ(*device_dst, *device_src);

  ASSERT_SUCCESS(urMemFree(context, device_dst));
  ASSERT_SUCCESS(urMemFree(context, device_src));
}

TEST_P(urEnqueueUSMMemcpyTest, InvalidNullQueueHandle) {
  int *dst = nullptr, *src = nullptr;
  ur_usm_mem_flags_t flags = 0;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&dst)));

  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&src)));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueUSMMemcpy(nullptr, false, dst, src, sizeof(int), 0,
                                      nullptr, nullptr));
  ASSERT_SUCCESS(urMemFree(context, dst));
  ASSERT_SUCCESS(urMemFree(context, src));
}

TEST_P(urEnqueueUSMMemcpyTest, InvalidNullPtr) {
  // We need a valid pointer to check the params separately.
  int *valid_ptr = nullptr;
  ur_usm_mem_flags_t flags = 0;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&valid_ptr)));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urEnqueueUSMMemcpy(queue, false, valid_ptr, nullptr,
                                      sizeof(int), 0, nullptr, nullptr));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urEnqueueUSMMemcpy(queue, false, nullptr, valid_ptr,
                                      sizeof(int), 0, nullptr, nullptr));
  ASSERT_SUCCESS(urMemFree(context, valid_ptr));
}

TEST_P(urEnqueueUSMMemcpyTest, InvalidNullPtrEventWaitList) {
  int *dst = nullptr, *src = nullptr;
  ur_usm_mem_flags_t flags = 0;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&dst)));

  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&src)));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urEnqueueUSMMemcpy(queue, false, dst, src, sizeof(int), 1,
                                      nullptr, nullptr));
  ASSERT_SUCCESS(urMemFree(context, dst));
  ASSERT_SUCCESS(urMemFree(context, src));
}

TEST_P(urEnqueueUSMMemcpyTest, InvalidMemObject) {
  // We need a valid pointer to check the params separately.
  int *valid_ptr = nullptr;
  ur_usm_mem_flags_t flags = 0;
  ASSERT_SUCCESS(urUSMDeviceAlloc(context, device, &flags, sizeof(int), 0,
                                  reinterpret_cast<void **>(&valid_ptr)));

  // Random pointer which is not a usm allocation
  intptr_t address = 0xDEADBEEF;
  int *bad_ptr = reinterpret_cast<int *>(address);
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_MEM_OBJECT,
                   urEnqueueUSMMemcpy(queue, false, bad_ptr, valid_ptr,
                                      sizeof(int), 0, nullptr, nullptr));

  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_MEM_OBJECT,
                   urEnqueueUSMMemcpy(queue, false, valid_ptr, bad_ptr,
                                      sizeof(int), 0, nullptr, nullptr));
  ASSERT_SUCCESS(urMemFree(context, valid_ptr));
}
