// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
