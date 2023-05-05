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

#include <cargo/small_vector.h>

#include "cl_codeplay_wfv.h"

TEST_F(cl_codeplay_wfv_BinaryTest, KernelStatus) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  cl_kernel_wfv_status_codeplay status;
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(
      kernel, device, dims, nullptr, nullptr, CL_KERNEL_WFV_STATUS_CODEPLAY,
      sizeof(status), &status, nullptr));
  ASSERT_EQ(CL_WFV_NONE_CODEPLAY, status);
}

TEST_F(cl_codeplay_wfv_BinaryTest, KernelWidths) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  cargo::small_vector<size_t, 3> widths;
  ASSERT_EQ(cargo::success, widths.resize(dims));
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(
      kernel, device, dims, nullptr, nullptr, CL_KERNEL_WFV_WIDTHS_CODEPLAY,
      sizeof(size_t) * widths.size(), widths.data(), nullptr));
  for (auto width : widths) {
    ASSERT_EQ(1, width);
  }
}

TEST_F(cl_codeplay_wfv_BinaryTest, KernelStatusReqd) {
  const char *source =
      "__attribute__((reqd_work_group_size(4, 1, 1))) "
      "__kernel void foo() {}";
  BuildKernel(source, "foo", "-cl-wfv=always");
  cl_kernel_wfv_status_codeplay status;
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(
      kernel, device, dims, nullptr, nullptr, CL_KERNEL_WFV_STATUS_CODEPLAY,
      sizeof(status), &status, nullptr));
  ASSERT_EQ(CL_WFV_NONE_CODEPLAY, status);
}

TEST_F(cl_codeplay_wfv_BinaryTest, KernelWidthsReqd) {
  const char *source =
      "__attribute__((reqd_work_group_size(4, 1, 1))) "
      "__kernel void foo() {}";
  BuildKernel(source, "foo", "-cl-wfv=always");
  cargo::small_vector<size_t, 3> widths;
  ASSERT_EQ(cargo::success, widths.resize(dims));
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(
      kernel, device, dims, nullptr, nullptr, CL_KERNEL_WFV_WIDTHS_CODEPLAY,
      sizeof(size_t) * widths.size(), widths.data(), nullptr));
  for (auto width : widths) {
    ASSERT_EQ(1, width);
  }
}
