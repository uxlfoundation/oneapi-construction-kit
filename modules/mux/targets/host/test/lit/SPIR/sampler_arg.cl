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

// Bitcode files generated using Khronos' https://github.com/KhronosGroup/SPIR
// modified version of Clang:
//
// clang -cc1 -emit-llvm-bc -triple spir-unknown-unknown
//       -include path/to/opencl_spir.h -O0
//       -o sampler_arg.bc32
//          sampler_arg.cl
//
// clang -cc1 -emit-llvm-bc -triple spir64-unknown-unknown
//       -include path/to/opencl_spir.h -O0
//       -o sampler_arg.bc64
//          sampler_arg.cl

kernel void sampler_arg(global int4 *dst, read_only image2d_t img,
                        sampler_t smplr) {
  size_t gid = get_global_id(0);
  dst[gid] = read_imagei(img, smplr, (int2)(2, 3));
}

// -cl-opt-disable needed to disable inlining and thus verify that the
// spir_fixup pass is setting the correct calling convention
// RUN: oclc %p/Inputs/sampler_arg%spir_extension -cl-options '-cl-opt-disable -x spir -spir-std=1.2' -stage spir | FileCheck %s

// Original function which has been wrapped and know should be unnamed with the SPIR_FUNC calling convention
// CHECK: define spir_func void @0(ptr addrspace(1) %dst, ptr addrspace(1) %img, i32 %smplr)

// New wrapper function which calls into original kernel function, should have SPIR_KERNEL calling convention
// CHECK: define spir_kernel void @sampler_arg(ptr addrspace(1){{( %0)?}}, ptr addrspace(1){{( %1)?}}, ptr addrspace(2){{( %2)?}})

// CHECK: call spir_func void @0(ptr addrspace(1) %0, ptr addrspace(1) %1,
