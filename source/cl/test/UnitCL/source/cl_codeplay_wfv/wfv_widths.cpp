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

#include "cl_codeplay_wfv.h"

// Disabling as some targets vectorize and encode in the binary the
// vectorization information - which breaks this path - see CA-4025
// Also note further issues with vector width assumptions (CA-3980)
TEST_F(cl_codeplay_wfv_Test, DISABLED_KernelWidthsX) {
  const char *source =
      "__attribute__((reqd_work_group_size(4, 1, 1))) "
      "__kernel void foo() {}";
  BuildKernel(source, "foo", "-cl-wfv=always");
  std::array<size_t, 3> widths;
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(
      kernel, device, widths.size(), nullptr, nullptr,
      CL_KERNEL_WFV_WIDTHS_CODEPLAY, sizeof(size_t) * widths.size(),
      widths.data(), nullptr));
  ASSERT_EQ(4, widths[0]);
  ASSERT_EQ(1, widths[1]);
  ASSERT_EQ(1, widths[2]);
}

// Disabling as some targets vectorize and encode in the binary the
// vectorization information - which breaks this path - see CA-4025
// Also note further issues with vector width assumptions (CA-3980)
TEST_F(cl_codeplay_wfv_Test, DISABLED_KernelWidthsY) {
  const char *source =
      "__attribute__((reqd_work_group_size(1, 4, 1))) "
      "__kernel void foo() {}";
  BuildKernel(source, "foo", "-cl-wfv=always");
  std::array<size_t, 3> widths;
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(
      kernel, device, widths.size(), nullptr, nullptr,
      CL_KERNEL_WFV_WIDTHS_CODEPLAY, sizeof(size_t) * widths.size(),
      widths.data(), nullptr));
  ASSERT_EQ(1, widths[0]);
  ASSERT_EQ(4, widths[1]);
  ASSERT_EQ(1, widths[2]);
}

// Disabling as some targets vectorize and encode in the binary the
// vectorization information - which breaks this path - see CA-4025
// Also note further issues with vector width assumptions (CA-3980)
TEST_F(cl_codeplay_wfv_Test, DISABLED_KernelWidthsZ) {
  const char *source =
      "__attribute__((reqd_work_group_size(1, 1, 4))) "
      "__kernel void foo() {}";
  BuildKernel(source, "foo", "-cl-wfv=always");
  std::array<size_t, 3> widths;
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(
      kernel, device, widths.size(), nullptr, nullptr,
      CL_KERNEL_WFV_WIDTHS_CODEPLAY, sizeof(size_t) * widths.size(),
      widths.data(), nullptr));
  ASSERT_EQ(1, widths[0]);
  ASSERT_EQ(1, widths[1]);
  ASSERT_EQ(4, widths[2]);
}
