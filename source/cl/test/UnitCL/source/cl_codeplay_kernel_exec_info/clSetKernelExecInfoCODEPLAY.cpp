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

#include "cl_codeplay_kernel_exec_info.h"

TEST_F(clSetKernelExecInfoCODEPLAYTest, InvalidKernel) {
  cl_bool param_value = 1;
  EXPECT_EQ_ERRCODE(CL_INVALID_KERNEL,
                    clSetKernelExecInfoCODEPLAY(nullptr, 0, sizeof(param_value),
                                                &param_value));
}

TEST_F(clSetKernelExecInfoCODEPLAYTest, InvalidValue) {
  cl_bool param_value = 1;
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clSetKernelExecInfoCODEPLAY(kernel, 0, 0, &param_value));
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clSetKernelExecInfoCODEPLAY(kernel, 0, sizeof(param_value), nullptr));
}
