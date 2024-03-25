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

class clCreatePipeTest : public ucl::ContextTest {
 protected:
  virtual void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::ContextTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_F(clCreatePipeTest, NotImplemented) {
  cl_bool pipe_support{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PIPE_SUPPORT,
                                 sizeof(pipe_support), &pipe_support, nullptr));
  if (CL_FALSE != pipe_support) {
    // Since we test against other implementations that may implement this
    // but we aren't actually testing the functionality, just skip.
    GTEST_SKIP();
  }
  const cl_mem_flags flags{};
  const cl_uint pipe_packet_size{};
  const cl_uint pipe_max_packets{};
  const cl_pipe_properties *properties{};
  cl_int errcode{};
  EXPECT_EQ(clCreatePipe(context, flags, pipe_packet_size, pipe_max_packets,
                         properties, &errcode),
            nullptr);
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, errcode);
}
