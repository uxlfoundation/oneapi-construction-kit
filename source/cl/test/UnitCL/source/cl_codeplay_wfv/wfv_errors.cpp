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

TEST_F(cl_codeplay_wfv_Test, InvalidDevice) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  cl_device_id invalid_device = (cl_device_id)1;
  ASSERT_EQ(CL_INVALID_DEVICE,
            clGetKernelWFVInfoCODEPLAY(kernel, invalid_device, 1, nullptr,
                                       nullptr, CL_KERNEL_WFV_STATUS_CODEPLAY,
                                       0, nullptr, nullptr));
}

TEST_F(cl_codeplay_wfv_Test, InvalidKernel) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  ASSERT_EQ(CL_INVALID_KERNEL,
            clGetKernelWFVInfoCODEPLAY(nullptr, device, 1, nullptr, nullptr,
                                       CL_KERNEL_WFV_STATUS_CODEPLAY, 0,
                                       nullptr, nullptr));
}

TEST_F(cl_codeplay_wfv_Test, InvalidValue) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  ASSERT_EQ(CL_INVALID_VALUE,
            clGetKernelWFVInfoCODEPLAY(kernel, device, 1, nullptr, nullptr, 0x0,
                                       0, nullptr, nullptr));
}

TEST_F(cl_codeplay_wfv_Test, InvalidWorkDimension0) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  ASSERT_EQ(CL_INVALID_WORK_DIMENSION,
            clGetKernelWFVInfoCODEPLAY(kernel, device, 0, nullptr, nullptr,
                                       CL_KERNEL_WFV_STATUS_CODEPLAY, 0,
                                       nullptr, nullptr));
}

TEST_F(cl_codeplay_wfv_Test, InvalidWorkDimensionN) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  const cl_uint invalid_dims = dims + 1;
  ASSERT_EQ(CL_INVALID_WORK_DIMENSION,
            clGetKernelWFVInfoCODEPLAY(kernel, device, invalid_dims, nullptr,
                                       nullptr, CL_KERNEL_WFV_STATUS_CODEPLAY,
                                       0, nullptr, nullptr));
}

TEST_F(cl_codeplay_wfv_Test, InvalidGlobalWorkSize0) {
  if (UCL::isDeviceVersionAtLeast({2, 1})) {
    // Returning an error code for zero dimensional ND range was deprecated by
    // OpenCL 2.1.
    GTEST_SKIP();
  }
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  cargo::small_vector<size_t, 3> global_size;
  ASSERT_EQ(cargo::success, global_size.resize(dims));
  for (cl_uint dim = 0; dim < dims; ++dim) {
    for (size_t i = 0; i < global_size.size(); ++i) {
      global_size[i] = 1;
    }
    global_size[dim] = 0;
    ASSERT_EQ(CL_INVALID_GLOBAL_WORK_SIZE,
              clGetKernelWFVInfoCODEPLAY(
                  kernel, device, dims, global_size.data(), nullptr,
                  CL_KERNEL_WFV_STATUS_CODEPLAY, 0, nullptr, nullptr));
  }
}

TEST_F(cl_codeplay_wfv_Test, InvalidWorkGroupSizeReqd) {
  const char *source =
      "__attribute__((reqd_work_group_size(1, 1, 1))) "
      "__kernel void foo() {}";
  BuildKernel(source, "foo", "-cl-wfv=never");
  cargo::small_vector<size_t, 3> local_size;
  ASSERT_EQ(cargo::success, local_size.resize(dims));
  for (cl_uint dim = 0; dim < dims; ++dim) {
    for (size_t i = 0; i < local_size.size(); ++i) {
      local_size[i] = 1;
    }
    local_size[dim] = 2;
    ASSERT_EQ(CL_INVALID_WORK_GROUP_SIZE,
              clGetKernelWFVInfoCODEPLAY(
                  kernel, device, dims, nullptr, local_size.data(),
                  CL_KERNEL_WFV_STATUS_CODEPLAY, 0, nullptr, nullptr));
  }
}

TEST_F(cl_codeplay_wfv_Test, InvalidWorkGroupSizeMax) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  auto max_work_group_size = getDeviceMaxWorkGroupSize();
  auto max_work_item_size_x = getDeviceMaxWorkItemSizes()[0];
  auto size_y = (max_work_group_size / max_work_item_size_x) + 1;
  size_t local_size[] = {max_work_item_size_x, size_y, 1};
  ASSERT_EQ(CL_INVALID_WORK_GROUP_SIZE,
            clGetKernelWFVInfoCODEPLAY(kernel, device, 2, nullptr, local_size,
                                       CL_KERNEL_WFV_STATUS_CODEPLAY, 0,
                                       nullptr, nullptr));
}

TEST_F(cl_codeplay_wfv_Test, InvalidWorkGroupSize0) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  cargo::small_vector<size_t, 3> local_size;
  ASSERT_EQ(cargo::success, local_size.resize(dims));
  for (cl_uint dim = 0; dim < dims; ++dim) {
    for (size_t i = 0; i < local_size.size(); ++i) {
      local_size[i] = 1;
    }
    local_size[dim] = 0;
    ASSERT_EQ(CL_INVALID_WORK_GROUP_SIZE,
              clGetKernelWFVInfoCODEPLAY(
                  kernel, device, dims, nullptr, local_size.data(),
                  CL_KERNEL_WFV_STATUS_CODEPLAY, 0, nullptr, nullptr));
  }
}

TEST_F(cl_codeplay_wfv_Test, InvalidWorkItemSize) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  cargo::small_vector<size_t, 3> local_size;
  ASSERT_EQ(cargo::success, local_size.resize(dims));
  for (cl_uint dim = 0; dim < dims; ++dim) {
    for (size_t i = 0; i < local_size.size(); ++i) {
      local_size[i] = 1;
    }
    local_size[dim] = getDeviceMaxWorkItemSizes()[dim] + 1;
    // If the max work group size isn't big enough, we can't actually test work
    // item sizes that are too large, so we skip the test.
    if (local_size[dim] > getDeviceMaxWorkGroupSize()) {
      GTEST_SKIP();
    }
    ASSERT_EQ(CL_INVALID_WORK_ITEM_SIZE,
              clGetKernelWFVInfoCODEPLAY(
                  kernel, device, dims, nullptr, local_size.data(),
                  CL_KERNEL_WFV_STATUS_CODEPLAY, 0, nullptr, nullptr));
  }
}
