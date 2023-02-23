// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
