// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "cl_codeplay_wfv.h"

TEST_F(cl_codeplay_wfv_Test, KernelStatus) {
  BuildKernel("__kernel void foo() {}", "foo", "");
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(kernel, device, 1, nullptr, nullptr,
                                            CL_KERNEL_WFV_STATUS_CODEPLAY, 0,
                                            nullptr, nullptr));
}

TEST_F(cl_codeplay_wfv_Test, KernelStatusSizeRet) {
  BuildKernel("__kernel void foo() {}", "foo", "");
  size_t size;
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(kernel, device, 1, nullptr, nullptr,
                                            CL_KERNEL_WFV_STATUS_CODEPLAY, 0,
                                            nullptr, &size));
  ASSERT_EQ(sizeof(cl_kernel_wfv_status_codeplay), size);
}

TEST_F(cl_codeplay_wfv_Test, KernelWidths) {
  BuildKernel("__kernel void foo() {}", "foo", "");
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(kernel, device, 1, nullptr, nullptr,
                                            CL_KERNEL_WFV_WIDTHS_CODEPLAY, 0,
                                            nullptr, nullptr));
}

TEST_F(cl_codeplay_wfv_Test, KernelWidthsSizeRet) {
  BuildKernel("__kernel void foo() {}", "foo", "");
  size_t size;
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(
      kernel, device, dims, nullptr, nullptr, CL_KERNEL_WFV_WIDTHS_CODEPLAY, 0,
      nullptr, &size));
  ASSERT_EQ(sizeof(size_t) * dims, size);
}
