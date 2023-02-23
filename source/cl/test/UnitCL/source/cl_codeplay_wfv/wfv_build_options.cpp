// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "cl_codeplay_wfv.h"

TEST_F(cl_codeplay_wfv_Test, clCompileWFVAlways) {
  CompileProgram("", "-cl-wfv=always");
}

TEST_F(cl_codeplay_wfv_Test, clCompileWFVNever) {
  CompileProgram("", "-cl-wfv=never");
}

TEST_F(cl_codeplay_wfv_Test, clCompileWFVAuto) {
  CompileProgram("", "-cl-wfv=auto");
}

TEST_F(cl_codeplay_wfv_Test, clBuildWFVAlways) {
  BuildProgram("", "-cl-wfv=always");
}

TEST_F(cl_codeplay_wfv_Test, clBuildWFVNever) {
  BuildProgram("", "-cl-wfv=never");
}

TEST_F(cl_codeplay_wfv_Test, clBuildWFVAuto) {
  BuildProgram("", "-cl-wfv=auto");
}
