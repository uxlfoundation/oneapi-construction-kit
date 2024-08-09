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

#define CL_USE_DEPRECATED_OPENCL_1_0_APIS 1
#include "Common.h"

class clSetCommandQueuePropertyTest : public ucl::CommandQueueTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
  }
};

TEST_F(clSetCommandQueuePropertyTest, NotImplemented) {
  cl_command_queue_properties old_properties;
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clSetCommandQueueProperty(
                        command_queue, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
                        CL_TRUE, &old_properties));
}
