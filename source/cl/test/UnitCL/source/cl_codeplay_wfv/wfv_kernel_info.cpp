// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
