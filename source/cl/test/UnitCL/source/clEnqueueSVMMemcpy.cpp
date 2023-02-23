// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clEnqueueSVMMemcpyTest : public ucl::CommandQueueTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_F(clEnqueueSVMMemcpyTest, NotImplemented) {
  cl_device_svm_capabilities svm_capabilities{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES,
                                 sizeof(svm_capabilities), &svm_capabilities,
                                 nullptr));
  if (0 != svm_capabilities) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  cl_bool blocking_copy{};
  void *dst_ptr{};
  void *src_ptr{};
  size_t size{};
  cl_uint num_events_in_wait_list{};
  const cl_event *event_wait_list{};
  cl_event *event{};
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueSVMMemcpy(command_queue, blocking_copy, dst_ptr, src_ptr, size,
                         num_events_in_wait_list, event_wait_list, event));
}
