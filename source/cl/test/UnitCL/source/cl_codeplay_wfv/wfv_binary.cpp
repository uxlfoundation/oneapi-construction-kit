// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
