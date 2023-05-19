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

// REQUIRES: parameters

// Reduced from OpenCL 1.2 CTS test_convert_short3_uchar3:
//     ./conformance_test_conversions -3 short_uchar
__kernel void short3_uchar3_codegen(__global uchar *src, __global short *dest) {
  size_t gid = get_global_id(0);
  uchar3 in = vload3(gid, src);
  short3 out = CONVERT_FUNCTION(in);
  vstore3(out, gid, dest);
}
