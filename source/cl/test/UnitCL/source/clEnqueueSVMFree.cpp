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

#include "Common.h"

class clEnqueueSVMFreeTest : public ucl::CommandQueueTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_F(clEnqueueSVMFreeTest, NotImplemented) {
  cl_device_svm_capabilities svm_capabilities{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES,
                                 sizeof(svm_capabilities), &svm_capabilities,
                                 nullptr));
  if (0 != svm_capabilities) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  const cl_uint num_svm_pointers{};
  void *svm_pointers[1]{};
  void(CL_CALLBACK * pfn_free_func)(cl_command_queue, cl_uint, void *[],
                                    void *){};
  void *user_data{};
  const cl_uint num_events_in_wait_list{};
  const cl_event *event_wait_list{};
  cl_event *event{};
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clEnqueueSVMFree(command_queue, num_svm_pointers, svm_pointers,
                       pfn_free_func, user_data, num_events_in_wait_list,
                       event_wait_list, event));
}
