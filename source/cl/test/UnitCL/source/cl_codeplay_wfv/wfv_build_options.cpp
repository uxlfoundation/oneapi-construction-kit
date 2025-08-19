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
