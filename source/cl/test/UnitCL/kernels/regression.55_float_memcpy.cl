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

// REQUIRES: noclc; nospirv;

// Note: This kernel isn't valid OpenCL C since __builtin_memcpy is a clang
//       specific intrinsic. Therefore we only have a SpirExecution test for
//       the generated SPIR, omitting the Execution test for this .cl file.
//       The SPIR-V version of this test uses spvasm files built manually using
//       the legacy SPIR generator, because modern clang can't compile this
//       test to SPIR (hence the nospirv above). See
//       spirv_regression.55_float_memcpy.spvasm{32|64}.
__kernel void float_memcpy(__global float *in, __global float *out, int size) {
  __builtin_memcpy((void*)out,(void*)in, size);
}
