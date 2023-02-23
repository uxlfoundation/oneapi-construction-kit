// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "cl_codeplay_wfv.h"

TEST_F(cl_codeplay_wfv_Test, KernelStatusNone) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=never");
  cl_kernel_wfv_status_codeplay status;
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(kernel, device, 1, nullptr, nullptr,
                                            CL_KERNEL_WFV_STATUS_CODEPLAY,
                                            sizeof(status), &status, nullptr));
  ASSERT_EQ(CL_WFV_NONE_CODEPLAY, status);
}

// Disabling as some targets vectorize and encode in the binary the
// vectorization information - which breaks this path - see CA-4025
TEST_F(cl_codeplay_wfv_Test, DISABLED_KernelStatusSuccess) {
  BuildKernel("__kernel void foo() {}", "foo", "-cl-wfv=always");
  cl_kernel_wfv_status_codeplay status;
  ASSERT_SUCCESS(clGetKernelWFVInfoCODEPLAY(kernel, device, 1, nullptr, nullptr,
                                            CL_KERNEL_WFV_STATUS_CODEPLAY,
                                            sizeof(status), &status, nullptr));
  ASSERT_EQ(CL_WFV_SUCCESS_CODEPLAY, status);
}
