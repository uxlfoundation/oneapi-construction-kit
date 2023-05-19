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

// This test is testing device-dependent OpenCL C behaviour, it doesnt make
// sense in SPIR or SPIR-V form.
// REQUIRES: nospir; nospirv;

__kernel void double_constant(__global float* out) {
  // 0x1.0p-126 is FLT_MIN (but without the f suffix)
  // 4294967296.0 is 2^32, chosen such that FLT_MIN/(2^32) is not representable
  // as a single precision float denormal. If doubles are supported, this
  // returns FLT_MIN. Otherwise, it will underflow to 0.
  out[0] = (0x1.0p-126 / 4294967296.0) * 4294967296.0;
}
