// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "Common.h"

class clEnqueueSVMMigrateMemTest : public ucl::CommandQueueTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_F(clEnqueueSVMMigrateMemTest, NotImplemented) {
  cl_device_svm_capabilities svm_capabilities{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES,
                                 sizeof(svm_capabilities), &svm_capabilities,
                                 nullptr));
  if (0 != svm_capabilities) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  cl_uint num_svm_pointers{};
  const void **svm_pointers{};
  const size_t *sizes{};
  cl_mem_migration_flags flags{};
  cl_uint num_events_in_wait_list{};
  const cl_event *event_wait_list{};
  cl_event *event{};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueSVMMigrateMem(command_queue, num_svm_pointers, svm_pointers,
                             sizes, flags, num_events_in_wait_list,
                             event_wait_list, event));
}
